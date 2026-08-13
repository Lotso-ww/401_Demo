#include "imagefilestore.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QRegularExpression>

namespace { void setError(QString *error, const QString &value) { if (error) *error = value; } }

ImageFileStore::ImageFileStore(QString root) : m_root(std::move(root)) {}

bool ImageFileStore::savePng(const QImage &image, const QString &uid, int roundNo, int wellNo,
                             const QDateTime &capturedAt, QString *relativePath, QString *error) const
{
    if (image.isNull() || uid.isEmpty() || roundNo < 1 || wellNo < 1 || wellNo > 16) {
        setError(error, QStringLiteral("Invalid image capture arguments.")); return false;
    }
    QString safeUid = uid;
    safeUid.replace(QRegularExpression(QStringLiteral("[^0-9A-Za-z_-]")), QStringLiteral("_"));
    const QString date = capturedAt.toLocalTime().toString(QStringLiteral("yyyyMMdd"));
    const QString fileName = capturedAt.toLocalTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))
            + QStringLiteral("_") + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("_layer_00.png");
    const QString rel = QStringLiteral("images/%1/%2/round_%3/well_%4/%5")
            .arg(safeUid, date).arg(roundNo, 4, 10, QLatin1Char('0')).arg(wellNo, 2, 10, QLatin1Char('0')).arg(fileName);
    const QString absolute = QDir(m_root).filePath(rel);
    if (!QDir().mkpath(QFileInfo(absolute).absolutePath())) { setError(error, QStringLiteral("Cannot create image directory.")); return false; }
    const QString temporary = absolute + QStringLiteral(".tmp");
    if (!image.save(temporary, "PNG") || (QFileInfo::exists(absolute) && !QFile::remove(absolute))) {
        setError(error, QStringLiteral("Cannot write PNG image.")); QFile::remove(temporary); return false;
    }
    if (!QFile::rename(temporary, absolute)) { setError(error, QStringLiteral("Cannot finalize PNG image.")); QFile::remove(temporary); return false; }
    if (relativePath) *relativePath = QDir(m_root).relativeFilePath(absolute).replace(QDir::separator(), QLatin1Char('/'));
    return true;
}

void ImageFileStore::remove(const QString &relativePath) const
{
    if (!relativePath.isEmpty()) QFile::remove(QDir(m_root).filePath(relativePath));
}
