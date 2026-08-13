#include "rfidpayloadcodec.h"
#include <QRegularExpression>
#include <QTextCodec>

namespace { void fail(QString *error, const QString &value) { if (error) *error = value; } }

bool RfidPayloadCodec::decode(const QByteArray &raw, DecodedRfidPayload *payload, QString *error)
{
    if (!payload || raw.size() < 16) { fail(error, QStringLiteral("RFID payload is truncated.")); return false; }
    const int declared = static_cast<unsigned char>(raw.at(1));
    const bool whole = declared >= 16 && declared <= raw.size();
    const bool legacy = declared >= 14 && declared + 2 <= raw.size();
    if (!whole && !legacy) { fail(error, QStringLiteral("RFID payload length is invalid.")); return false; }
    const int total = whole ? declared : declared + 2;
    const auto bcd = raw.mid(3, 5);
    QString digits;
    for (char c : bcd) {
        const unsigned char v = static_cast<unsigned char>(c);
        if ((v >> 4) > 9 || (v & 0xf) > 9) { fail(error, QStringLiteral("RFID BCD time is invalid.")); return false; }
        digits += QString::number(v >> 4) + QString::number(v & 0xf);
    }
    const QDate date(2000 + digits.mid(0,2).toInt(), digits.mid(2,2).toInt(), digits.mid(4,2).toInt());
    const QTime time(digits.mid(6,2).toInt(), digits.mid(8,2).toInt());
    if (!date.isValid() || !time.isValid()) { fail(error, QStringLiteral("RFID BCD time is invalid.")); return false; }
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    if (!codec) { fail(error, QStringLiteral("GBK codec is unavailable.")); return false; }
    QByteArray nameBytes = raw.mid(8, 8); while (!nameBytes.isEmpty() && nameBytes.at(0) == ' ') nameBytes.remove(0,1);
    const QString name = codec->toUnicode(nameBytes).trimmed();
    if (name.isEmpty() || name.contains(QChar::ReplacementCharacter)) { fail(error, QStringLiteral("RFID female name is invalid.")); return false; }
    const QByteArray medical = raw.mid(16, total - 16);
    if (medical.isEmpty() || medical.size() > 64 || !QRegularExpression(QStringLiteral("^[A-Za-z0-9_-]+$")).match(QString::fromLatin1(medical)).hasMatch()) { fail(error, QStringLiteral("RFID medical record is invalid.")); return false; }
    payload->formatVersion = static_cast<unsigned char>(raw.at(0));
    payload->dishNumber = static_cast<unsigned char>(raw.at(2));
    payload->inseminationTime = QDateTime(date, time);
    payload->femaleName = name;
    payload->medicalRecordNumber = QString::fromLatin1(medical);
    return true;
}
