#pragma once

#include "domain/models.h"
#include <QObject>

class IRfidService : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void initialize() = 0;
    virtual void recognize() = 0;
    virtual void cancel() = 0;
signals:
    void stateChanged(DeviceState state, const QString &message);
    void recognized(const RfidResult &result);
};

class ICameraService : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual void startPreview() = 0;
    virtual void stopPreview() = 0;
    virtual void setExposure(double microseconds) = 0;
    virtual void setGain(double db) = 0;
    virtual double exposure() const = 0;
    virtual double gain() const = 0;
    virtual void capture() = 0;
signals:
    void stateChanged(DeviceState state, const QString &message);
    void previewFrame(const QImage &image);
    void captured(const QImage &image);
    void captureFailed(const QString &reason);
};
