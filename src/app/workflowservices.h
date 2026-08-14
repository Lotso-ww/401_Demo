#pragma once

#include "domain/models.h"
#include <QObject>

class Repository;
class ImageFileStore;

class ChamberSessionService : public QObject {
    Q_OBJECT
public:
    explicit ChamberSessionService(QObject *parent = nullptr);
    void setRepository(Repository *repository) { m_repository = repository; }
    void restoreProfiles(const QVector<ChamberModel> &profiles);
    const QVector<ChamberModel> &chambers() const { return m_chambers; }
    int selectedChamber() const { return m_selected; }
    void selectChamber(int chamberNo);
    bool bindProfile(const TagProfile &profile, QString *error = nullptr);
    void clearSelected();
    bool clearAll(QString *error = nullptr);
    ChamberModel *selectedModel();
signals:
    void changed();
    void selectionChanged();
private:
    QVector<ChamberModel> m_chambers;
    int m_selected = 0;
    Repository *m_repository = nullptr;
};

class CaptureWorkflowService : public QObject {
    Q_OBJECT
public:
    struct PendingCapture {
        TagProfile profile;
        int chamberNo = 0;
        int roundNo = 0;
        int wellNo = 0;
        qint64 roundId = 0;
        bool retake = false;
        QDateTime capturedAt;
        double exposureUs = 0;
        double gainDb = 0;
        QImage image;
    };
    explicit CaptureWorkflowService(ChamberSessionService *sessions, QObject *parent = nullptr);
    void setPersistence(Repository *repository, ImageFileStore *imageStore) { m_repository = repository; m_imageStore = imageStore; }
    bool createRound(QString *error = nullptr);
    bool finishRound();
    void discardActiveRound();
    bool selectWell(int wellNo);
    bool prepareCapture(const QImage &image, bool retake, double exposureUs, double gainDb,
                        PendingCapture *pending, QString *error = nullptr);
    bool commitCapture(const PendingCapture &pending, QString *error = nullptr);
    bool acceptCapture(const QImage &image, bool retake, double exposureUs = 0, double gainDb = 0, QString *error = nullptr);
    bool acceptCapture(const QImage &image, bool retake, QString *error);
    int currentWell() const { return m_currentWell; }
    bool hasActiveRound() const;
    QVector<WellState> wellStates() const;
    QVector<WellCapture> historyForWell(int wellNo) const;
signals:
    void changed();
    void roundCompleted();
private:
    CaptureRound *activeRound();
    void advanceToNext();
    void selectFirstPendingWell();
    ChamberSessionService *m_sessions;
    Repository *m_repository = nullptr;
    ImageFileStore *m_imageStore = nullptr;
    int m_currentWell = 1;
    qint64 m_activeRoundId = 0;
    bool persistCompletedRound(CaptureRound *round, const TagProfile &profile, int chamberNo, QString *error);
};
