#include "mockdevices.h"

#include <QPainter>
#include <QTimer>

void MockRfidService::initialize() { emit stateChanged(DeviceState::Ready, QString::fromUtf8("Mock RFID \xE5\xB0\xB1\xE7\xBB\xAA")); }

void MockRfidService::recognize(RfidScenario scenario) {
    m_cancelled = false;
    emit stateChanged(DeviceState::Busy, QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE6\xA8\xA1\xE6\x8B\x9F\xE8\xAF\x86\xE5\x88\xAB"));
    QTimer::singleShot(550, this, [this, scenario] {
        if (m_cancelled) return;
        RfidResult result;
        if (scenario == RfidScenario::NoTag) { result.error = RfidError::NoTag; result.message = QString::fromUtf8("\xE6\x9C\xAA\xE6\xA3\x80\xE6\xB5\x8B\xE5\x88\xB0\xE6\xA0\x87\xE7\xAD\xBE"); }
        else if (scenario == RfidScenario::InvalidPayload) { result.error = RfidError::InvalidPayload; result.message = QString::fromUtf8("\xE6\xA0\x87\xE7\xAD\xBE\xE6\x95\xB0\xE6\x8D\xAE\xE5\xBC\x82\xE5\xB8\xB8"); }
        else if (scenario == RfidScenario::DuplicateUid) { result.error = RfidError::DuplicateUid; result.profile.uid = QString::fromUtf8("E280-401-DEMO-01"); result.message = QString::fromUtf8("\xE6\xA0\x87\xE7\xAD\xBE\xE5\xB7\xB2\xE7\xBB\x91\xE5\xAE\x9A\xE5\x88\xB0\xE5\x85\xB6\xE4\xBB\x96\xE8\x88\xB1\xE5\xAE\xA4"); }
        else {
            result.profile = {QString::fromUtf8("E280-401-DEMO-01"), QString::fromUtf8("DISH-20260812-01"), QDateTime::currentDateTime().addDays(-3), QString::fromUtf8("\xE7\x8E\x8B\xE5\xA5\xB3\xE5\xA3\xAB"), QString::fromUtf8("MR-20260812-01"), QString(), QDateTime::currentDateTime()};
            result.message = QString::fromUtf8("\xE6\xA0\x87\xE7\xAD\xBE\xE8\xAF\x86\xE5\x88\xAB\xE6\x88\x90\xE5\x8A\x9F");
        }
        emit stateChanged(DeviceState::Ready, QString::fromUtf8("Mock RFID \xE5\xB0\xB1\xE7\xBB\xAA"));
        emit recognized(result);
    });
}

void MockRfidService::cancel() { m_cancelled = true; emit stateChanged(DeviceState::Ready, QString::fromUtf8("\xE8\xAF\x86\xE5\x88\xAB\xE5\xB7\xB2\xE5\x8F\x96\xE6\xB6\x88")); }

MockCameraService::MockCameraService(QObject *parent) : ICameraService(parent) {
    m_timer.setInterval(90);
    connect(&m_timer, &QTimer::timeout, this, [this] { emit previewFrame(makeFrame()); });
}
void MockCameraService::connectDevice() { m_connected = true; emit stateChanged(DeviceState::Ready, QString::fromUtf8("Mock CCD \xE5\xB7\xB2\xE8\xBF\x9E\xE6\x8E\xA5")); }
void MockCameraService::disconnectDevice() { stopPreview(); m_connected = false; emit stateChanged(DeviceState::Offline, QString::fromUtf8("Mock CCD \xE6\x9C\xAA\xE8\xBF\x9E\xE6\x8E\xA5")); }
void MockCameraService::startPreview() { if (!m_connected) connectDevice(); m_timer.start(); emit stateChanged(DeviceState::Busy, QString::fromUtf8("\xE5\xAE\x9E\xE6\x97\xB6\xE9\xA2\x84\xE8\xA7\x88\xE4\xB8\xAD")); }
void MockCameraService::stopPreview() { m_timer.stop(); emit stateChanged(m_connected ? DeviceState::Ready : DeviceState::Offline, QString::fromUtf8("\xE9\xA2\x84\xE8\xA7\x88\xE5\xB7\xB2\xE5\x81\x9C\xE6\xAD\xA2")); }
void MockCameraService::setExposure(double microseconds) { m_exposure = microseconds; }
void MockCameraService::setGain(double db) { m_gain = db; }
void MockCameraService::capture() { if (!m_connected) { emit captureFailed(QString::fromUtf8("\xE7\x9B\xB8\xE6\x9C\xBA\xE6\x9C\xAA\xE8\xBF\x9E\xE6\x8E\xA5")); return; } QTimer::singleShot(120, this, [this] { emit captured(makeFrame().copy()); }); }
QImage MockCameraService::makeFrame() const {
    QImage image(800, 520, QImage::Format_RGB32);
    image.fill(QColor(21, 35, 46));
    QPainter p(&image); p.setRenderHint(QPainter::Antialiasing);
    const int phase = m_frameNumber++ % 220;
    p.setBrush(QColor(39, 168, 177)); p.setPen(Qt::NoPen); p.drawEllipse(QPoint(380 + phase - 110, 250), 115, 115);
    p.setBrush(QColor(241, 179, 78, 190)); p.drawEllipse(QPoint(390 - phase / 3, 250), 55, 55);
    p.setPen(QColor(231, 244, 247)); p.setFont(QFont(QString::fromUtf8("Microsoft YaHei"), 14));
    p.drawText(QRect(24, 20, 740, 40), QString::fromUtf8("Mock CCD  |  \xE6\x9B\x9D\xE5\x85\x89 %1 us  |  \xE5\xA2\x9E\xE7\x9B\x8A %2 dB").arg(m_exposure, 0, 'f', 0).arg(m_gain, 0, 'f', 1));
    return image;
}
