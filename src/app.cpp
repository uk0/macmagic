#include "app.h"
#include "networkinterface.h"
#include "arpspoofer.h"
#include "dhcpclient.h"
#include "traymanager.h"
#include "iconprovider.h"

#include <QApplication>

App::App(QObject *parent)
    : QObject(parent)
    , m_netIface(new NetworkInterface(this))
    , m_arpSpoofer(new ArpSpoofer(this))
    , m_dhcpClient(new DhcpClient(this))
    , m_tray(new TrayManager(m_netIface, m_arpSpoofer, m_dhcpClient, this))
{
    connect(qApp, &QApplication::aboutToQuit, this, [this]() {
        m_arpSpoofer->stopAll(true);
        m_dhcpClient->cancel();
    });

    m_tray->show();
    qApp->setWindowIcon(IconProvider::appIcon());
}

App::~App() = default;
