#pragma once

#include <QSqlDatabase>
#include <QString>

class Database final {
public:
    Database() = default;
    ~Database();
    bool open(const QString &path, QString *error = nullptr);
    void close();
    bool isOpen() const { return m_db.isOpen(); }
    QSqlDatabase database() const { return m_db; }
    QSqlDatabase connection() const { return m_db; }
    static bool initializeSchema(QSqlDatabase db, QString *error = nullptr);
private:
    QSqlDatabase m_db;
    QString m_connectionName;
};
