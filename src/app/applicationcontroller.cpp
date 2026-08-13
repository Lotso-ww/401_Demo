#include "applicationcontroller.h"
#include "devices/mockdevices.h"
#include "devices/realdevices.h"
#include "storage/database.h"
#include "storage/repository.h"
#include "storage/imagefilestore.h"
#include <QDir>
#include <QStandardPaths>
#include <QProcessEnvironment>

ApplicationController::ApplicationController(QObject *parent) : QObject(parent), m_sessions(this), m_workflow(&m_sessions, this), m_rfid(nullptr), m_camera(nullptr) {
    const bool useReal = QProcessEnvironment::systemEnvironment().value(QString::fromUtf8("TLS401_DEVICE_BACKEND")) == QString::fromUtf8("real");
    if (useReal) { m_rfid = new RealRfidService(this); m_camera = new RealCameraService(this); }
    else { m_rfid = new MockRfidService(this); m_camera = new MockCameraService(this); }
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
        QString loadError;
        QVector<ChamberModel> persisted = m_sessions.chambers();
        if (m_repository->loadAssignments(&persisted, &loadError) && m_repository->loadRounds(&persisted, &loadError)) m_sessions.restoreProfiles(persisted);
        else emit message(QString::fromUtf8("SQLite recovery failed: %1").arg(loadError), true);
    } else {
        emit message(QString::fromUtf8("SQLite initialization failed: %1").arg(dbError), true);
    }
    connect(m_rfid, &IRfidService::stateChanged, this, [this](DeviceState state, const QString &message) {
        m_rfidState = state;
        emit stateChanged();
        if (state == DeviceState::Error && !message.isEmpty())
            emit this->message(QString::fromUtf8("RFID\xEF\xBC\x9A%1").arg(message), true);
    });
    connect(m_camera, &ICameraService::stateChanged, this, [this](DeviceState state, const QString &message) {
        m_cameraState = state;
        emit stateChanged();
        if (state == DeviceState::Error && !message.isEmpty())
            emit this->message(QString::fromUtf8("CCD\xEF\xBC\x9A%1").arg(message), true);
    });
    connect(m_camera, &ICameraService::captureFailed, this, [this](const QString &reason) {
        emit message(QString::fromUtf8("\xE6\x8B\x8D\xE7\x85\xA7\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x9A%1").arg(reason), true);
    });
    connect(m_rfid, &IRfidService::recognized, this, [this](const RfidResult &result) { if (!result.ok()) { emit message(result.message, true); return; } QString error; if (!m_sessions.bindProfile(result.profile, &error)) emit message(error, true); else emit message(result.message, false); });
    connect(&m_workflow, &CaptureWorkflowService::roundCompleted, this, [this] { emit message(QString::fromUtf8("16 \xE5\xAD\x94\xE5\xB7\xB2\xE5\xAE\x8C\xE6\x88\x90\xEF\xBC\x8C\xE6\x9C\xAC\xE8\xBD\xAE\xE9\x87\x87\xE9\x9B\x86\xE5\xB7\xB2\xE7\xBB\x93\xE6\x9D\x9F"), false); });
    m_rfid->initialize(); m_camera->connectDevice();
}
ApplicationController::~ApplicationController()
{
    // The device services own SDK handles and worker threads.  Stop them
    // before closing persistent storage used by capture callbacks.
    if (m_camera) m_camera->disconnectDevice();
    if (m_rfid) m_rfid->cancel();
    delete m_imageStore;
    delete m_repository;
    delete m_database;
}
void ApplicationController::identify(RfidScenario scenario) { if (!m_sessions.selectedChamber()) { emit message(QString::fromUtf8("\xE8\xAF\xB7\xE5\x85\x88\xE9\x80\x89\xE6\x8B\xA9\xE8\x88\xB1\xE5\xAE\xA4"), true); return; } m_rfid->recognize(scenario); }
