#pragma once

#include "domain/models.h"
#include <QSqlDatabase>

class Repository final {
public:
    explicit Repository(QSqlDatabase db = {});
    bool upsertTag(const TagProfile &profile, QString *error = nullptr);
    bool assignChamber(int chamberNo, const TagProfile &profile, const QDateTime &identifiedAt, QString *error = nullptr);
    bool clearChamber(int chamberNo, QString *error = nullptr);
    bool loadAssignments(QVector<ChamberModel> *chambers, QString *error = nullptr) const;
    bool loadRounds(QVector<ChamberModel> *chambers, QString *error = nullptr) const;
    bool loadRoundsForUid(const QString &uid, QVector<CaptureRound> *rounds, QString *error = nullptr) const;
    bool nextRoundNumber(const QString &tagUid, int *roundNo, QString *error = nullptr) const;
    bool createRound(const TagProfile &profile, int chamberNo, int roundNo, qint64 *id, QString *error = nullptr);
    bool finishRound(qint64 id, const QString &status, QString *error = nullptr);
    bool deleteRound(qint64 id, QString *error = nullptr);
    bool insertImage(qint64 roundId, const TagProfile &profile, int wellNo, const QString &path,
                     const QImage &image, const QDateTime &capturedAt, double exposure, double gain,
                     qint64 replacedId, qint64 *id, QString *error = nullptr);
private:
    QSqlDatabase m_db;
};
