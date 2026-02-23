#pragma once
#include <QObject>
#include <QString>
#include <QList>

struct InterfaceInfo {
    QString hardwarePort;  // e.g. "Wi-Fi", "Thunderbolt 1"
    QString device;        // e.g. "en0", "en1"
    QString currentMac;    // current MAC address (from ifconfig)
    QString originalMac;   // MAC at first discovery (from ifconfig)
    QString hardwareMac;   // true hardware MAC (from networksetup, never changes)
    bool isModified() const { return currentMac != originalMac; }
};

class NetworkInterface : public QObject {
    Q_OBJECT
public:
    explicit NetworkInterface(QObject *parent = nullptr);

    QList<InterfaceInfo> listInterfaces() const;
    QString getCurrentMac(const QString &device) const;
    QString getOriginalMac(const QString &device) const;
    void refresh();

signals:
    void interfacesChanged();

private:
    void discover();
    QList<InterfaceInfo> m_interfaces;
};
