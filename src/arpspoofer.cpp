#include "arpspoofer.h"
#include "networkinfo.h"

#include <QStringList>
#include <cstring>

ArpSpoofer::ArpSpoofer(QObject *parent)
    : QObject(parent)
{
}

ArpSpoofer::~ArpSpoofer()
{
    stopAll(false);
}

QByteArray ArpSpoofer::buildArpReply(const QByteArray &senderMac, const QByteArray &senderIp,
                                      const QByteArray &targetMac, const QByteArray &targetIp)
{
    QByteArray packet(42, 0);

    // Ethernet header (14 bytes)
    std::memcpy(packet.data(), targetMac.constData(), 6);       // dst MAC
    std::memcpy(packet.data() + 6, senderMac.constData(), 6);  // src MAC
    packet[12] = 0x08;
    packet[13] = 0x06;  // EtherType = ARP

    // ARP header (28 bytes)
    packet[14] = 0x00; packet[15] = 0x01;  // Hardware type = Ethernet
    packet[16] = 0x08; packet[17] = 0x00;  // Protocol type = IPv4
    packet[18] = 0x06;                      // Hardware size
    packet[19] = 0x04;                      // Protocol size
    packet[20] = 0x00; packet[21] = 0x02;  // Opcode = Reply

    std::memcpy(packet.data() + 22, senderMac.constData(), 6);  // Sender MAC
    std::memcpy(packet.data() + 28, senderIp.constData(), 4);   // Sender IP
    std::memcpy(packet.data() + 32, targetMac.constData(), 6);  // Target MAC
    std::memcpy(packet.data() + 38, targetIp.constData(), 4);   // Target IP

    return packet;
}

bool ArpSpoofer::sendPacket(pcap_t *handle, const QByteArray &packet)
{
    if (!handle)
        return false;
    return pcap_inject(handle, packet.constData(), static_cast<size_t>(packet.size())) == packet.size();
}

void ArpSpoofer::sendSpoofPacket(const SpoofSession &session)
{
    const QByteArray senderMac = NetworkInfo::macToBytes(session.spoofedMac);
    const QByteArray senderIp  = NetworkInfo::ipToBytes(session.ourIp);
    const QByteArray targetMac = NetworkInfo::macToBytes(session.gatewayMac);
    const QByteArray targetIp  = NetworkInfo::ipToBytes(session.gatewayIp);

    const QByteArray packet = buildArpReply(senderMac, senderIp, targetMac, targetIp);
    if (!sendPacket(session.handle, packet))
        emit error(session.device, QStringLiteral("Failed to send ARP spoof packet"));
}

void ArpSpoofer::sendCorrectivePacket(const SpoofSession &session)
{
    const QByteArray senderMac = NetworkInfo::macToBytes(session.realMac);
    const QByteArray senderIp  = NetworkInfo::ipToBytes(session.ourIp);
    const QByteArray targetMac = NetworkInfo::macToBytes(session.gatewayMac);
    const QByteArray targetIp  = NetworkInfo::ipToBytes(session.gatewayIp);

    const QByteArray packet = buildArpReply(senderMac, senderIp, targetMac, targetIp);
    sendPacket(session.handle, packet);
}

bool ArpSpoofer::start(const QString &device, const QString &spoofedMac)
{
    if (m_sessions.contains(device)) {
        emit error(device, QStringLiteral("Spoofing already active on this device"));
        return false;
    }

    const QString ourIp = NetworkInfo::getInterfaceIp(device);
    if (ourIp.isEmpty()) {
        emit error(device, QStringLiteral("Could not determine interface IP"));
        return false;
    }

    const QString gatewayIp = NetworkInfo::getGatewayIp(device);
    if (gatewayIp.isEmpty()) {
        emit error(device, QStringLiteral("Could not determine gateway IP"));
        return false;
    }

    const QString gatewayMac = NetworkInfo::getGatewayMac(gatewayIp);
    if (gatewayMac.isEmpty()) {
        emit error(device, QStringLiteral("Could not determine gateway MAC"));
        return false;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(device.toUtf8().constData(), 65535, 0, 100, errbuf);
    if (!handle) {
        emit error(device, QStringLiteral("pcap_open_live failed: %1").arg(QString::fromUtf8(errbuf)));
        return false;
    }

    SpoofSession session;
    session.handle     = handle;
    session.device     = device;
    session.spoofedMac = spoofedMac;
    session.realMac    = NetworkInfo::getInterfaceMac(device);
    session.ourIp      = ourIp;
    session.gatewayIp  = gatewayIp;
    session.gatewayMac = gatewayMac;

    auto *timer = new QTimer(this);
    session.timer = timer;
    m_sessions.insert(device, session);

    connect(timer, &QTimer::timeout, this, [this, device]() {
        auto it = m_sessions.find(device);
        if (it != m_sessions.end())
            sendSpoofPacket(it.value());
    });

    // Send first packet immediately, then every 5 seconds
    sendSpoofPacket(session);
    timer->start(5000);

    emit started(device);
    return true;
}

void ArpSpoofer::stop(const QString &device, bool sendCorrective)
{
    auto it = m_sessions.find(device);
    if (it == m_sessions.end())
        return;

    SpoofSession &session = it.value();

    if (session.timer) {
        session.timer->stop();
        delete session.timer;
        session.timer = nullptr;
    }

    if (sendCorrective)
        sendCorrectivePacket(session);

    if (session.handle) {
        pcap_close(session.handle);
        session.handle = nullptr;
    }

    m_sessions.erase(it);
    emit stopped(device);
}

void ArpSpoofer::stopAll(bool sendCorrective)
{
    const QStringList devices = m_sessions.keys();
    for (const QString &device : devices)
        stop(device, sendCorrective);
}

bool ArpSpoofer::isActive(const QString &device) const
{
    return m_sessions.contains(device);
}

QStringList ArpSpoofer::activeDevices() const
{
    return m_sessions.keys();
}

QString ArpSpoofer::spoofedMac(const QString &device) const
{
    auto it = m_sessions.find(device);
    if (it != m_sessions.end())
        return it.value().spoofedMac;
    return QString();
}
