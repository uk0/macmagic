#include "traymanager.h"
#include "networkinterface.h"
#include "macgenerator.h"
#include "arpspoofer.h"
#include "dhcpclient.h"
#include "networkinfo.h"
#include "wifiinfo.h"

#include <QApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>

TrayManager::TrayManager(NetworkInterface *netIface,
                         ArpSpoofer *arpSpoofer, DhcpClient *dhcpClient,
                         QObject *parent)
    : QObject(parent)
    , m_trayIcon(nullptr)
    , m_menu(new QMenu)
    , m_netIface(netIface)
    , m_arpSpoofer(arpSpoofer)
    , m_dhcpClient(dhcpClient)
{
    createTrayIcon();
    rebuildMenu();

    connect(m_netIface, &NetworkInterface::interfacesChanged, this, &TrayManager::rebuildMenu);

    connect(m_arpSpoofer, &ArpSpoofer::started, this, [this](const QString &device) {
        updateTrayIcon();
        rebuildMenu();
        m_trayIcon->showMessage(QStringLiteral("Spoofing Active"),
                                QStringLiteral("MAC spoofing on %1").arg(device),
                                QSystemTrayIcon::Information, 3000);
    });

    connect(m_arpSpoofer, &ArpSpoofer::stopped, this, [this](const QString &device) {
        updateTrayIcon();
        rebuildMenu();
        m_trayIcon->showMessage(QStringLiteral("Spoofing Stopped"),
                                QStringLiteral("Stopped on %1").arg(device),
                                QSystemTrayIcon::Information, 3000);
    });

    connect(m_arpSpoofer, &ArpSpoofer::error, this, [this](const QString &device, const QString &message) {
        m_trayIcon->showMessage(QStringLiteral("Spoof Error"),
                                QStringLiteral("%1: %2").arg(device, message),
                                QSystemTrayIcon::Critical, 5000);
    });
    connect(m_dhcpClient, &DhcpClient::ipObtained, this, [this](const QString &device, const QString &ip) {
        rebuildMenu();
        m_trayIcon->showMessage(QStringLiteral("DHCP Success"),
                                QStringLiteral("%1 obtained IP: %2").arg(device, ip),
                                QSystemTrayIcon::Information, 5000);
    });

    connect(m_dhcpClient, &DhcpClient::error, this, [this](const QString &device, const QString &message) {
        m_trayIcon->showMessage(QStringLiteral("DHCP Error"),
                                QStringLiteral("%1: %2").arg(device, message),
                                QSystemTrayIcon::Critical, 5000);
    });
}

void TrayManager::show()
{
    m_trayIcon->show();
}

void TrayManager::createTrayIcon()
{
    m_normalIcon = IconProvider::trayNormal();
    m_activeIcon = IconProvider::trayActive();
    m_trayIcon = new QSystemTrayIcon(m_normalIcon, this);
    m_trayIcon->setToolTip(QStringLiteral("MacMagic"));
}

void TrayManager::updateTrayIcon()
{
    if (!m_arpSpoofer->activeDevices().isEmpty())
        m_trayIcon->setIcon(m_activeIcon);
    else
        m_trayIcon->setIcon(m_normalIcon);
}
void TrayManager::rebuildMenu()
{
    m_menu->clear();

    const bool bpfOk = NetworkInfo::isBpfAvailable();
    const auto interfaces = m_netIface->listInterfaces();

    for (const auto &iface : interfaces) {
        bool active = m_arpSpoofer->isActive(iface.device);
        QString dev = iface.device;

        // --- Submenu with colored dot icon ---
        QIcon dotIcon = active ? IconProvider::greenDot() : IconProvider::grayDot();
        QString title = QStringLiteral("%1 (%2)").arg(iface.hardwarePort, dev);
        auto *sub = m_menu->addMenu(dotIcon, title);

        // === Network Info section ===
        auto *hdrInfo = sub->addAction(IconProvider::networkIcon(), QStringLiteral("Network"));
        hdrInfo->setEnabled(false);

        QString ip = NetworkInfo::getInterfaceIp(dev);
        auto *ipAct = sub->addAction(QStringLiteral("    IP Address          %1").arg(
            ip.isEmpty() ? QStringLiteral("\u2014") : ip));
        ipAct->setEnabled(false);

        QString gw = NetworkInfo::getGatewayIp(dev);
        auto *gwAct = sub->addAction(QStringLiteral("    Gateway              %1").arg(
            gw.isEmpty() ? QStringLiteral("\u2014") : gw));
        gwAct->setEnabled(false);

        auto *macAct = sub->addAction(QStringLiteral("    MAC Address     %1").arg(iface.currentMac.toUpper()));
        macAct->setEnabled(false);

        if (iface.hardwareMac != iface.currentMac) {
            auto *hwAct = sub->addAction(QStringLiteral("    Hardware MAC   %1").arg(iface.hardwareMac.toUpper()));
            hwAct->setEnabled(false);
        }

        // Wi-Fi details
        bool isWifi = iface.hardwarePort.contains(QStringLiteral("Wi-Fi"), Qt::CaseInsensitive);
        if (isWifi) {
            WifiDetails wifi = getWifiDetails(dev);
            if (wifi.connected) {
                QString ssidText = wifi.ssid.isEmpty()
                    ? QStringLiteral("Connected")
                    : wifi.ssid;
                auto *ssidAct = sub->addAction(IconProvider::wifiIcon(),
                    QStringLiteral("%1  %2 Ch%3").arg(ssidText).arg(wifi.band).arg(wifi.channel));
                ssidAct->setEnabled(false);

                auto *sigAct = sub->addAction(QStringLiteral("    Signal                %1 dBm   %2 Mbps")
                    .arg(wifi.rssi).arg(wifi.txRate, 0, 'f', 0));
                sigAct->setEnabled(false);
            } else {
                auto *dcAct = sub->addAction(IconProvider::wifiIcon(), QStringLiteral("Wi-Fi Disconnected"));
                dcAct->setEnabled(false);
            }
        }

        // === Spoof Status section ===
        sub->addSeparator();

        if (active) {
            QString spoofMac = m_arpSpoofer->spoofedMac(dev);
            auto *statusAct = sub->addAction(IconProvider::shieldIcon(),
                QStringLiteral("Spoofing as %1").arg(spoofMac.toUpper()));
            statusAct->setEnabled(false);

            sub->addSeparator();
            connect(sub->addAction(IconProvider::stopIcon(), QStringLiteral("Stop Spoofing")),
                    &QAction::triggered, this, [this, dev]() { onStopSpoof(dev); });
        } else {
            auto *statusAct = sub->addAction(IconProvider::shieldIcon(), QStringLiteral("Not active"));
            statusAct->setEnabled(false);

            if (!bpfOk) {
                auto *hint = sub->addAction(IconProvider::warningIcon(),
                    QStringLiteral("BPF unavailable (join access_bpf)"));
                hint->setEnabled(false);
            } else {
                sub->addSeparator();

                connect(sub->addAction(IconProvider::diceIcon(), QStringLiteral("Random MAC")),
                        &QAction::triggered, this, [this, dev]() { onSpoofRandom(dev); });
                connect(sub->addAction(IconProvider::pencilIcon(), QStringLiteral("Custom MAC...")),
                        &QAction::triggered, this, [this, dev]() { onSpoofCustom(dev); });

                sub->addSeparator();

                connect(sub->addAction(IconProvider::diceIcon(), QStringLiteral("Random + DHCP")),
                        &QAction::triggered, this, [this, dev]() { onSpoofDhcpRandom(dev); });
                connect(sub->addAction(IconProvider::pencilIcon(), QStringLiteral("Custom + DHCP...")),
                        &QAction::triggered, this, [this, dev]() { onSpoofDhcpCustom(dev); });
            }
        }
    }

    // === Global actions ===
    m_menu->addSeparator();

    if (!m_arpSpoofer->activeDevices().isEmpty()) {
        connect(m_menu->addAction(IconProvider::stopIcon(), QStringLiteral("Stop All Spoofing")),
                &QAction::triggered, this, &TrayManager::onStopAll);
        m_menu->addSeparator();
    }

    connect(m_menu->addAction(IconProvider::refreshIcon(), QStringLiteral("Refresh")),
            &QAction::triggered, m_netIface, &NetworkInterface::refresh);

    connect(m_menu->addAction(IconProvider::infoIcon(), QStringLiteral("About MacMagic")),
            &QAction::triggered, this, [this]() {
        QMessageBox about;
        about.setWindowTitle(QStringLiteral("About MacMagic"));
        about.setIconPixmap(IconProvider::appIcon().pixmap(128, 128));
        about.setTextFormat(Qt::RichText);
        about.setText(QStringLiteral(
            "<h2 style='margin-bottom:2px;'>MacMagic</h2>"
            "<p style='color:#888; margin-top:0;'>v2.1.0</p>"
            "<p>ARP-based MAC Spoofing for macOS<br>With DHCP support</p>"
            "<p>"
            "<a href='https://github.com/uk0'>GitHub: github.com/uk0</a><br>"
            "<a href='https://firsh.me'>Blog: firsh.me</a>"
            "</p>"
        ));
        about.setStandardButtons(QMessageBox::Ok);
        about.exec();
    });

    connect(m_menu->addAction(IconProvider::quitIcon(), QStringLiteral("Quit")),
            &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_menu);
}
void TrayManager::onSpoofRandom(const QString &device)
{
    QString mac = MACGenerator::generateRandom();
    if (!m_arpSpoofer->start(device, mac)) {
        m_trayIcon->showMessage(QStringLiteral("Spoof Failed"),
                                QStringLiteral("Could not start ARP spoof on %1").arg(device),
                                QSystemTrayIcon::Critical, 5000);
    }
}

void TrayManager::onSpoofCustom(const QString &device)
{
    bool ok = false;
    QString mac = QInputDialog::getText(nullptr, QStringLiteral("Custom MAC"),
                                        QStringLiteral("Enter MAC address (e.g. AA:BB:CC:DD:EE:FF):"),
                                        QLineEdit::Normal, QString(), &ok);
    if (!ok || mac.isEmpty())
        return;

    mac = MACGenerator::normalize(mac);
    if (!MACGenerator::isValid(mac)) {
        QMessageBox::warning(nullptr, QStringLiteral("Invalid MAC"),
                             QStringLiteral("'%1' is not a valid MAC address.").arg(mac));
        return;
    }

    if (!m_arpSpoofer->start(device, mac)) {
        m_trayIcon->showMessage(QStringLiteral("Spoof Failed"),
                                QStringLiteral("Could not start ARP spoof on %1").arg(device),
                                QSystemTrayIcon::Critical, 5000);
    }
}

void TrayManager::onSpoofDhcpRandom(const QString &device)
{
    QString mac = MACGenerator::generateRandom();
    if (!m_arpSpoofer->start(device, mac)) {
        m_trayIcon->showMessage(QStringLiteral("Spoof Failed"),
                                QStringLiteral("Could not start ARP spoof on %1").arg(device),
                                QSystemTrayIcon::Critical, 5000);
        return;
    }
    if (!m_dhcpClient->requestIp(device, mac)) {
        m_trayIcon->showMessage(QStringLiteral("DHCP Failed"),
                                QStringLiteral("Could not send DHCP request on %1").arg(device),
                                QSystemTrayIcon::Warning, 5000);
    }
}

void TrayManager::onSpoofDhcpCustom(const QString &device)
{
    bool ok = false;
    QString mac = QInputDialog::getText(nullptr, QStringLiteral("Custom MAC + DHCP"),
                                        QStringLiteral("Enter MAC address (e.g. AA:BB:CC:DD:EE:FF):"),
                                        QLineEdit::Normal, QString(), &ok);
    if (!ok || mac.isEmpty())
        return;

    mac = MACGenerator::normalize(mac);
    if (!MACGenerator::isValid(mac)) {
        QMessageBox::warning(nullptr, QStringLiteral("Invalid MAC"),
                             QStringLiteral("'%1' is not a valid MAC address.").arg(mac));
        return;
    }

    if (!m_arpSpoofer->start(device, mac)) {
        m_trayIcon->showMessage(QStringLiteral("Spoof Failed"),
                                QStringLiteral("Could not start ARP spoof on %1").arg(device),
                                QSystemTrayIcon::Critical, 5000);
        return;
    }
    if (!m_dhcpClient->requestIp(device, mac)) {
        m_trayIcon->showMessage(QStringLiteral("DHCP Failed"),
                                QStringLiteral("Could not send DHCP request on %1").arg(device),
                                QSystemTrayIcon::Warning, 5000);
    }
}

void TrayManager::onStopSpoof(const QString &device)
{
    m_arpSpoofer->stop(device, true);
}

void TrayManager::onStopAll()
{
    m_arpSpoofer->stopAll(true);
}
