#pragma once

#include "app/workflowservices.h"
#include "devices/deviceinterfaces.h"
#include <QPointer>
#include <QSet>
#include <QThread>
#include <QVector>

class ApplicationController : public QObject {
    Q_OBJECT
    enum class SequenceMode { None, Identifying, Capturing };
public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;
    ChamberSessionService *sessions() { return &m_sessions; }
    CaptureWorkflowService *workflow() { return &m_workflow; }
    IRfidService *rfid() { return m_rfid; }
    ICameraService *camera() { return m_camera; }
    DeviceState rfidState() const { return m_rfidState; }
    DeviceState cameraState() const { return m_cameraState; }
    bool databaseReady() const { return m_databaseReady; }
    void initializeDevices();
    void identify();
    void identifyAll();
    void startCaptureSequence();
    bool sequenceActive() const { return m_sequenceMode != SequenceMode::None; }
    bool identifying() const { return m_sequenceMode == SequenceMode::Identifying; }
    bool chamberCompletedInCurrentCapture(int chamberNo) const { return m_completedCaptureChambers.contains(chamberNo); }
    bool canStartCapture() const { return m_captureSequenceAuthorized && !sequenceActive(); }
    QString sequenceStatus() const { return m_sequenceStatus; }
    bool captureSequencePaused() const { return m_capturePaused; }
    void setCaptureSequencePaused(bool paused);
    void captureCurrent(bool retake = false);
signals:
    void message(const QString &text, bool error);
    void stateChanged();
private:
    void identifyNext();
    void captureNext();
    void finishSequence(const QString &message = {});
    void handleCapturedFrame(const QImage &image);
    ChamberSessionService m_sessions;
    CaptureWorkflowService m_workflow;
    IRfidService *m_rfid;
    ICameraService *m_camera;
    DeviceState m_rfidState = DeviceState::Offline;
    DeviceState m_cameraState = DeviceState::Offline;
    bool m_databaseReady = false;
    bool m_devicesInitialized = false;
    class Database *m_database = nullptr;
    class Repository *m_repository = nullptr;
    class ImageFileStore *m_imageStore = nullptr;
    QVector<int> m_sequenceChambers;
    QSet<int> m_completedCaptureChambers;
    int m_sequenceIndex = 0;

    SequenceMode m_sequenceMode = SequenceMode::None;
    bool m_captureRetake = false;
    bool m_capturePaused = false;
    bool m_previewFrameAvailable = false;
    bool m_captureRequestPending = false;
    bool m_captureRequestDispatched = false;
    bool m_discardPendingCaptureResult = false;
    bool m_captureSequenceAuthorized = false;
    quint64 m_captureRequestToken = 0;
    QPointer<QThread> m_saveThread;
    QString m_sequenceStatus;
};
