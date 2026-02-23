#pragma once
#include <QString>
#include <QByteArray>

class NetworkInfo {
public:
    // Get the IPv4 address of a network interface (e.g. "en0")
    static QString getInterfaceIp(const QString &device);

    // Get the default gateway IP for a given interface
    static QString getGatewayIp(const QString &device);

    // Get the MAC address of a given IP from the ARP table
    static QString getGatewayMac(const QString &gatewayIp);

    // Convert "XX:XX:XX:XX:XX:XX" to 6-byte QByteArray
    static QByteArray macToBytes(const QString &mac);

    // Convert "x.x.x.x" to 4-byte QByteArray
    static QByteArray ipToBytes(const QString &ip);

    // Get the MAC address of a network interface via ifconfig
    static QString getInterfaceMac(const QString &device);

    // Check if BPF devices (/dev/bpf*) are accessible for packet injection
    static bool isBpfAvailable();

private:
    NetworkInfo() = default;
};
