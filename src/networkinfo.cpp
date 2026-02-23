#include "networkinfo.h"
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

QString NetworkInfo::getInterfaceIp(const QString &device)
{
    QProcess proc;
    proc.start("ifconfig", {device});
    proc.waitForFinished(3000);

    const QString output = proc.readAllStandardOutput();
    static const QRegularExpression re(R"(inet (\d+\.\d+\.\d+\.\d+))");

    QRegularExpressionMatchIterator it = re.globalMatch(output);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        const QString ip = m.captured(1);
        if (!ip.startsWith("127."))
            return ip;
    }
    return QString();
}

QString NetworkInfo::getGatewayIp(const QString &device)
{
    // Parse netstat -rn for the device-specific default route
    // This is more reliable than "route -n get default" which returns
    // the global default (often a VPN tunnel, not the physical interface)
    QProcess proc;
    proc.start("netstat", {"-rn", "-f", "inet"});
    proc.waitForFinished(3000);

    const QString output = proc.readAllStandardOutput();
    const QStringList lines = output.split('\n');

    for (const QString &line : lines) {
        if (!line.trimmed().startsWith("default"))
            continue;
        // Match: default  <gateway_ip>  <flags>  <iface>
        // Columns are whitespace-separated, iface is the last token
        const QStringList cols = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (cols.size() < 4)
            continue;
        // Last column is the interface name
        if (cols.last() != device)
            continue;
        // Second column is the gateway IP
        static const QRegularExpression reIp(R"(^\d+\.\d+\.\d+\.\d+$)");
        if (reIp.match(cols[1]).hasMatch())
            return cols[1];
    }

    // Fallback: route -n get default -ifscope <device>
    QProcess proc2;
    proc2.start("route", {"-n", "get", "default", "-ifscope", device});
    proc2.waitForFinished(3000);

    const QString output2 = proc2.readAllStandardOutput();
    static const QRegularExpression reGateway(R"(gateway:\s+(\d+\.\d+\.\d+\.\d+))");
    QRegularExpressionMatch m = reGateway.match(output2);
    if (m.hasMatch())
        return m.captured(1);

    return QString();
}

QString NetworkInfo::getGatewayMac(const QString &gatewayIp)
{
    QProcess proc;
    proc.start("arp", {"-n", gatewayIp});
    proc.waitForFinished(3000);

    const QString output = proc.readAllStandardOutput();
    static const QRegularExpression re(
        R"(([0-9A-Fa-f]{1,2}:[0-9A-Fa-f]{1,2}:[0-9A-Fa-f]{1,2}:[0-9A-Fa-f]{1,2}:[0-9A-Fa-f]{1,2}:[0-9A-Fa-f]{1,2}))");
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch())
        return m.captured(1).toUpper();

    return QString();
}

QByteArray NetworkInfo::macToBytes(const QString &mac)
{
    const QStringList parts = mac.split(':');
    if (parts.size() != 6)
        return QByteArray();

    QByteArray result;
    result.reserve(6);
    for (const QString &part : parts) {
        bool ok = false;
        const uint val = part.toUInt(&ok, 16);
        if (!ok || val > 0xFF)
            return QByteArray();
        result.append(static_cast<char>(val));
    }
    return result;
}

QByteArray NetworkInfo::ipToBytes(const QString &ip)
{
    const QStringList parts = ip.split('.');
    if (parts.size() != 4)
        return QByteArray();

    QByteArray result;
    result.reserve(4);
    for (const QString &part : parts) {
        bool ok = false;
        const uint val = part.toUInt(&ok, 10);
        if (!ok || val > 255)
            return QByteArray();
        result.append(static_cast<char>(val));
    }
    return result;
}

QString NetworkInfo::getInterfaceMac(const QString &device)
{
    QProcess proc;
    proc.start("ifconfig", {device});
    proc.waitForFinished(3000);

    const QString output = proc.readAllStandardOutput();
    static const QRegularExpression re(R"(\bether\s+([0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5})\b)");
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch())
        return m.captured(1).toUpper();
    return QString();
}

bool NetworkInfo::isBpfAvailable()
{
    const int fd = ::open("/dev/bpf0", O_RDWR);
    if (fd >= 0) {
        ::close(fd);
        return true;
    }
    return false;
}
