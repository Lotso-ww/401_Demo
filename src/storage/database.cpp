#include "database.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {
void setError(QString *error, const QString &value) { if (error) *error = value; }
}

Database::~Database() { close(); }

bool Database::open(const QString &path, QString *error)
{
    close();
    m_connectionName = QStringLiteral("tls401_%1").arg(static_cast<qulonglong>(reinterpret_cast<quintptr>(this)));
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(path);
    if (!m_db.open()) { setError(error, m_db.lastError().text()); close(); return false; }
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    return initializeSchema(m_db, error);
}

void Database::close()
{
    if (m_connectionName.isEmpty()) return;
    if (m_db.isValid()) m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName.clear();
}

bool Database::initializeSchema(QSqlDatabase db, QString *error)
{
    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS tag_profile (uid TEXT PRIMARY KEY, format_version INTEGER NOT NULL, dish_number INTEGER NOT NULL, insemination_time TEXT NOT NULL, female_name TEXT NOT NULL, medical_record_no TEXT NOT NULL, male_name TEXT, first_seen_at TEXT NOT NULL, last_seen_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS chamber_assignment (chamber_no INTEGER PRIMARY KEY CHECK(chamber_no BETWEEN 1 AND 4), tag_uid TEXT UNIQUE, identified_at TEXT, updated_at TEXT NOT NULL, FOREIGN KEY(tag_uid) REFERENCES tag_profile(uid))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS capture_round (id INTEGER PRIMARY KEY AUTOINCREMENT, tag_uid TEXT NOT NULL, chamber_no INTEGER NOT NULL, round_no INTEGER NOT NULL, status TEXT NOT NULL, started_at TEXT NOT NULL, finished_at TEXT, UNIQUE(tag_uid, round_no), FOREIGN KEY(tag_uid) REFERENCES tag_profile(uid))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS capture_image (id INTEGER PRIMARY KEY AUTOINCREMENT, round_id INTEGER NOT NULL, tag_uid TEXT NOT NULL, well_no INTEGER NOT NULL CHECK(well_no BETWEEN 1 AND 16), focal_layer INTEGER NOT NULL DEFAULT 0, file_path TEXT NOT NULL, captured_at TEXT NOT NULL, width INTEGER NOT NULL, height INTEGER NOT NULL, exposure_us REAL NOT NULL DEFAULT 0, gain_db REAL NOT NULL DEFAULT 0, is_active INTEGER NOT NULL DEFAULT 1, file_available INTEGER NOT NULL DEFAULT 1, replaced_image_id INTEGER, FOREIGN KEY(round_id) REFERENCES capture_round(id), FOREIGN KEY(tag_uid) REFERENCES tag_profile(uid), FOREIGN KEY(replaced_image_id) REFERENCES capture_image(id))"),
        QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_capture_image_active ON capture_image(round_id, well_no, focal_layer) WHERE is_active=1"),
        QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_capture_round_active ON capture_round(tag_uid) WHERE status='in_progress'"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_capture_image_round_well ON capture_image(round_id, well_no, captured_at)")
    };
    for (const QString &statement : statements) {
        QSqlQuery query(db);
        if (!query.exec(statement)) { setError(error, query.lastError().text()); return false; }
    }
    QSqlQuery columns(db);
    if (!columns.exec(QStringLiteral("PRAGMA table_info(capture_image)"))) { setError(error, columns.lastError().text()); return false; }
    bool hasAvailability = false;
    while (columns.next()) hasAvailability = hasAvailability || columns.value(1).toString() == QStringLiteral("file_available");
    if (!hasAvailability) {
        QSqlQuery migration(db);
        if (!migration.exec(QStringLiteral("ALTER TABLE capture_image ADD COLUMN file_available INTEGER NOT NULL DEFAULT 1"))) { setError(error, migration.lastError().text()); return false; }
    }
    return true;
}
