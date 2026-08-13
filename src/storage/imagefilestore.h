#pragma once

#include <QImage>
#include <QDateTime>
#include <QString>

class ImageFileStore final {
public:
    explicit ImageFileStore(QString root = {});
    QString root() const { return m_root; }
    bool savePng(const QImage &image, const QString &uid, int roundNo, int wellNo,
                 const QDateTime &capturedAt, QString *relativePath, QString *error = nullptr) const;
    void remove(const QString &relativePath) const;
private:
    QString m_root;
};
