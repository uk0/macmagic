#pragma once
#include <QObject>
#include <QString>

class NetworkInterface;

class MACChanger : public QObject {
    Q_OBJECT
public:
    explicit MACChanger(NetworkInterface *netIface, QObject *parent = nullptr);

    bool changeMac(const QString &device, const QString &newMac);
    bool restoreMac(const QString &device);
    bool restoreAll();

signals:
    void macChanged(const QString &device, const QString &newMac);
    void macRestored(const QString &device);
    void error(const QString &device, const QString &message);

private:
    bool executeChange(const QString &device, const QString &mac);
    bool disableInterface(const QString &device);
    bool enableInterface(const QString &device);
    bool isWifiInterface(const QString &device) const;
    NetworkInterface *m_netIface;
};
