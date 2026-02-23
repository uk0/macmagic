#pragma once
#include <QObject>

class NetworkInterface;
class ArpSpoofer;
class DhcpClient;
class TrayManager;

class App : public QObject {
    Q_OBJECT
public:
    explicit App(QObject *parent = nullptr);
    ~App() override;

private:
    NetworkInterface *m_netIface;
    ArpSpoofer *m_arpSpoofer;
    DhcpClient *m_dhcpClient;
    TrayManager *m_tray;
};
