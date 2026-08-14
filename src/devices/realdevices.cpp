#include "realdevices.h"
#include "rfidpayloadcodec.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <new>
#include <stdexcept>

#ifdef TLS401_HAS_RFID_SDK
#define HMODULE RFIDLIB_HMODULE
#include "rfidlib_reader.h"
#include "rfidlib_aip_iso15693.h"
#undef HMODULE
#endif

#ifdef TLS401_HAS_IDS_PEAK
#include <peak/peak.hpp>
#include <peak_ipl/peak_ipl.hpp>
#include <memory>
#endif

namespace {
void state(QObject *object, DeviceState value, const QString &message)
{
    if (auto *rfid = qobject_cast<IRfidService *>(object)) emit rfid->stateChanged(value, message);
    if (auto *camera = qobject_cast<ICameraService *>(object)) emit camera->stateChanged(value, message);
}

void logCcd(const QString &message)
{
    static QMutex logMutex;
    QMutexLocker lock(&logMutex);
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty() || !QDir().mkpath(QDir(root).filePath(QStringLiteral("logs")))) return;
    QFile file(QDir(root).filePath(QStringLiteral("logs/ccd.log")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) return;
    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << ' ' << message << '\n';
}

#ifdef TLS401_HAS_RFID_SDK
struct RfidObservation { QString uid; quint32 tagType = 0; quint32 antenna = 0; };

static bool inventoryOnce(RFID_READER_HANDLE reader, QVector<RfidObservation> *observations, QString *error)
{
    const QByteArray antennaIds(1, char(1));
    RFID_DN_HANDLE params = RDR_CreateInvenParamSpecList();
    if (!params) { if (error) *error = QStringLiteral("Cannot allocate RFID inventory parameters."); return false; }
    ISO15693_CreateInvenParam(params, 0, false, 0, 0);
    const err_t inventoryResult = RDR_TagInventory(reader, AI_TYPE_NEW, 1,
                                                     reinterpret_cast<BYTE *>(const_cast<char *>(antennaIds.constData())), params);
    DNODE_Destroy(params);
    if (inventoryResult != NO_ERR) { if (error) *error = QStringLiteral("RFID inventory failed (%1).").arg(inventoryResult); return false; }
    RFID_DN_HANDLE report = RDR_GetTagDataReport(reader, RFID_SEEK_FIRST);
    while (report) {
        DWORD aip = 0, type = 0, antenna = 0, readCount = 0; BYTE dsfid = 0, uid[8] = {}; WORD rssi = 0;
        if (ISO15693_ParseTagDataReportEx(report, &aip, &type, &antenna, &dsfid, &rssi, &readCount, uid) == NO_ERR) {
            RfidObservation item; item.uid = QString::fromLatin1(QByteArray(reinterpret_cast<char *>(uid), 8).toHex().toUpper()); item.tagType = type; item.antenna = antenna; observations->append(item);
        }
        report = RDR_GetTagDataReport(reader, RFID_SEEK_NEXT);
    }
    return true;
}

static bool readTagPayload(RFID_READER_HANDLE reader, const RfidObservation &observation, RfidResult *result)
{
    if (RDR_SetAcessAntenna(reader, 1) != NO_ERR) {
        result->error = RfidError::NoTag;
        result->message = QStringLiteral("RFID antenna 1 could not be selected.");
        return false;
    }
    RFID_TAG_HANDLE tag = nullptr;
    QByteArray uid = QByteArray::fromHex(observation.uid.toLatin1());
    if (ISO15693_Connect(reader, observation.tagType, 1, reinterpret_cast<BYTE *>(uid.data()), &tag) != NO_ERR || !tag) { result->error = RfidError::NoTag; result->message = QStringLiteral("RFID tag connection failed."); return false; }
    BYTE tagUid[8] = {}; BYTE dsfid = 0, afi = 0, icRef = 0; DWORD blockSize = 0, blockCount = 0;
    if (ISO15693_GetSystemInfo(reader, tag, tagUid, &dsfid, &afi, &blockSize, &blockCount, &icRef) != NO_ERR || blockSize == 0 || blockCount == 0) { RDR_TagDisconnect(reader, tag); result->error = RfidError::InvalidPayload; result->message = QStringLiteral("RFID tag system information is invalid."); return false; }
    QByteArray first(static_cast<int>(blockSize), 0); DWORD blocksRead = 0, bytesRead = 0;
    if (ISO15693_ReadMultiBlocks(reader, tag, false, 0, 1, &blocksRead, reinterpret_cast<BYTE *>(first.data()), static_cast<DWORD>(first.size()), &bytesRead) != NO_ERR || bytesRead == 0) { RDR_TagDisconnect(reader, tag); result->error = RfidError::InvalidPayload; result->message = QStringLiteral("RFID tag read failed."); return false; }
    const int length = first.size() > 1 ? static_cast<unsigned char>(first.at(1)) : 0;
    const int blocks = qMax(1, (qMax(length, 16) + static_cast<int>(blockSize) - 1) / static_cast<int>(blockSize));
    QByteArray raw(blocks * static_cast<int>(blockSize), 0); if (ISO15693_ReadMultiBlocks(reader, tag, false, 0, static_cast<DWORD>(blocks), &blocksRead, reinterpret_cast<BYTE *>(raw.data()), static_cast<DWORD>(raw.size()), &bytesRead) != NO_ERR) { RDR_TagDisconnect(reader, tag); result->error = RfidError::InvalidPayload; result->message = QStringLiteral("RFID tag payload read failed."); return false; }
    RDR_TagDisconnect(reader, tag);
    DecodedRfidPayload decoded; QString decodeError; if (!RfidPayloadCodec::decode(raw, &decoded, &decodeError)) { result->error = RfidError::InvalidPayload; result->message = decodeError; return false; }
    result->profile.uid = observation.uid; result->profile.dishNumber = QString::number(decoded.dishNumber); result->profile.inseminationTime = decoded.inseminationTime; result->profile.femaleName = decoded.femaleName; result->profile.medicalRecordNumber = decoded.medicalRecordNumber; result->profile.identifiedAt = QDateTime::currentDateTime(); result->message = QStringLiteral("RFID tag recognized successfully."); return true;
}
#endif

#ifdef TLS401_HAS_IDS_PEAK
struct CameraContext {
    std::shared_ptr<peak::core::Device> device;
    std::shared_ptr<peak::core::DataStream> stream;
    std::shared_ptr<peak::core::NodeMap> nodeMap;
    QMutex mutex;
    QImage latest;
};

struct CameraFrameConfig {
    qint64 width = 0;
    qint64 height = 0;
    qint64 payload = 0;
    QString mode;
};

constexpr quint64 kCameraMemoryBudgetBytes = 512ull * 1024 * 1024;

bool restoreMaximumRoi(const std::shared_ptr<peak::core::NodeMap> &nodeMap)
{
    try {
        const auto width = nodeMap->FindNode<peak::core::nodes::IntegerNode>("Width");
        const auto height = nodeMap->FindNode<peak::core::nodes::IntegerNode>("Height");
        try { const auto offset = nodeMap->FindNode<peak::core::nodes::IntegerNode>("OffsetX"); offset->SetValue(offset->Minimum()); } catch (...) {}
        try { const auto offset = nodeMap->FindNode<peak::core::nodes::IntegerNode>("OffsetY"); offset->SetValue(offset->Minimum()); } catch (...) {}
        width->SetValue(width->Maximum());
        height->SetValue(height->Maximum());
        return true;
    } catch (...) {
        return false;
    }
}

CameraFrameConfig configureCameraFrame(const std::shared_ptr<peak::core::NodeMap> &nodeMap)
{
    auto payloadNode = nodeMap->FindNode<peak::core::nodes::IntegerNode>("PayloadSize");
    auto widthNode = nodeMap->FindNode<peak::core::nodes::IntegerNode>("Width");
    auto heightNode = nodeMap->FindNode<peak::core::nodes::IntegerNode>("Height");

    // Earlier versions repeatedly halved the camera ROI whenever preview was
    // opened. Recover only from that invalid one-line image state; otherwise
    // preserve the operator's camera resolution and binning settings.
    if (widthNode->Value() < 32 || heightNode->Value() < 32) {
        if (!restoreMaximumRoi(nodeMap))
            throw std::runtime_error("The camera ROI is invalid and could not be restored.");
    }

    qint64 payload = payloadNode->Value();
    qint64 width = widthNode->Value();
    qint64 height = heightNode->Value();
    if (static_cast<quint64>(payload) * 5ull + static_cast<quint64>(width) * static_cast<quint64>(height) * 6ull > kCameraMemoryBudgetBytes)
        throw std::runtime_error("CCD frame size exceeds the 32-bit process memory budget. Reduce the camera resolution in the IDS configuration before starting preview.");
    return {width, height, payload, QStringLiteral("camera configuration")};
}
#endif
}

RealRfidService::~RealRfidService() { cancel();
#ifdef TLS401_HAS_RFID_SDK
    if (m_reader) { RDR_Close(reinterpret_cast<RFID_READER_HANDLE>(m_reader)); m_reader = nullptr; }
#endif
}

void RealRfidService::initialize()
{
#ifdef TLS401_HAS_RFID_SDK
    const QString driverDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("Drivers"));
    const QString configuredConnection = QProcessEnvironment::systemEnvironment().value(QStringLiteral("TLS401_RFID_CONNECTION"));
    const QString connection = configuredConnection.isEmpty()
        ? QStringLiteral("RDType=RD5200;CommType=COM;COMName=COM9;BaudRate=38400;Frame=8E1;BusAddr=255")
        : configuredConnection;
    if (RDR_LoadReaderDrivers(reinterpret_cast<LPCTSTR>(const_cast<ushort *>(driverDir.utf16()))) != NO_ERR) { emit stateChanged(DeviceState::Error, QStringLiteral("RFID driver loading failed.")); return; }
    RFID_READER_HANDLE reader = nullptr;
    if (RDR_Open(reinterpret_cast<LPCTSTR>(const_cast<ushort *>(connection.utf16())), &reader) != NO_ERR || !reader) { emit stateChanged(DeviceState::Error, QStringLiteral("RFID reader open failed (RD5200 / COM9 / 38400 / 8E1).")); return; }
    if (RDR_SetAcessAntenna(reader, 1) != NO_ERR) { RDR_Close(reader); emit stateChanged(DeviceState::Error, QStringLiteral("RFID antenna 1 could not be selected.")); return; }
    m_reader = reader; m_ready = true; emit stateChanged(DeviceState::Ready, QStringLiteral("RFID reader connected."));
#else
    emit stateChanged(DeviceState::Error, QStringLiteral("RFID Win32 SDK is not configured."));
#endif
}

void RealRfidService::recognize()
{
#ifdef TLS401_HAS_RFID_SDK
    if (!m_ready || !m_reader) { emit stateChanged(DeviceState::Error, QStringLiteral("RFID reader is not ready.")); return; }
    if (m_thread && m_thread->isRunning()) return;
    m_cancelled = false; emit stateChanged(DeviceState::Busy, QStringLiteral("Reading RFID tag."));
    m_thread = QThread::create([this] {
        QVector<RfidObservation> observations; QString error;
        for (int i = 0; i < 50 && !m_cancelled; ++i) {
            QString attemptError;
            if (!inventoryOnce(reinterpret_cast<RFID_READER_HANDLE>(m_reader), &observations, &attemptError)) {
                error = attemptError;
            } else if (!observations.isEmpty()) {
                error.clear();
                break;
            } else {
                error.clear();
            }
            QThread::msleep(100);
        }
        RfidResult result;
        QSet<QString> uids; for (const auto &item : observations) uids.insert(item.uid);
        if (m_cancelled) { result.error = RfidError::Cancelled; result.message = QStringLiteral("RFID recognition cancelled."); }
        else if (!error.isEmpty()) { result.error = RfidError::NoTag; result.message = error; }
        else if (uids.size() != 1) { result.error = uids.isEmpty() ? RfidError::NoTag : RfidError::DuplicateUid; result.message = uids.isEmpty() ? QStringLiteral("No RFID tag found.") : QStringLiteral("More than one RFID tag found."); }
        else { const auto item = observations.first(); readTagPayload(reinterpret_cast<RFID_READER_HANDLE>(m_reader), item, &result); }
        QMetaObject::invokeMethod(this, [this, result] { emit stateChanged(DeviceState::Ready, QStringLiteral("RFID reader ready.")); emit recognized(result); }, Qt::QueuedConnection);
    });
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater); m_thread->start();
#else
    emit stateChanged(DeviceState::Error, QStringLiteral("RFID hardware backend is unavailable."));
#endif
}

void RealRfidService::cancel()
{
    m_cancelled = true;
    if (m_thread) {
        // The worker owns the reader while inventory is in progress.  Do not
        // close its SDK handle until it has finished using it.
        m_thread->wait();
        m_thread = nullptr;
    }
    emit stateChanged(DeviceState::Ready, QStringLiteral("RFID operation cancelled."));
}

RealCameraService::~RealCameraService() { disconnectDevice(); }

void RealCameraService::connectDevice()
{
#ifdef TLS401_HAS_IDS_PEAK
    if (m_context) return;
    try {
        peak::Library::Initialize();
        auto *context = new CameraContext;
        auto &manager = peak::DeviceManager::Instance();
        manager.Update();
        if (manager.Devices().empty()) throw std::runtime_error("No IDS camera found.");
        auto descriptor = manager.Devices().at(0);
        logCcd(QStringLiteral("Connecting camera model=%1 serial=%2.")
                   .arg(QString::fromStdString(descriptor->ModelName()), QString::fromStdString(descriptor->SerialNumber())));
        context->device = descriptor->OpenDevice(peak::core::DeviceAccessType::Control);
        context->nodeMap = context->device->RemoteDevice()->NodeMaps().at(0);
        context->stream = context->device->DataStreams().at(0)->OpenDataStream();
        m_exposure = context->nodeMap->FindNode<peak::core::nodes::FloatNode>("ExposureTime")->Value();
        m_gain = context->nodeMap->FindNode<peak::core::nodes::FloatNode>("Gain")->Value();
        m_context = context;
        m_connected = true;
        emit stateChanged(DeviceState::Ready, QStringLiteral("IDS camera connected."));
    } catch (const std::exception &e) {
        emit stateChanged(DeviceState::Error, QString::fromLocal8Bit(e.what()));
        try { peak::Library::Close(); } catch (...) {}
    }
#else
    emit stateChanged(DeviceState::Error, QStringLiteral("IDS Peak SDK is not configured."));
#endif
}

void RealCameraService::disconnectDevice()
{
#ifdef TLS401_HAS_IDS_PEAK
    m_running = false;
    auto *context = static_cast<CameraContext *>(m_context);
    if (context && context->stream) {
        try { context->stream->KillWait(); } catch (...) {}
    }
    if (m_thread) {
        // KillWait unblocks WaitForFinishedBuffer.  Waiting here prevents the
        // thread from touching the stream after its owning context is released.
        m_thread->wait();
        m_thread = nullptr;
    }
    if (context) { delete context; m_context = nullptr; }
    if (m_connected) { try { peak::Library::Close(); } catch (...) {} }
    m_connected = false;
    emit stateChanged(DeviceState::Offline, QStringLiteral("IDS camera disconnected."));
#else
    m_connected = false; emit stateChanged(DeviceState::Offline, QStringLiteral("Camera disconnected."));
#endif
}

void RealCameraService::startPreview()
{
#ifdef TLS401_HAS_IDS_PEAK
    auto *context = static_cast<CameraContext *>(m_context); if (!context || !context->stream) { emit stateChanged(DeviceState::Error, QStringLiteral("Camera is not connected.")); return; } if (m_thread && m_thread->isRunning()) return;
    CameraFrameConfig config;
    try {
        config = configureCameraFrame(context->nodeMap);
        logCcd(QStringLiteral("Preview configured: mode=%1 size=%2x%3 payload=%4 bytes buffers=5.")
                   .arg(config.mode).arg(config.width).arg(config.height).arg(config.payload));
    } catch (const std::exception &e) {
        const QString message = QString::fromLocal8Bit(e.what());
        logCcd(QStringLiteral("Preview configuration failed: %1").arg(message));
        emit stateChanged(DeviceState::Error, message);
        return;
    }
    m_running = true;
    emit stateChanged(DeviceState::Busy, QStringLiteral("Camera preview started (%1, %2x%3).").arg(config.mode).arg(config.width).arg(config.height));
    m_thread = QThread::create([this, context, config] {
        try {
            QElapsedTimer previewTimer;
            previewTimer.start();
            QElapsedTimer firstFrameTimer;
            firstFrameTimer.start();
            bool firstFrameReceived = false;
            // Keep the UI preview small and rate-limited.  Sending a complete
            // CCD frame through the queued signal on every acquisition quickly
            // exhausts memory when the display thread cannot keep up.
            qint64 lastPreviewMs = -100;
            // Some IDS transports do not deliver their first frame reliably
            // with only the reported minimum queued buffers.  Keep the
            // established five-buffer acquisition queue; preview copies are
            // rate-limited below, which is where the former memory growth was.
            const size_t bufferCount = qMax<size_t>(5, context->stream->NumBuffersAnnouncedMinRequired());
            for (size_t i = 0; i < bufferCount; ++i) context->stream->AllocAndAnnounceBuffer(static_cast<size_t>(config.payload), nullptr);
            for (const auto &buffer : context->stream->AnnouncedBuffers()) context->stream->QueueBuffer(buffer);
            context->stream->StartAcquisition();
            context->nodeMap->FindNode<peak::core::nodes::CommandNode>("AcquisitionStart")->Execute();
            while (m_running) {
                try {
                    auto buffer = context->stream->WaitForFinishedBuffer(1000);
                    if (!firstFrameReceived) {
                        firstFrameReceived = true;
                        logCcd(QStringLiteral("First frame received after %1 ms: size=%2x%3 pixelFormat=%4.")
                                   .arg(firstFrameTimer.elapsed()).arg(buffer->Width()).arg(buffer->Height()).arg(buffer->PixelFormat()));
                    }
                    const qint64 elapsed = previewTimer.elapsed();
                    if (elapsed - lastPreviewMs < 100) {
                        context->stream->QueueBuffer(buffer);
                        continue;
                    }
                    lastPreviewMs = elapsed;
                    peak::ipl::Image raw(peak::ipl::PixelFormat(static_cast<peak::ipl::PixelFormatName>(buffer->PixelFormat())), static_cast<uint8_t *>(buffer->BasePtr()), static_cast<size_t>(buffer->Size()), static_cast<size_t>(buffer->Width()), static_cast<size_t>(buffer->Height()));
                    auto rgb = raw.ConvertTo(peak::ipl::PixelFormat(peak::ipl::PixelFormatName::RGB8));
                    QImage image(static_cast<uchar *>(rgb.PixelPointer(0,0)), static_cast<int>(rgb.Width()), static_cast<int>(rgb.Height()), static_cast<int>(rgb.Width()) * 3, QImage::Format_RGB888);
                    context->stream->QueueBuffer(buffer);
                    { QMutexLocker lock(&context->mutex); context->latest = image.copy(); }
                    const QImage preview = image.scaled(QSize(960, 720), Qt::KeepAspectRatio, Qt::FastTransformation);
                    QMetaObject::invokeMethod(this, [this, preview] { emit previewFrame(preview); }, Qt::QueuedConnection);
                } catch (const peak::core::TimeoutException &) {
                    if (!firstFrameReceived && firstFrameTimer.elapsed() >= 5000) {
                        m_running = false;
                        logCcd(QStringLiteral("No CCD frame received within 5 seconds."));
                        QMetaObject::invokeMethod(this, [this] { emit stateChanged(DeviceState::Error, QStringLiteral("No CCD frame was received within 5 seconds. Check the camera connection and trigger mode.")); }, Qt::QueuedConnection);
                        break;
                    }
                    continue;
                }
                catch (const peak::core::AbortedException &) { break; }
            }
        } catch (const std::bad_alloc &) {
            m_running = false;
            logCcd(QStringLiteral("Preview stopped: insufficient memory after output configuration."));
            QMetaObject::invokeMethod(this, [this] { emit stateChanged(DeviceState::Error, QStringLiteral("CCD preview stopped because there is not enough memory for the current camera resolution.")); }, Qt::QueuedConnection);
        } catch (const std::exception &e) {
            m_running = false;
            const QString message = QString::fromLocal8Bit(e.what());
            logCcd(QStringLiteral("Preview failed: %1").arg(message));
            QMetaObject::invokeMethod(this, [this, message] { emit stateChanged(DeviceState::Error, message); }, Qt::QueuedConnection);
        }
        try { context->nodeMap->FindNode<peak::core::nodes::CommandNode>("AcquisitionStop")->Execute(); } catch (...) {}
        try {
            context->stream->KillWait();
            context->stream->StopAcquisition();
            context->stream->Flush(peak::core::DataStreamFlushMode::DiscardAll);
            for (const auto &buffer : context->stream->AnnouncedBuffers()) context->stream->RevokeBuffer(buffer);
        } catch (...) {}
    }); connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater); m_thread->start();
#else
    emit stateChanged(DeviceState::Error, QStringLiteral("IDS Peak SDK is not configured."));
#endif
}

void RealCameraService::stopPreview() { m_running = false; emit stateChanged(m_connected ? DeviceState::Ready : DeviceState::Offline, QStringLiteral("Camera preview stopped.")); }
void RealCameraService::setExposure(double value)
{
#ifdef TLS401_HAS_IDS_PEAK
    auto *context = static_cast<CameraContext *>(m_context);
    if (!context || !context->nodeMap) { emit stateChanged(DeviceState::Error, QStringLiteral("Camera is not connected.")); return; }
    try {
        context->nodeMap->FindNode<peak::core::nodes::EnumerationNode>("ExposureAuto")->SetCurrentEntry("Off");
        const auto node = context->nodeMap->FindNode<peak::core::nodes::FloatNode>("ExposureTime");
        m_exposure = qBound(node->Minimum(), value, node->Maximum());
        node->SetValue(m_exposure);
    } catch (const std::exception &e) { emit stateChanged(DeviceState::Error, QString::fromLocal8Bit(e.what())); }
#else
    Q_UNUSED(value)
    emit stateChanged(DeviceState::Error, QStringLiteral("IDS Peak SDK is not configured."));
#endif
}

void RealCameraService::setGain(double value)
{
#ifdef TLS401_HAS_IDS_PEAK
    auto *context = static_cast<CameraContext *>(m_context);
    if (!context || !context->nodeMap) { emit stateChanged(DeviceState::Error, QStringLiteral("Camera is not connected.")); return; }
    try {
        context->nodeMap->FindNode<peak::core::nodes::EnumerationNode>("GainAuto")->SetCurrentEntry("Off");
        const auto node = context->nodeMap->FindNode<peak::core::nodes::FloatNode>("Gain");
        m_gain = qBound(node->Minimum(), value, node->Maximum());
        node->SetValue(m_gain);
    } catch (const std::exception &e) { emit stateChanged(DeviceState::Error, QString::fromLocal8Bit(e.what())); }
#else
    Q_UNUSED(value)
    emit stateChanged(DeviceState::Error, QStringLiteral("IDS Peak SDK is not configured."));
#endif
}
void RealCameraService::capture()
{
#ifdef TLS401_HAS_IDS_PEAK
    auto *context = static_cast<CameraContext *>(m_context); if (!context) { emit captureFailed(QStringLiteral("Camera is not connected.")); return; } QImage image; { QMutexLocker lock(&context->mutex); image = context->latest.copy(); } if (image.isNull()) emit captureFailed(QStringLiteral("No complete camera frame is available.")); else emit captured(image);
#else
    emit captureFailed(QStringLiteral("IDS Peak SDK is not configured."));
#endif
}
