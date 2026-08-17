#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>

struct DecodedRfidPayload {
    int formatVersion = 0;
    int dishNumber = 0;
    QDateTime inseminationTime;
    QString femaleName;
    QString medicalRecordNumber;
};

class RfidPayloadCodec final {
public:
    static bool decode(const QByteArray &raw, DecodedRfidPayload *payload, QString *error = nullptr);
};
