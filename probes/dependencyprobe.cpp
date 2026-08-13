#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>

#ifdef TLS401_HAS_IDS_PEAK
#include <peak/peak.hpp>
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    bool sqliteOk = false;
    {
        auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("probe"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        sqliteOk = db.open() && QSqlQuery(db).exec(QStringLiteral("SELECT 1"));
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("probe"));
    qInfo().noquote() << "Qt Widgets: PASS";
    qInfo().noquote() << "SQLite in-memory:" << (sqliteOk ? "PASS" : "FAIL");
#ifdef TLS401_HAS_RFID_SDK
    qInfo().noquote() << "RFID Win64 SDK link: PASS";
#else
    qWarning().noquote() << "RFID Win64 SDK link: NOT CONFIGURED";
#endif
#ifdef TLS401_HAS_IDS_PEAK
    try {
        peak::Library::Initialize();
        peak::Library::Close();
        qInfo().noquote() << "IDS Peak x64 SDK initialize/close: PASS";
    } catch (const std::exception &error) {
        qWarning().noquote() << "IDS Peak x64 SDK initialize/close: FAIL" << error.what();
        return 1;
    }
#else
    qWarning().noquote() << "IDS Peak x64 SDK initialize/close: NOT CONFIGURED";
#endif
    return sqliteOk ? 0 : 1;
}
