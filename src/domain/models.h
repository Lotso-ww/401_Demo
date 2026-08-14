#pragma once


#include <QDateTime>
#include <QImage>
#include <QString>
#include <QVector>
#include <optional>

enum class DeviceState { Offline, Ready, Busy, Error };

enum class RfidError { None, NoTag, InvalidPayload, DuplicateUid, Cancelled };
enum class WellState { Empty, Current, Complete, RetakeRequired };

struct TagProfile {
    QString uid;
    QString dishNumber;
    QDateTime inseminationTime;
    QString femaleName;
    QString medicalRecordNumber;
    QString maleName;
    QDateTime identifiedAt;
};

struct RfidResult {
    RfidError error = RfidError::None;
    TagProfile profile;
    QString message;
    bool ok() const { return error == RfidError::None; }
};

struct WellCapture {
    QImage image;
    QDateTime capturedAt;
    bool active = true;
    bool available = true;
    double exposureUs = 0;
    double gainDb = 0;
};

struct CaptureRound {
    int number = 0;
    bool finished = false;
    QVector<QVector<WellCapture>> history;
    qint64 persistentId = 0;

    CaptureRound() : history(16) {}
    CaptureRound(int roundNumber, bool isFinished, qint64 id = 0)
        : number(roundNumber), finished(isFinished), history(16), persistentId(id) {}
};

struct ChamberModel {
    int number = 0;
    std::optional<TagProfile> profile;
    QVector<CaptureRound> rounds;
};

inline int developmentDays(const TagProfile &profile) {
    return profile.inseminationTime.isValid() ? qMax(0, static_cast<int>(profile.inseminationTime.daysTo(QDateTime::currentDateTime()))) : 0;
}
