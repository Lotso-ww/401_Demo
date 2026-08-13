#include "repository.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDir>
#include <QFileInfo>

namespace { void setError(QString *error, const QString &value) { if (error) *error = value; } }
Repository::Repository(QSqlDatabase db) : m_db(std::move(db)) {}

bool Repository::upsertTag(const TagProfile &p, QString *error)
{
    QSqlQuery q(m_db); q.prepare(QStringLiteral("INSERT INTO tag_profile(uid,format_version,dish_number,insemination_time,female_name,medical_record_no,male_name,first_seen_at,last_seen_at) VALUES(?,?,?,?,?,?,?,?,?) ON CONFLICT(uid) DO UPDATE SET format_version=excluded.format_version,dish_number=excluded.dish_number,insemination_time=excluded.insemination_time,female_name=excluded.female_name,medical_record_no=excluded.medical_record_no,last_seen_at=excluded.last_seen_at"));
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    q.addBindValue(p.uid); q.addBindValue(1); q.addBindValue(p.dishNumber.toInt()); q.addBindValue(p.inseminationTime.toUTC().toString(Qt::ISODateWithMs)); q.addBindValue(p.femaleName); q.addBindValue(p.medicalRecordNumber); q.addBindValue(p.maleName.isEmpty() ? QVariant() : QVariant(p.maleName)); q.addBindValue(p.identifiedAt.isValid() ? p.identifiedAt.toUTC().toString(Qt::ISODateWithMs) : now); q.addBindValue(now);
    if (!q.exec()) { setError(error, q.lastError().text()); return false; } return true;
}

bool Repository::assignChamber(int chamberNo, const TagProfile &p, const QDateTime &identifiedAt, QString *error)
{
    if (chamberNo < 1 || chamberNo > 4 || !upsertTag(p, error)) return false;
    QSqlQuery q(m_db); if (!m_db.transaction()) { setError(error, m_db.lastError().text()); return false; }
    q.prepare(QStringLiteral("SELECT chamber_no FROM chamber_assignment WHERE tag_uid=? AND chamber_no<>?")); q.addBindValue(p.uid); q.addBindValue(chamberNo);
    if (!q.exec()) { m_db.rollback(); setError(error, q.lastError().text()); return false; }
    if (q.next()) { m_db.rollback(); setError(error, QStringLiteral("UID is already bound to chamber %1.").arg(q.value(0).toInt())); return false; }
    q.prepare(QStringLiteral("UPDATE chamber_assignment SET tag_uid=NULL, identified_at=NULL, updated_at=? WHERE tag_uid=?")); q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)); q.addBindValue(p.uid); if (!q.exec()) { m_db.rollback(); setError(error, q.lastError().text()); return false; }
    q.prepare(QStringLiteral("INSERT INTO chamber_assignment(chamber_no,tag_uid,identified_at,updated_at) VALUES(?,?,?,?) ON CONFLICT(chamber_no) DO UPDATE SET tag_uid=excluded.tag_uid,identified_at=excluded.identified_at,updated_at=excluded.updated_at")); const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); q.addBindValue(chamberNo); q.addBindValue(p.uid); q.addBindValue(identifiedAt.toUTC().toString(Qt::ISODateWithMs)); q.addBindValue(now);
    if (!q.exec() || !m_db.commit()) { m_db.rollback(); setError(error, q.lastError().text()); return false; } return true;
}

bool Repository::clearChamber(int chamberNo, QString *error)
{
    QSqlQuery q(m_db); q.prepare(QStringLiteral("UPDATE chamber_assignment SET tag_uid=NULL,identified_at=NULL,updated_at=? WHERE chamber_no=?")); q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)); q.addBindValue(chamberNo); if (!q.exec()) { setError(error, q.lastError().text()); return false; } return true;
}

bool Repository::loadAssignments(QVector<ChamberModel> *chambers, QString *error) const
{
    if (!chambers) return false; QSqlQuery q(m_db); if (!q.exec(QStringLiteral("SELECT chamber_no,tag_uid,identified_at FROM chamber_assignment ORDER BY chamber_no"))) { setError(error,q.lastError().text()); return false; }
    while (q.next()) { const int no=q.value(0).toInt(); if (no<1||no>4) continue; TagProfile p; QSqlQuery t(m_db); t.prepare(QStringLiteral("SELECT uid,dish_number,insemination_time,female_name,medical_record_no,male_name FROM tag_profile WHERE uid=?")); t.addBindValue(q.value(1)); if (t.exec()&&t.next()) { p.uid=t.value(0).toString(); p.dishNumber=QString::number(t.value(1).toInt()); p.inseminationTime=QDateTime::fromString(t.value(2).toString(),Qt::ISODate); p.femaleName=t.value(3).toString(); p.medicalRecordNumber=t.value(4).toString(); p.maleName=t.value(5).toString(); p.identifiedAt=QDateTime::fromString(q.value(2).toString(),Qt::ISODate); (*chambers)[no-1].profile=p; } }
    return true;
}

bool Repository::loadRounds(QVector<ChamberModel> *chambers, QString *error) const
{
    if (!chambers) return false;
    QSqlQuery rounds(m_db);
    if (!rounds.exec(QStringLiteral("SELECT id,tag_uid,chamber_no,round_no,status FROM capture_round ORDER BY chamber_no,round_no"))) { setError(error, rounds.lastError().text()); return false; }
    while (rounds.next()) {
        const int chamberNo = rounds.value(2).toInt();
        if (chamberNo < 1 || chamberNo > chambers->size()) continue;
        const QString uid = rounds.value(1).toString();
        auto &model = (*chambers)[chamberNo - 1];
        if (!model.profile || model.profile->uid != uid) continue;
        CaptureRound round(rounds.value(3).toInt(), rounds.value(4).toString() != QStringLiteral("in_progress"), rounds.value(0).toLongLong());
        QSqlQuery images(m_db); images.prepare(QStringLiteral("SELECT id,well_no,file_path,captured_at,is_active,file_available FROM capture_image WHERE round_id=? ORDER BY captured_at,id")); images.addBindValue(rounds.value(0));
        if (!images.exec()) { setError(error, images.lastError().text()); return false; }
        while (images.next()) {
            const int well = images.value(1).toInt(); if (well < 1 || well > 16) continue;
            WellCapture capture; capture.capturedAt = QDateTime::fromString(images.value(3).toString(), Qt::ISODate); capture.active = images.value(4).toInt() != 0; capture.available = images.value(5).toInt() != 0;
            const QString absolutePath = QDir(QFileInfo(m_db.databaseName()).absolutePath()).filePath(images.value(2).toString());
            if (capture.available && (!QFileInfo::exists(absolutePath) || !capture.image.load(absolutePath))) {
                capture.available = false;
                QSqlQuery unavailable(m_db);
                unavailable.prepare(QStringLiteral("UPDATE capture_image SET file_available=0 WHERE id=?"));
                unavailable.addBindValue(images.value(0));
                unavailable.exec();
            }
            round.history[well - 1].append(capture);
        }
        model.rounds.append(round);
    }
    return true;
}

bool Repository::nextRoundNumber(const QString &tagUid, int *roundNo, QString *error) const
{
    if (!roundNo || tagUid.isEmpty()) { setError(error, QStringLiteral("A tag UID is required.")); return false; }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT COALESCE(MAX(round_no), 0) + 1 FROM capture_round WHERE tag_uid=?"));
    query.addBindValue(tagUid);
    if (!query.exec() || !query.next()) { setError(error, query.lastError().text()); return false; }
    *roundNo = query.value(0).toInt();
    return true;
}

bool Repository::createRound(const TagProfile &p,int chamberNo,int roundNo,qint64 *id,QString *error)
{
    QSqlQuery q(m_db); q.prepare(QStringLiteral("INSERT INTO capture_round(tag_uid,chamber_no,round_no,status,started_at) VALUES(?,?,?,?,?)")); q.addBindValue(p.uid); q.addBindValue(chamberNo); q.addBindValue(roundNo); q.addBindValue(QStringLiteral("in_progress")); q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)); if(!q.exec()){setError(error,q.lastError().text());return false;} if(id)*id=q.lastInsertId().toLongLong(); return true;
}

bool Repository::finishRound(qint64 id, const QString &status, QString *error)
{
    QSqlQuery q(m_db); q.prepare(QStringLiteral("UPDATE capture_round SET status=?, finished_at=? WHERE id=?"));
    q.addBindValue(status); q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)); q.addBindValue(id);
    if (!q.exec()) { setError(error, q.lastError().text()); return false; }
    return true;
}

bool Repository::insertImage(qint64 roundId,const TagProfile&p,int wellNo,const QString&path,const QImage&image,const QDateTime&capturedAt,double exposure,double gain,qint64 replacedId,qint64*id,QString*error)
{
    if(!m_db.transaction()){setError(error,m_db.lastError().text());return false;} QSqlQuery q(m_db);
    if (replacedId <= 0) { q.prepare(QStringLiteral("SELECT id FROM capture_image WHERE round_id=? AND well_no=? AND focal_layer=0 AND is_active=1 ORDER BY id DESC LIMIT 1")); q.addBindValue(roundId); q.addBindValue(wellNo); if (!q.exec()) { m_db.rollback(); setError(error, q.lastError().text()); return false; } if (q.next()) replacedId = q.value(0).toLongLong(); }
    if(replacedId>0){q.prepare(QStringLiteral("UPDATE capture_image SET is_active=0 WHERE id=?"));q.addBindValue(replacedId);if(!q.exec()){m_db.rollback();setError(error,q.lastError().text());return false;}}
    q.prepare(QStringLiteral("INSERT INTO capture_image(round_id,tag_uid,well_no,focal_layer,file_path,captured_at,width,height,exposure_us,gain_db,is_active,file_available,replaced_image_id) VALUES(?,?,?,?,?,?,?,?,?,?,1,1,?)")); q.addBindValue(roundId);q.addBindValue(p.uid);q.addBindValue(wellNo);q.addBindValue(0);q.addBindValue(path);q.addBindValue(capturedAt.toUTC().toString(Qt::ISODateWithMs));q.addBindValue(image.width());q.addBindValue(image.height());q.addBindValue(exposure);q.addBindValue(gain);q.addBindValue(replacedId>0?QVariant(replacedId):QVariant()); if(!q.exec()||!m_db.commit()){m_db.rollback();setError(error,q.lastError().text());return false;} if(id)*id=q.lastInsertId().toLongLong();return true;
}
