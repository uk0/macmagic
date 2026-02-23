#include "macchanger.h"
#include "macgenerator.h"
#include "networkinterface.h"

#include <QProcess>

MACChanger::MACChanger(NetworkInterface *netIface, QObject *parent)
    : QObject(parent)
    , m_netIface(netIface)
{
}

bool MACChanger::changeMac(const QString &device, const QString &newMac)
{
    if (!MACGenerator::isValid(newMac)) {
        emit error(device, QStringLiteral("Invalid MAC address format: %1").arg(newMac));
        return false;
    }

    const QString normalized = MACGenerator::normalize(newMac);

    if (!executeChange(device, normalized)) {
        if (isWifiInterface(device))
            emit error(device, QStringLiteral("ifconfig cannot change Wi-Fi MAC on macOS 15+. Use ARP Spoof instead."));
        else
            emit error(device, QStringLiteral("Failed to change MAC address on %1").arg(device));
        return false;
    }

    m_netIface->refresh();
    emit macChanged(device, normalized);
    return true;
}

bool MACChanger::restoreMac(const QString &device)
{
    const QString originalMac = m_netIface->getOriginalMac(device);
    if (originalMac.isEmpty()) {
        emit error(device, QStringLiteral("No original MAC recorded for %1").arg(device));
        return false;
    }

    const QString normalized = MACGenerator::normalize(originalMac);

    if (!executeChange(device, normalized)) {
        emit error(device, QStringLiteral("Failed to restore MAC address on %1").arg(device));
        return false;
    }

    m_netIface->refresh();
    emit macRestored(device);
    return true;
}

bool MACChanger::restoreAll()
{
    bool allOk = true;
    const auto interfaces = m_netIface->listInterfaces();
    for (const auto &iface : interfaces) {
        if (iface.isModified()) {
            if (!restoreMac(iface.device))
                allOk = false;
        }
    }
    return allOk;
}

bool MACChanger::executeChange(const QString &device, const QString &mac)
{
    const bool wifi = isWifiInterface(device);

    if (wifi) {
        if (!disableInterface(device))
            return false;
    }

    // Use osascript to prompt for admin privileges via the macOS auth dialog
    const QString script = QStringLiteral(
        "do shell script \"ifconfig %1 ether %2\" with administrator privileges"
    ).arg(device, mac);

    QProcess proc;
    proc.start(QStringLiteral("osascript"), {QStringLiteral("-e"), script});

    // 30s timeout: user needs time to authenticate in the dialog
    if (!proc.waitForFinished(30000)) {
        proc.kill();
        if (wifi)
            enableInterface(device);
        return false;
    }

    const bool cmdOk = (proc.exitCode() == 0);

    if (wifi)
        enableInterface(device);

    if (!cmdOk)
        return false;

    // Verify the MAC actually changed (macOS 15+ may silently ignore ifconfig ether on Wi-Fi)
    const QString actual = m_netIface->getCurrentMac(device);
    if (!actual.isEmpty() && actual.compare(mac, Qt::CaseInsensitive) != 0)
        return false;

    return true;
}

bool MACChanger::disableInterface(const QString &device)
{
    if (isWifiInterface(device)) {
        QProcess proc;
        proc.start(QStringLiteral("networksetup"),
                    {QStringLiteral("-setairportpower"), device, QStringLiteral("off")});
        return proc.waitForFinished(10000) && proc.exitCode() == 0;
    }

    const QString script = QStringLiteral(
        "do shell script \"ifconfig %1 down\" with administrator privileges"
    ).arg(device);

    QProcess proc;
    proc.start(QStringLiteral("osascript"), {QStringLiteral("-e"), script});
    return proc.waitForFinished(30000) && proc.exitCode() == 0;
}

bool MACChanger::enableInterface(const QString &device)
{
    if (isWifiInterface(device)) {
        QProcess proc;
        proc.start(QStringLiteral("networksetup"),
                    {QStringLiteral("-setairportpower"), device, QStringLiteral("on")});
        return proc.waitForFinished(10000) && proc.exitCode() == 0;
    }

    const QString script = QStringLiteral(
        "do shell script \"ifconfig %1 up\" with administrator privileges"
    ).arg(device);

    QProcess proc;
    proc.start(QStringLiteral("osascript"), {QStringLiteral("-e"), script});
    return proc.waitForFinished(30000) && proc.exitCode() == 0;
}

bool MACChanger::isWifiInterface(const QString &device) const
{
    const auto interfaces = m_netIface->listInterfaces();
    for (const auto &iface : interfaces) {
        if (iface.device == device)
            return iface.hardwarePort.contains(QStringLiteral("Wi-Fi"), Qt::CaseInsensitive);
    }
    // Fallback: en0 is typically Wi-Fi on macOS
    return device == QStringLiteral("en0");
}
