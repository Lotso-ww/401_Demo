#pragma once

#include "deviceinterfaces.h"
#include <QThread>
#include <QPointer>
#include <atomic>

// The vendor SDKs are intentionally hidden behind these services.  The classes
// remain constructible on development machines without hardware; they expose a
// deterministic error instead of silently falling back to a mock device.
class RealRfidService final : public IRfidService {
    Q_OBJECT
public:
    using IRfidService::IRfidService;
    ~RealRfidService() override;
    void initialize() override;
    void recognize() override;
    void cancel() override;
private:
    QPointer<QThread> m_thread;
    void *m_reader = nullptr;
    std::atomic_bool m_cancelled{false};
    std::atomic_bool m_ready{false};
};

class RealCameraService final : public ICameraService {
    Q_OBJECT
public:
    using ICameraService::ICameraService;
    ~RealCameraService() override;
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
    void *m_context = nullptr;
    QPointer<QThread> m_thread;
    std::atomic_bool m_running{false};
    bool m_connected = false;
    double m_exposure = 0;
    double m_gain = 0;
};
