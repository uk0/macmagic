#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include "iconprovider.h"

class NetworkInterface;
class ArpSpoofer;
class DhcpClient;

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(NetworkInterface *netIface,
                         ArpSpoofer *arpSpoofer, DhcpClient *dhcpClient,
                         QObject *parent = nullptr);
    void show();

private slots:
    void rebuildMenu();
    void onSpoofRandom(const QString &device);
    void onSpoofCustom(const QString &device);
    void onSpoofDhcpRandom(const QString &device);
    void onSpoofDhcpCustom(const QString &device);
    void onStopSpoof(const QString &device);
    void onStopAll();

private:
    void createTrayIcon();
    void updateTrayIcon();
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    NetworkInterface *m_netIface;
    ArpSpoofer *m_arpSpoofer;
    DhcpClient *m_dhcpClient;
    QIcon m_normalIcon;
    QIcon m_activeIcon;
};
