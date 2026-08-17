#include "applicationcontroller.h"
#include "devices/realdevices.h"
#include "storage/database.h"
#include "storage/repository.h"
#include "storage/imagefilestore.h"

#include <algorithm>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

namespace {
bool isUsableCapture(const QImage &image)
{
    // An empty well can legitimately produce a dark or nearly uniform frame.
    // Brightness/variance are not indicators of transport failure and used to
    // abort an entire automatic sequence for valid empty-well images.
    return !image.isNull() && image.width() >= 32 && image.height() >= 32;
}
}

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent), m_sessions(this), m_workflow(&m_sessions, this), m_rfid(nullptr), m_camera(nullptr)
{
    m_rfid = new RealRfidService(this);
    m_camera = new RealCameraService(this);

    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(root);
    m_database = new Database;
    QString dbError;
    if (m_database->open(QDir(root).filePath(QString::fromUtf8("tls401.sqlite")), &dbError)) {
        m_databaseReady = true;
        m_repository = new Repository(m_database->connection());
        m_imageStore = new ImageFileStore(root);
        m_sessions.setRepository(m_repository);
        m_workflow.setPersistence(m_repository, m_imageStore);
    } else {
        emit message(QString::fromUtf8("SQLite initialization failed: %1").arg(dbError), true);
    }

    connect(m_rfid, &IRfidService::stateChanged, this, [this](DeviceState state, const QString &detail) {
        m_rfidState = state;
        emit stateChanged();
        if (state == DeviceState::Error && !detail.isEmpty())
            emit message(QString::fromUtf8("RFID\xEF\xBC\x9A%1").arg(detail), true);
    });
    connect(m_camera, &ICameraService::stateChanged, this, [this](DeviceState state, const QString &detail) {
        m_cameraState = state;
        emit stateChanged();
        if (state == DeviceState::Error && !detail.isEmpty())
            emit message(QString::fromUtf8("CCD\xEF\xBC\x9A%1").arg(detail), true);
    });
    connect(m_camera, &ICameraService::captureFailed, this, [this](const QString &reason) {
        if (!m_captureRequestPending)
            return;
        const bool discardResult = m_discardPendingCaptureResult;
        m_captureRequestPending = false;
        m_captureRequestDispatched = false;
        m_discardPendingCaptureResult = false;
        if (discardResult) {
            if (m_sequenceMode == SequenceMode::Capturing && !m_capturePaused)
                QTimer::singleShot(0, this, [this] { captureNext(); });
            return;
        }
        emit message(QString::fromUtf8("\xE6\x8B\x8D\xE7\x85\xA7\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x9A%1").arg(reason), true);
        if (m_sequenceMode == SequenceMode::Capturing)
            finishSequence();
    });
    connect(m_camera, &ICameraService::previewFrame, this, [this](const QImage &) {
        m_previewFrameAvailable = true;
        if (m_sequenceMode == SequenceMode::Capturing && !m_capturePaused)
            captureNext();
    });
    connect(m_camera, &ICameraService::captured, this, [this](const QImage &image) { handleCapturedFrame(image); });
    connect(m_rfid, &IRfidService::recognized, this, [this](const RfidResult &result) {
        if (m_sequenceMode == SequenceMode::Identifying) {
            const int chamber = m_sequenceChambers.value(m_sequenceIndex);
            if (!result.ok()) {
                emit message(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1\xE8\xAF\x86\xE5\x88\xAB\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x9A%2").arg(chamber).arg(result.message), true);
            } else {
                QString error;
                if (!m_sessions.bindProfile(result.profile, &error))
                    emit message(error, true);
            }
            ++m_sequenceIndex;
            QTimer::singleShot(600, this, [this] { identifyNext(); });
            return;
        }
        if (!result.ok()) {
            emit message(result.message, true);
            return;
        }
        QString error;
        if (!m_sessions.bindProfile(result.profile, &error))
            emit message(error, true);
        else {
            m_captureSequenceAuthorized = true;
            emit stateChanged();
            emit message(result.message, false);
        }
    });
    connect(&m_workflow, &CaptureWorkflowService::roundCompleted, this, [this] {
        emit message(QString::fromUtf8("16 \xE5\xAD\x94\xE5\xB7\xB2\xE5\xAE\x8C\xE6\x88\x90\xEF\xBC\x8C\xE6\x9C\xAC\xE8\xBD\xAE\xE9\x87\x87\xE9\x9B\x86\xE5\xB7\xB2\xE7\xBB\x93\xE6\x9D\x9F"), false);
    });

}

ApplicationController::~ApplicationController()
{
    if (m_camera)
        m_camera->disconnectDevice();
    if (m_rfid)
        m_rfid->cancel();
    if (m_saveThread) {
        m_saveThread->wait();
        m_saveThread = nullptr;
    }
    delete m_imageStore;
    delete m_repository;
    delete m_database;
}

void ApplicationController::initializeDevices()
{
    if (m_devicesInitialized)
        return;
    m_devicesInitialized = true;
    m_rfid->initialize();
    m_camera->connectDevice();
}

void ApplicationController::identify()
{
    if (!m_sessions.selectedChamber()) {
        emit message(QString::fromUtf8("\xE8\xAF\xB7\xE5\x85\x88\xE9\x80\x89\xE6\x8B\xA9\xE8\x88\xB1\xE5\xAE\xA4"), true);
        return;
    }
    m_rfid->recognize();
}

void ApplicationController::identifyAll()
{
    if (sequenceActive()) {
        emit message(QString::fromUtf8("\xE8\xAF\x86\xE5\x88\xAB\xE6\x88\x96\xE6\x8B\x8D\xE7\x85\xA7\xE9\x98\x9F\xE5\x88\x97\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xBF\x90\xE8\xA1\x8C"), true);
        return;
    }
    QString clearError;
    if (!m_sessions.clearAll(&clearError)) {
        emit message(clearError, true);
        return;
    }
    m_captureSequenceAuthorized = false;
    m_completedCaptureChambers.clear();
    m_sequenceChambers = {4, 3, 2, 1};
    m_sequenceIndex = 0;

    m_sequenceMode = SequenceMode::Identifying;
    m_sequenceStatus = QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE6\x8C\x89 4 \xE2\x86\x92 3 \xE2\x86\x92 2 \xE2\x86\x92 1 \xE8\xAF\x86\xE5\x88\xAB\xE8\x88\xB1\xE5\xAE\xA4");
    emit stateChanged();
    identifyNext();
}

void ApplicationController::identifyNext()
{
    if (m_sequenceMode != SequenceMode::Identifying)
        return;
    if (m_sequenceIndex >= m_sequenceChambers.size()) {
        m_captureSequenceAuthorized = std::any_of(m_sessions.chambers().cbegin(), m_sessions.chambers().cend(),
                                                  [](const ChamberModel &chamber) { return chamber.profile.has_value(); });
        finishSequence(QString::fromUtf8("\xE5\x9B\x9B\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE8\xAF\x86\xE5\x88\xAB\xE5\xAE\x8C\xE6\x88\x90"));
        return;
    }
    const int chamber = m_sequenceChambers.at(m_sequenceIndex);
    m_sessions.selectChamber(chamber);
    m_sequenceStatus = QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xAF\x86\xE5\x88\xAB\xEF\xBC\x9A%1\xE5\x8F\xB7\xE8\x88\xB1\xEF\xBC\x8C\xE8\xAF\xB7\xE6\x94\xBE\xE7\xBD\xAE RFID \xE6\xA0\x87\xE7\xAD\xBE").arg(chamber);
    emit stateChanged();
    m_rfid->recognize();
}

void ApplicationController::startCaptureSequence()
{
    if (sequenceActive()) {
        emit message(QString::fromUtf8("\xE8\xAF\x86\xE5\x88\xAB\xE6\x88\x96\xE6\x8B\x8D\xE7\x85\xA7\xE9\x98\x9F\xE5\x88\x97\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xBF\x90\xE8\xA1\x8C"), true);
        return;
    }
    if (!m_captureSequenceAuthorized) {
        emit message(QString::fromUtf8("请先完成本轮 RFID 识别后再开始拍照"), true);
        return;
    }
    m_sequenceChambers.clear();
    for (int chamber : {4, 3, 2, 1})
        if (m_sessions.chambers().at(chamber - 1).profile)
            m_sequenceChambers.push_back(chamber);
    if (m_sequenceChambers.isEmpty()) {
        emit message(QStringLiteral("Please identify at least one chamber before starting capture."), true);
        return;
    }
    m_sequenceIndex = 0;
    m_sequenceMode = SequenceMode::Capturing;
    m_completedCaptureChambers.clear();
    m_captureSequenceAuthorized = false;
    m_capturePaused = false;
    m_previewFrameAvailable = false;
    m_captureRequestPending = false;
    m_sequenceStatus = QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE6\x8C\x89 4 \xE2\x86\x92 3 \xE2\x86\x92 2 \xE2\x86\x92 1 \xE6\x8B\x8D\xE7\x85\xA7");
    emit stateChanged();
    m_camera->startPreview();
    captureNext();
}

void ApplicationController::captureNext()
{
    if (m_sequenceMode != SequenceMode::Capturing || m_capturePaused || m_captureRequestPending)
        return;
    if (!m_previewFrameAvailable) {
        m_sequenceStatus = QString::fromUtf8("等待 CCD 实时画面后开始拍照");
        emit stateChanged();
        return;
    }
    if (m_sequenceChambers.isEmpty()) {
        finishSequence(QString::fromUtf8("\xE5\x9B\x9B\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE6\x8B\x8D\xE7\x85\xA7\xE5\xAE\x8C\xE6\x88\x90"));
        return;
    }
    const int chamberIndex = m_sequenceIndex % m_sequenceChambers.size();
    const int chamber = m_sequenceChambers.at(chamberIndex);
    m_sessions.selectChamber(chamber);
    if (!m_workflow.hasActiveRound()) {
        QString error;
        if (!m_workflow.createRound(&error)) {
            emit message(error, true);
            finishSequence();
            return;
        }
    }
    m_sequenceStatus = QString::fromUtf8("\xE6\x8B\x8D\xE7\x85\xA7\xE4\xB8\xAD\xEF\xBC\x9A%1 \xE5\x8F\xB7\xE8\x88\xB1 / %2 \xE5\x8F\xB7\xE5\xAD\x94").arg(chamber).arg(m_workflow.currentWell());
    emit stateChanged();
    m_captureRequestPending = true;
    m_captureRequestDispatched = false;
    m_discardPendingCaptureResult = false;
    const quint64 requestToken = ++m_captureRequestToken;
    // Let the current-well state reach the event loop before the synchronous
    // capture/save path starts, so the operator can see the green marker move.
    QTimer::singleShot(150, this, [this, requestToken] {
        if (m_sequenceMode == SequenceMode::Capturing && !m_capturePaused
            && m_captureRequestPending && !m_captureRequestDispatched
            && requestToken == m_captureRequestToken) {
            m_captureRequestDispatched = true;
            m_camera->capture();
        }
    });
}

void ApplicationController::captureCurrent(bool retake)
{
    if (m_sequenceMode != SequenceMode::None) {
        emit message(QString::fromUtf8("\xE8\xAF\xB7\xE7\xAD\x89\xE5\xBE\x85\xE8\x87\xAA\xE5\x8A\xA8\xE6\x8B\x8D\xE7\x85\xA7\xE9\x98\x9F\xE5\x88\x97\xE5\xAE\x8C\xE6\x88\x90"), true);
        return;
    }
    if (m_captureRequestPending) {
        emit message(QStringLiteral("The previous image is still being saved."), true);
        return;
    }
    m_captureRetake = retake;
    m_captureRequestPending = true;
    m_captureRequestDispatched = true;
    m_discardPendingCaptureResult = false;
    ++m_captureRequestToken;
    m_camera->capture();
}

void ApplicationController::handleCapturedFrame(const QImage &image)
{
    if (!m_captureRequestPending)
        return;
    if (m_discardPendingCaptureResult) {
        m_captureRequestPending = false;
        m_captureRequestDispatched = false;
        m_discardPendingCaptureResult = false;
        if (m_sequenceMode == SequenceMode::Capturing && !m_capturePaused)
            QTimer::singleShot(0, this, [this] { captureNext(); });
        return;
    }
    m_captureRequestDispatched = false;
    const int chamberBeforeCapture = m_sessions.selectedChamber();
    if (!isUsableCapture(image)) {
        m_captureRequestPending = false;
        emit message(QString::fromUtf8("CCD\xE5\x9B\xBE\xE5\x83\x8F\xE5\xBC\x82\xE5\xB8\xB8\xEF\xBC\x9A\xE6\x94\xB6\xE5\x88\xB0\xE7\x9A\x84\xE7\x94\xBB\xE9\x9D\xA2\xE8\xBF\x87\xE6\x9A\x97\xE6\x88\x96\xE6\x97\xA0\xE6\x9C\x89\xE6\x95\x88\xE7\xBB\x86\x8A\x82"), true);
        if (m_sequenceMode == SequenceMode::Capturing)
            finishSequence();
        return;
    }
    QString error;
    CaptureWorkflowService::PendingCapture pending;
    if (!m_workflow.prepareCapture(image, m_captureRetake, m_camera->exposure(), m_camera->gain(), &pending, &error)) {
        m_captureRequestPending = false;
        emit message(error, true);
        if (m_sequenceMode == SequenceMode::Capturing)
            finishSequence();
        return;
    }
    m_captureRequestPending = false;
    QString commitError;
    if (!m_workflow.commitCapture(pending, &commitError)) {
        emit message(commitError, true);
        if (m_sequenceMode == SequenceMode::Capturing)
            finishSequence();
        return;
    }
    m_captureRetake = false;
    if (m_sequenceMode == SequenceMode::Capturing && m_sessions.selectedChamber() == chamberBeforeCapture) {
        const auto *model = m_sessions.selectedModel();
        const bool chamberCompleted = model && !model->rounds.isEmpty() && model->rounds.last().finished;
        if (chamberCompleted) {
            m_completedCaptureChambers.insert(chamberBeforeCapture);
            ++m_sequenceIndex;
        }
        if (m_completedCaptureChambers.size() == m_sequenceChambers.size()) {
            finishSequence(QString::fromUtf8("\xE5\x9B\x9B\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE6\x8B\x8D\xE7\x85\xA7\xE5\xAE\x8C\xE6\x88\x90"));
            return;
        }
        QTimer::singleShot(80, this, [this] { captureNext(); });
    }
}

void ApplicationController::finishSequence(const QString &detail)
{
    const SequenceMode completedMode = m_sequenceMode;
    if (completedMode == SequenceMode::Capturing && m_workflow.hasActiveRound())
        m_workflow.discardActiveRound();
    if (completedMode == SequenceMode::Capturing)
        m_captureSequenceAuthorized = false;
    m_sequenceMode = SequenceMode::None;
    m_capturePaused = false;
    m_captureRequestPending = false;
    m_captureRequestDispatched = false;
    m_discardPendingCaptureResult = false;
    ++m_captureRequestToken;
    m_sequenceStatus.clear();
    emit stateChanged();
    if (!detail.isEmpty())
        emit message(detail, false);
}

void ApplicationController::setCaptureSequencePaused(bool paused)
{
    if (m_sequenceMode != SequenceMode::Capturing)
        return;
    if (m_capturePaused == paused)
        return;
    m_capturePaused = paused;
    // A delayed request can be invalidated immediately. A request already sent
    // to the camera must wait for its terminal signal, then discard that frame.
    if (m_capturePaused && m_captureRequestPending) {
        if (m_captureRequestDispatched) {
            m_discardPendingCaptureResult = true;
        } else {
            m_captureRequestPending = false;
            ++m_captureRequestToken;
        }
    }
    m_sequenceStatus = paused
        ? QString::fromUtf8("拍照已暂停：当前界面不写入新图片")
        : QString::fromUtf8("拍照已恢复：从上次成功拍照后的下一个孔继续");
    emit stateChanged();
    if (!m_capturePaused)
        QTimer::singleShot(0, this, [this] { captureNext(); });
}
