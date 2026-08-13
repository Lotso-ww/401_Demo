#pragma once

#include "deviceinterfaces.h"
#include <QTimer>

class MockRfidService final : public IRfidService {
    Q_OBJECT
public:
    using IRfidService::IRfidService;
    void initialize() override;
    void recognize(RfidScenario scenario) override;
    void cancel() override;
private:
    bool m_cancelled = false;
};

class MockCameraService final : public ICameraService {
    Q_OBJECT
public:
    explicit MockCameraService(QObject *parent = nullptr);
    void connectDevice() override;
    void disconnectDevice() override;
    void startPreview() override;
    void stopPreview() override;
    void setExposure(double microseconds) override;
    void setGain(double db) override;
    double exposure() const override { return m_exposure; }
    double gain() const override { return m_gain; }
    void capture() override;
private:
    QImage makeFrame() const;
    QTimer m_timer;
    bool m_connected = false;
    double m_exposure = 12000.0;
    double m_gain = 6.0;
    mutable int m_frameNumber = 0;
};
