#pragma once
#include <QString>
#include <QByteArray>

class MACGenerator {
public:
    // Generate a random locally-administered unicast MAC address
    static QString generateRandom();

    // Generate random MAC with specific OUI prefix (first 3 bytes)
    static QString generateWithPrefix(const QString &prefix);

    // Format 6 bytes to "XX:XX:XX:XX:XX:XX"
    static QString formatMac(const QByteArray &bytes);

    // Validate MAC address string format
    static bool isValid(const QString &mac);

    // Normalize MAC to uppercase with colons
    static QString normalize(const QString &mac);

private:
    MACGenerator() = default;
};
