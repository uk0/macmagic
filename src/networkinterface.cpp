#include "networkinterface.h"

#include <QProcess>
#include <QRegularExpression>

NetworkInterface::NetworkInterface(QObject *parent)
    : QObject(parent)
{
    discover();
}

QList<InterfaceInfo> NetworkInterface::listInterfaces() const
{
    return m_interfaces;
}

QString NetworkInterface::getCurrentMac(const QString &device) const
{
    QProcess proc;
    proc.start("ifconfig", QStringList() << device);
    proc.waitForFinished(3000);

    const QString output = proc.readAllStandardOutput();
    static const QRegularExpression re(R"(\bether\s+([0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5})\b)");
    const auto match = re.match(output);
    if (match.hasMatch())
        return match.captured(1).toLower();

    return QString();
}

QString NetworkInterface::getOriginalMac(const QString &device) const
{
    for (const auto &iface : m_interfaces) {
        if (iface.device == device)
            return iface.originalMac;
    }
    return QString();
}

void NetworkInterface::refresh()
{
    // Preserve original MACs before re-discovery
    QHash<QString, QString> originals;
    for (const auto &iface : m_interfaces)
        originals.insert(iface.device, iface.originalMac);

    discover();

    // Restore original MACs and update current MACs from live ifconfig
    for (auto &iface : m_interfaces) {
        if (originals.contains(iface.device))
            iface.originalMac = originals.value(iface.device);

        const QString live = getCurrentMac(iface.device);
        if (!live.isEmpty())
            iface.currentMac = live;
    }

    emit interfacesChanged();
}

void NetworkInterface::discover()
{
    QProcess proc;
    proc.start("networksetup", QStringList() << "-listallhardwareports");
    proc.waitForFinished(5000);

    const QString output = proc.readAllStandardOutput();
    const QStringList blocks = output.split("\n\n", Qt::SkipEmptyParts);

    QList<InterfaceInfo> discovered;

    static const QRegularExpression portRe(R"(Hardware Port:\s*(.+))");
    static const QRegularExpression deviceRe(R"(Device:\s*(\S+))");
    static const QRegularExpression macRe(R"(Ethernet Address:\s*([0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5}))");

    for (const QString &block : blocks) {
        const auto portMatch  = portRe.match(block);
        const auto deviceMatch = deviceRe.match(block);
        const auto macMatch   = macRe.match(block);

        if (!portMatch.hasMatch() || !deviceMatch.hasMatch() || !macMatch.hasMatch())
            continue;

        const QString mac = macMatch.captured(1).trimmed().toLower();
        if (mac.isEmpty() || mac.compare("n/a", Qt::CaseInsensitive) == 0)
            continue;

        InterfaceInfo info;
        info.hardwarePort = portMatch.captured(1).trimmed();
        info.device       = deviceMatch.captured(1).trimmed();
        info.hardwareMac  = mac;
        info.currentMac   = mac;
        info.originalMac  = mac;  // set on first discovery; preserved across refresh()

        discovered.append(info);
    }

    m_interfaces = discovered;

    // Update currentMac and originalMac from live ifconfig (networksetup reports
    // hardware MAC, but macOS 15+ private Wi-Fi address rotates the actual MAC)
    for (auto &iface : m_interfaces) {
        const QString live = getCurrentMac(iface.device);
        if (!live.isEmpty()) {
            iface.currentMac = live;
            iface.originalMac = live;  // on first discovery, original = live
        }
    }
}
