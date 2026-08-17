#include "imagefilestore.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QRegularExpression>

namespace { void setError(QString *error, const QString &value) { if (error) *error = value; } }

namespace {
QString sanitizedUid(QString uid)
{
    uid.replace(QRegularExpression(QStringLiteral("[^0-9A-Za-z_-]")), QStringLiteral("_"));
    return uid;
}

QString imageName(const QDateTime &capturedAt, const QString &extension)
{
    return capturedAt.toLocalTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))
        + QStringLiteral("_") + QUuid::createUuid().toString(QUuid::WithoutBraces)
        + QStringLiteral("_layer_00.") + extension;
}

bool writeImage(const QImage &image, const QString &absolutePath, const char *format, int quality, QString *error)
{
    if (!QDir().mkpath(QFileInfo(absolutePath).absolutePath())) {
        setError(error, QStringLiteral("Cannot create image directory."));
        return false;
    }
    const QString temporary = absolutePath + QStringLiteral(".tmp");
    if (!image.save(temporary, format, quality) || (QFileInfo::exists(absolutePath) && !QFile::remove(absolutePath))) {
        setError(error, QStringLiteral("Cannot write image file."));
        QFile::remove(temporary);
        return false;
    }
    if (!QFile::rename(temporary, absolutePath)) {
        setError(error, QStringLiteral("Cannot finalize image file."));
        QFile::remove(temporary);
        return false;
    }
    return true;
}
}

ImageFileStore::ImageFileStore(QString root) : m_root(std::move(root)) {}

bool ImageFileStore::savePng(const QImage &image, const QString &uid, int roundNo, int wellNo,
                             const QDateTime &capturedAt, QString *relativePath, QString *error) const
{
    if (image.isNull() || uid.isEmpty() || roundNo < 1 || wellNo < 1 || wellNo > 16) {
        setError(error, QStringLiteral("Invalid image capture arguments.")); return false;
    }
    const QString safeUid = sanitizedUid(uid);
    const QString date = capturedAt.toLocalTime().toString(QStringLiteral("yyyyMMdd"));
    const QString fileName = imageName(capturedAt, QStringLiteral("png"));
    const QString rel = QStringLiteral("images/%1/%2/round_%3/well_%4/%5")
            .arg(safeUid, date).arg(roundNo, 4, 10, QLatin1Char('0')).arg(wellNo, 2, 10, QLatin1Char('0')).arg(fileName);
    const QString absolute = QDir(m_root).filePath(rel);
    if (!writeImage(image, absolute, "PNG", -1, error)) return false;
    if (relativePath) *relativePath = QDir(m_root).relativeFilePath(absolute).replace(QDir::separator(), QLatin1Char('/'));
    return true;
}

bool ImageFileStore::stageJpeg(const QImage &image, const QString &uid, int roundNo, int wellNo,
                               const QDateTime &capturedAt, QString *relativePath, QString *error) const
{
    if (image.isNull() || uid.isEmpty() || roundNo < 1 || wellNo < 1 || wellNo > 16) {
        setError(error, QStringLiteral("Invalid image capture arguments."));
        return false;
    }
    const QString rel = QStringLiteral("staging/%1/%2/round_%3/well_%4/%5")
        .arg(sanitizedUid(uid), capturedAt.toLocalTime().toString(QStringLiteral("yyyyMMdd")))
        .arg(roundNo, 4, 10, QLatin1Char('0')).arg(wellNo, 2, 10, QLatin1Char('0'))
        .arg(imageName(capturedAt, QStringLiteral("jpg")));
    const QString absolute = QDir(m_root).filePath(rel);
    if (!writeImage(image, absolute, "JPG", 90, error)) return false;
    if (relativePath) *relativePath = rel;
    return true;
}

bool ImageFileStore::finalizeStagedJpeg(const QString &stagedPath, const QString &uid, int roundNo, int wellNo,
                                        const QDateTime &capturedAt, QString *relativePath, QString *error) const
{
    if (stagedPath.isEmpty() || uid.isEmpty() || roundNo < 1 || wellNo < 1 || wellNo > 16) {
        setError(error, QStringLiteral("Invalid staged image arguments."));
        return false;
    }
    const QString rel = QStringLiteral("images/%1/%2/round_%3/well_%4/%5")
        .arg(sanitizedUid(uid), capturedAt.toLocalTime().toString(QStringLiteral("yyyyMMdd")))
        .arg(roundNo, 4, 10, QLatin1Char('0')).arg(wellNo, 2, 10, QLatin1Char('0'))
        .arg(imageName(capturedAt, QStringLiteral("jpg")));
    const QString source = QDir(m_root).filePath(stagedPath);
    const QString destination = QDir(m_root).filePath(rel);
    if (!QFileInfo::exists(source) || !QDir().mkpath(QFileInfo(destination).absolutePath())
        || (QFileInfo::exists(destination) && !QFile::remove(destination)) || !QFile::copy(source, destination)) {
        setError(error, QStringLiteral("Cannot finalize staged image."));
        return false;
    }
    if (relativePath) *relativePath = rel;
    return true;
}

bool ImageFileStore::clearCaptureStorage(QString *error) const
{
    for (const QString &name : {QStringLiteral("images"), QStringLiteral("staging")}) {
        const QString path = QDir(m_root).filePath(name);
        const QFileInfo info(path);
        if (info.exists() && (!info.isDir() || !QDir(path).removeRecursively())) {
            setError(error, QStringLiteral("Cannot remove capture storage directory: %1").arg(path));
            return false;
        }
    }
    return true;
}

void ImageFileStore::remove(const QString &relativePath) const
{
    if (!relativePath.isEmpty()) QFile::remove(QDir(m_root).filePath(relativePath));
}
