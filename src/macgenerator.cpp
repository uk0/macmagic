#include "macgenerator.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>

QString MACGenerator::generateRandom()
{
    QByteArray bytes(6, 0);
    for (int i = 0; i < 6; ++i)
        bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));

    // Set locally administered bit (bit 1) and clear multicast bit (bit 0)
    bytes[0] = static_cast<char>((static_cast<quint8>(bytes[0]) & 0xFC) | 0x02);

    return formatMac(bytes);
}

QString MACGenerator::generateWithPrefix(const QString &prefix)
{
    // Parse prefix "XX:XX:XX"
    QString norm = normalize(prefix);
    QStringList parts = norm.split(':');
    if (parts.size() != 3)
        return generateRandom();

    QByteArray bytes(6, 0);
    for (int i = 0; i < 3; ++i)
        bytes[i] = static_cast<char>(parts[i].toUInt(nullptr, 16));

    for (int i = 3; i < 6; ++i)
        bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));

    return formatMac(bytes);
}

QString MACGenerator::formatMac(const QByteArray &bytes)
{
    if (bytes.size() < 6)
        return QString();

    QStringList parts;
    for (int i = 0; i < 6; ++i)
        parts << QString("%1").arg(static_cast<quint8>(bytes[i]), 2, 16, QChar('0')).toUpper();

    return parts.join(':');
}

bool MACGenerator::isValid(const QString &mac)
{
    static const QRegularExpression reColon(
        "^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$");
    static const QRegularExpression reDash(
        "^([0-9A-Fa-f]{2}-){5}[0-9A-Fa-f]{2}$");

    return reColon.match(mac).hasMatch() || reDash.match(mac).hasMatch();
}

QString MACGenerator::normalize(const QString &mac)
{
    QString result = mac.trimmed().toUpper();
    result.replace('-', ':');
    return result;
}
