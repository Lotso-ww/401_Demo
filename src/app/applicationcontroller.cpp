#include "applicationcontroller.h"
#include "devices/realdevices.h"
#include "storage/database.h"
#include "storage/repository.h"
#include "storage/imagefilestore.h"

#include <QDir>
#include <QStandardPaths>
#include <QTimer>

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
        QString loadError;
        QVector<ChamberModel> persisted = m_sessions.chambers();
        if (m_repository->loadAssignments(&persisted, &loadError) && m_repository->loadRounds(&persisted, &loadError))
            m_sessions.restoreProfiles(persisted);
        else
            emit message(QString::fromUtf8("SQLite recovery failed: %1").arg(loadError), true);
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
        emit message(QString::fromUtf8("\xE6\x8B\x8D\xE7\x85\xA7\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x9A%1").arg(reason), true);
        if (m_sequenceMode == SequenceMode::Capturing)
            finishSequence();
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
            identifyNext();
            return;
        }
        if (!result.ok()) {
            emit message(result.message, true);
            return;
        }
        QString error;
        if (!m_sessions.bindProfile(result.profile, &error))
            emit message(error, true);
        else
            emit message(result.message, false);
    });
    connect(&m_workflow, &CaptureWorkflowService::roundCompleted, this, [this] {
        emit message(QString::fromUtf8("16 \xE5\xAD\x94\xE5\xB7\xB2\xE5\xAE\x8C\xE6\x88\x90\xEF\xBC\x8C\xE6\x9C\xAC\xE8\xBD\xAE\xE9\x87\x87\xE9\x9B\x86\xE5\xB7\xB2\xE7\xBB\x93\xE6\x9D\x9F"), false);
    });

    m_rfid->initialize();
    m_camera->connectDevice();
}

ApplicationController::~ApplicationController()
{
    if (m_camera)
        m_camera->disconnectDevice();
    if (m_rfid)
        m_rfid->cancel();
    delete m_imageStore;
    delete m_repository;
    delete m_database;
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
        finishSequence(QString::fromUtf8("\xE5\x9B\x9B\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE8\xAF\x86\xE5\x88\xAB\xE5\xAE\x8C\xE6\x88\x90"));
        return;
    }
    m_sessions.selectChamber(m_sequenceChambers.at(m_sequenceIndex));
    m_rfid->recognize();
}

void ApplicationController::startCaptureSequence()
{
    if (sequenceActive()) {
        emit message(QString::fromUtf8("\xE8\xAF\x86\xE5\x88\xAB\xE6\x88\x96\xE6\x8B\x8D\xE7\x85\xA7\xE9\x98\x9F\xE5\x88\x97\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xBF\x90\xE8\xA1\x8C"), true);
        return;
    }
    for (int chamber : {4, 3, 2, 1}) {
        if (!m_sessions.chambers().at(chamber - 1).profile) {
            emit message(QString::fromUtf8("\xE8\xAF\xB7\xE5\x85\x88\xE5\xAE\x8C\xE6\x88\x90\xE5\x9B\x9B\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE8\xAF\x86\xE5\x88\xAB"), true);
            return;
        }
    }
    m_sequenceChambers = {4, 3, 2, 1};
    m_sequenceIndex = 0;
    m_sequenceMode = SequenceMode::Capturing;
    m_sequenceStatus = QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE6\x8C\x89 4 \xE2\x86\x92 3 \xE2\x86\x92 2 \xE2\x86\x92 1 \xE6\x8B\x8D\xE7\x85\xA7");
    emit stateChanged();
    captureNext();
}

void ApplicationController::captureNext()
{
    if (m_sequenceMode != SequenceMode::Capturing)
        return;
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
    m_camera->capture();
}

void ApplicationController::captureCurrent(bool retake)
{
    if (m_sequenceMode != SequenceMode::None) {
        emit message(QString::fromUtf8("\xE8\xAF\xB7\xE7\xAD\x89\xE5\xBE\x85\xE8\x87\xAA\xE5\x8A\xA8\xE6\x8B\x8D\xE7\x85\xA7\xE9\x98\x9F\xE5\x88\x97\xE5\xAE\x8C\xE6\x88\x90"), true);
        return;
    }
    m_captureRetake = retake;
    m_camera->capture();
}

void ApplicationController::handleCapturedFrame(const QImage &image)
{
    const int chamberBeforeCapture = m_sessions.selectedChamber();
    QString error;
    if (!m_workflow.acceptCapture(image, m_captureRetake, m_camera->exposure(), m_camera->gain(), &error)) {
        emit message(error, true);
        if (m_sequenceMode == SequenceMode::Capturing)
            finishSequence();
        return;
    }
    m_captureRetake = false;
    if (m_sequenceMode == SequenceMode::Capturing && m_sessions.selectedChamber() == chamberBeforeCapture) {
        ++m_sequenceIndex;
        bool allComplete = true;
        for (int chamber : m_sequenceChambers) {
            const auto &model = m_sessions.chambers().at(chamber - 1);
            if (model.rounds.isEmpty() || !model.rounds.last().finished) {
                allComplete = false;
                break;
            }
        }
        if (allComplete) {
            finishSequence(QString::fromUtf8("\xE5\x9B\x9B\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE6\x8B\x8D\xE7\x85\xA7\xE5\xAE\x8C\xE6\x88\x90"));
            return;
        }
        if (!m_workflow.hasActiveRound() || m_sequenceMode == SequenceMode::Capturing) {
            QTimer::singleShot(80, this, [this] { captureNext(); });
        }
    }
}

void ApplicationController::finishSequence(const QString &detail)
{
    m_sequenceMode = SequenceMode::None;
    m_sequenceStatus.clear();
    emit stateChanged();
    if (!detail.isEmpty())
        emit message(detail, false);
}
