#pragma once

#include "app/workflowservices.h"
#include "devices/deviceinterfaces.h"

class ApplicationController : public QObject {
    Q_OBJECT
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
    void identify(RfidScenario scenario);
signals:
    void message(const QString &text, bool error);
    void stateChanged();
private:
    ChamberSessionService m_sessions;
    CaptureWorkflowService m_workflow;
    IRfidService *m_rfid;
    ICameraService *m_camera;
    DeviceState m_rfidState = DeviceState::Offline;
    DeviceState m_cameraState = DeviceState::Offline;
    bool m_databaseReady = false;
    class Database *m_database = nullptr;
    class Repository *m_repository = nullptr;
    class ImageFileStore *m_imageStore = nullptr;
};
