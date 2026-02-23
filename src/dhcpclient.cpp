#include "dhcpclient.h"
#include <QSocketNotifier>
#include <QRandomGenerator>
#include <QHostAddress>
#include <cstring>

namespace {
constexpr int ETH_HLEN = 14;
constexpr int IP_HLEN = 20;
constexpr int UDP_HLEN = 8;
constexpr int DHCP_BASE = 236;
constexpr int DHCP_MAGIC_LEN = 4;
constexpr int DHCP_OFFSET = ETH_HLEN + IP_HLEN + UDP_HLEN;
constexpr quint8 IP_PROTO_UDP = 17;
constexpr quint16 DHCP_SERVER_PORT = 67;
constexpr quint16 DHCP_CLIENT_PORT = 68;
constexpr quint32 DHCP_MAGIC_COOKIE = 0x63825363;

void appendU16(QByteArray &buf, quint16 val)
{
    buf.append(static_cast<char>((val >> 8) & 0xFF));
    buf.append(static_cast<char>(val & 0xFF));
}

void appendU32(QByteArray &buf, quint32 val)
{
    buf.append(static_cast<char>((val >> 24) & 0xFF));
    buf.append(static_cast<char>((val >> 16) & 0xFF));
    buf.append(static_cast<char>((val >> 8) & 0xFF));
    buf.append(static_cast<char>(val & 0xFF));
}

QByteArray buildDhcpPayload(const QByteArray &clientMac, quint32 xid,
                             const QByteArray &options)
{
    QByteArray dhcp;
    dhcp.reserve(DHCP_BASE + DHCP_MAGIC_LEN + options.size());
    dhcp.append(char(1));  // op: BOOTREQUEST
    dhcp.append(char(1));  // htype: Ethernet
    dhcp.append(char(6));  // hlen
    dhcp.append(char(0));  // hops
    appendU32(dhcp, xid);
    appendU16(dhcp, 0);        // secs
    appendU16(dhcp, 0x8000);   // flags: broadcast
    appendU32(dhcp, 0);        // ciaddr
    appendU32(dhcp, 0);        // yiaddr
    appendU32(dhcp, 0);        // siaddr
    appendU32(dhcp, 0);        // giaddr
    dhcp.append(clientMac.left(6));
    dhcp.append(QByteArray(10, '\0'));  // chaddr padding
    dhcp.append(QByteArray(64 + 128, '\0')); // sname + file
    appendU32(dhcp, DHCP_MAGIC_COOKIE);
    dhcp.append(options);
    return dhcp;
}

QByteArray wrapInUdpIpEth(const QByteArray &clientMac, const QByteArray &dhcpPayload)
{
    const int dhcpLen = dhcpPayload.size();
    const int udpLen = UDP_HLEN + dhcpLen;
    const int ipTotalLen = IP_HLEN + udpLen;

    // --- Ethernet header (14 bytes) ---
    QByteArray pkt;
    pkt.reserve(ETH_HLEN + ipTotalLen);
    pkt.append(QByteArray(6, '\xFF'));           // dst: broadcast
    pkt.append(clientMac.left(6));               // src: spoofed MAC
    appendU16(pkt, 0x0800);                      // EtherType: IPv4

    // --- IP header (20 bytes) ---
    int ipStart = pkt.size();
    pkt.append(char(0x45));                      // version + IHL
    pkt.append(char(0x00));                      // TOS
    appendU16(pkt, static_cast<quint16>(ipTotalLen));
    appendU16(pkt, static_cast<quint16>(QRandomGenerator::global()->generate() & 0xFFFF)); // ID
    appendU16(pkt, 0);                           // flags + fragment
    pkt.append(char(128));                       // TTL
    pkt.append(char(IP_PROTO_UDP));              // protocol
    appendU16(pkt, 0);                           // checksum placeholder
    appendU32(pkt, 0x00000000);                  // src IP: 0.0.0.0
    appendU32(pkt, 0xFFFFFFFF);                  // dst IP: 255.255.255.255

    // Calculate and fill IP checksum
    quint16 cksum = DhcpClient::ipChecksum(pkt.constData() + ipStart, IP_HLEN);
    pkt[ipStart + 10] = static_cast<char>((cksum >> 8) & 0xFF);
    pkt[ipStart + 11] = static_cast<char>(cksum & 0xFF);

    // --- UDP header (8 bytes) ---
    appendU16(pkt, DHCP_CLIENT_PORT);            // src port: 68
    appendU16(pkt, DHCP_SERVER_PORT);            // dst port: 67
    appendU16(pkt, static_cast<quint16>(udpLen));
    appendU16(pkt, 0);                           // UDP checksum: 0 (optional for IPv4)

    // --- DHCP payload ---
    pkt.append(dhcpPayload);

    return pkt;
}
} // namespace

// --- DhcpClient implementation ---

DhcpClient::DhcpClient(QObject *parent)
    : QObject(parent)
{
}

DhcpClient::~DhcpClient()
{
    cleanup();
}

bool DhcpClient::isActive() const
{
    return m_state != State::Idle;
}

void DhcpClient::cancel()
{
    if (m_state != State::Idle)
        cleanup();
}

bool DhcpClient::requestIp(const QString &device, const QString &spoofedMac)
{
    if (m_state != State::Idle) {
        emit error(device, "DHCP request already in progress");
        return false;
    }

    m_clientMac.clear();
    const QStringList parts = spoofedMac.split(':');
    if (parts.size() != 6) {
        emit error(device, "Invalid MAC address format");
        return false;
    }
    for (const QString &p : parts) {
        bool ok = false;
        m_clientMac.append(static_cast<char>(p.toUInt(&ok, 16)));
        if (!ok) {
            emit error(device, "Invalid MAC address hex value");
            return false;
        }
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    m_handle = pcap_open_live(device.toUtf8().constData(), 65535, 0, 1000, errbuf);
    if (!m_handle) {
        emit error(device, QString("pcap_open_live failed: %1").arg(errbuf));
        return false;
    }

    struct bpf_program fp;
    const char *filter = "udp src port 67 and udp dst port 68";
    if (pcap_compile(m_handle, &fp, filter, 1, PCAP_NETMASK_UNKNOWN) == -1 ||
        pcap_setfilter(m_handle, &fp) == -1) {
        pcap_freecode(&fp);
        pcap_close(m_handle);
        m_handle = nullptr;
        emit error(device, "Failed to set BPF filter");
        return false;
    }
    pcap_freecode(&fp);

    int fd = pcap_get_selectable_fd(m_handle);
    if (fd < 0) {
        pcap_close(m_handle);
        m_handle = nullptr;
        emit error(device, "pcap_get_selectable_fd failed");
        return false;
    }

    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &DhcpClient::onPacketReady);

    m_device = device;
    m_xid = QRandomGenerator::global()->generate();

    QByteArray discover = buildDhcpDiscover(m_clientMac, m_xid);
    if (pcap_sendpacket(m_handle, reinterpret_cast<const u_char *>(discover.constData()),
                        discover.size()) != 0) {
        QString err = QString("Failed to send DHCP Discover: %1").arg(pcap_geterr(m_handle));
        cleanup();
        emit error(device, err);
        return false;
    }

    m_state = State::WaitingOffer;
    return true;
}

void DhcpClient::onPacketReady()
{
    struct pcap_pkthdr *header;
    const u_char *data;
    int res = pcap_next_ex(m_handle, &header, &data);
    if (res <= 0)
        return;

    QByteArray packet(reinterpret_cast<const char *>(data), static_cast<int>(header->caplen));
    quint8 msgType = 0;
    quint32 offeredIp = 0;
    quint32 serverIp = 0;

    if (!parseDhcpResponse(packet, msgType, offeredIp, serverIp))
        return;

    if (m_state == State::WaitingOffer && msgType == 2) {
        m_offeredIp = offeredIp;
        m_serverIp = serverIp;
        QByteArray request = buildDhcpRequest(m_clientMac, m_xid, m_offeredIp, m_serverIp);
        if (pcap_sendpacket(m_handle, reinterpret_cast<const u_char *>(request.constData()),
                            request.size()) != 0) {
            emit error(m_device, "Failed to send DHCP Request");
            cleanup();
            return;
        }
        m_state = State::WaitingAck;
    } else if (m_state == State::WaitingAck && msgType == 5) {
        QHostAddress addr(offeredIp);
        QString ip = addr.toString();
        QString dev = m_device;
        cleanup();
        emit ipObtained(dev, ip);
    }
}

bool DhcpClient::parseDhcpResponse(const QByteArray &packet, quint8 &msgType,
                                    quint32 &offeredIp, quint32 &serverIp)
{
    const int minLen = DHCP_OFFSET + DHCP_BASE + DHCP_MAGIC_LEN + 1;
    if (packet.size() < minLen)
        return false;

    const auto *raw = reinterpret_cast<const quint8 *>(packet.constData());

    quint32 cookie = (quint32(raw[DHCP_OFFSET + 236]) << 24) |
                     (quint32(raw[DHCP_OFFSET + 237]) << 16) |
                     (quint32(raw[DHCP_OFFSET + 238]) << 8) |
                     quint32(raw[DHCP_OFFSET + 239]);
    if (cookie != DHCP_MAGIC_COOKIE)
        return false;

    quint32 pktXid = (quint32(raw[DHCP_OFFSET + 4]) << 24) |
                     (quint32(raw[DHCP_OFFSET + 5]) << 16) |
                     (quint32(raw[DHCP_OFFSET + 6]) << 8) |
                     quint32(raw[DHCP_OFFSET + 7]);
    if (pktXid != m_xid)
        return false;

    offeredIp = (quint32(raw[DHCP_OFFSET + 16]) << 24) |
                (quint32(raw[DHCP_OFFSET + 17]) << 16) |
                (quint32(raw[DHCP_OFFSET + 18]) << 8) |
                quint32(raw[DHCP_OFFSET + 19]);

    int optOffset = DHCP_OFFSET + DHCP_BASE + DHCP_MAGIC_LEN;
    msgType = 0;
    serverIp = 0;

    while (optOffset < packet.size()) {
        quint8 optCode = raw[optOffset];
        if (optCode == 255) break;
        if (optCode == 0) { optOffset++; continue; }
        if (optOffset + 1 >= packet.size()) break;
        quint8 optLen = raw[optOffset + 1];
        if (optOffset + 2 + optLen > packet.size()) break;
        if (optCode == 53 && optLen >= 1)
            msgType = raw[optOffset + 2];
        else if (optCode == 54 && optLen >= 4)
            serverIp = (quint32(raw[optOffset + 2]) << 24) |
                       (quint32(raw[optOffset + 3]) << 16) |
                       (quint32(raw[optOffset + 4]) << 8) |
                       quint32(raw[optOffset + 5]);
        optOffset += 2 + optLen;
    }
    return msgType != 0;
}

quint16 DhcpClient::ipChecksum(const char *data, int len)
{
    quint32 sum = 0;
    const auto *ptr = reinterpret_cast<const quint8 *>(data);
    while (len > 1) {
        sum += (quint16(ptr[0]) << 8) | ptr[1];
        ptr += 2;
        len -= 2;
    }
    if (len == 1)
        sum += quint16(ptr[0]) << 8;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<quint16>(~sum & 0xFFFF);
}

QByteArray DhcpClient::buildDhcpDiscover(const QByteArray &clientMac, quint32 xid)
{
    QByteArray opts;
    opts.append(char(53)); opts.append(char(1)); opts.append(char(1));
    opts.append(char(55)); opts.append(char(4));
    opts.append(char(1)); opts.append(char(3)); opts.append(char(6)); opts.append(char(15));
    opts.append(char(255));
    return wrapInUdpIpEth(clientMac, buildDhcpPayload(clientMac, xid, opts));
}

QByteArray DhcpClient::buildDhcpRequest(const QByteArray &clientMac, quint32 xid,
                                         quint32 offeredIp, quint32 serverIp)
{
    QByteArray opts;
    opts.append(char(53)); opts.append(char(1)); opts.append(char(3));
    opts.append(char(50)); opts.append(char(4));
    appendU32(opts, offeredIp);
    opts.append(char(54)); opts.append(char(4));
    appendU32(opts, serverIp);
    opts.append(char(55)); opts.append(char(4));
    opts.append(char(1)); opts.append(char(3)); opts.append(char(6)); opts.append(char(15));
    opts.append(char(255));
    return wrapInUdpIpEth(clientMac, buildDhcpPayload(clientMac, xid, opts));
}

void DhcpClient::cleanup()
{
    delete m_notifier;
    m_notifier = nullptr;
    if (m_handle) {
        pcap_close(m_handle);
        m_handle = nullptr;
    }
    m_state = State::Idle;
    m_device.clear();
    m_clientMac.clear();
    m_xid = 0;
    m_offeredIp = 0;
    m_serverIp = 0;
}
