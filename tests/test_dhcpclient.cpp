#include <QtTest>
#include "dhcpclient.h"

class TestDhcpClient : public QObject {
    Q_OBJECT

private slots:
    void testBuildDhcpDiscoverSize()
    {
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpDiscover(mac, 0x12345678);
        // Minimum: Eth(14) + IP(20) + UDP(8) + DHCP base(236) + magic(4) + options
        QVERIFY(pkt.size() >= 14 + 20 + 8 + 236 + 4);
    }

    void testBuildDhcpDiscoverEtherType()
    {
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpDiscover(mac, 0x12345678);
        // EtherType at bytes [12-13] should be 0x0800 (IPv4)
        QCOMPARE(quint8(pkt[12]), quint8(0x08));
        QCOMPARE(quint8(pkt[13]), quint8(0x00));
    }

    void testBuildDhcpDiscoverBroadcast()
    {
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpDiscover(mac, 0x12345678);
        // First 6 bytes should be FF:FF:FF:FF:FF:FF (broadcast)
        for (int i = 0; i < 6; ++i)
            QCOMPARE(quint8(pkt[i]), quint8(0xFF));
    }

    void testBuildDhcpDiscoverMagicCookie()
    {
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpDiscover(mac, 0x12345678);
        // Magic cookie at offset 42 (Eth+IP+UDP) + 236 (DHCP base)
        int off = 42 + 236;
        quint32 cookie = (quint32(quint8(pkt[off])) << 24) |
                         (quint32(quint8(pkt[off+1])) << 16) |
                         (quint32(quint8(pkt[off+2])) << 8) |
                         quint32(quint8(pkt[off+3]));
        QCOMPARE(cookie, quint32(0x63825363));
    }

    void testBuildDhcpDiscoverMessageType()
    {
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpDiscover(mac, 0x12345678);
        // DHCP options start at offset 42 + 240
        int optStart = 42 + 240;
        // Find option 53 (DHCP Message Type)
        bool found = false;
        int i = optStart;
        while (i < pkt.size()) {
            quint8 code = quint8(pkt[i]);
            if (code == 255) break;
            if (code == 0) { i++; continue; }
            quint8 len = quint8(pkt[i + 1]);
            if (code == 53) {
                QCOMPARE(quint8(pkt[i + 2]), quint8(1)); // Discover
                found = true;
                break;
            }
            i += 2 + len;
        }
        QVERIFY(found);
    }

    void testBuildDhcpRequestMessageType()
    {
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpRequest(mac, 0x12345678, 0xC0A80164, 0xC0A80101);
        int optStart = 42 + 240;
        bool found = false;
        int i = optStart;
        while (i < pkt.size()) {
            quint8 code = quint8(pkt[i]);
            if (code == 255) break;
            if (code == 0) { i++; continue; }
            quint8 len = quint8(pkt[i + 1]);
            if (code == 53) {
                QCOMPARE(quint8(pkt[i + 2]), quint8(3)); // Request
                found = true;
                break;
            }
            i += 2 + len;
        }
        QVERIFY(found);
    }

    void testIpChecksum()
    {
        // Known IP header (from RFC 1071 example-style verification)
        // Simple test: all zeros -> checksum should be 0xFFFF
        char zeros[20] = {};
        quint16 cksum = DhcpClient::ipChecksum(zeros, 20);
        QCOMPARE(cksum, quint16(0xFFFF));

        // Verify checksum of a real packet header is zero when checksum field is correct
        QByteArray mac = QByteArray::fromHex("AABBCCDDEEFF");
        QByteArray pkt = DhcpClient::buildDhcpDiscover(mac, 0xAABBCCDD);
        // IP header starts at offset 14 (after Ethernet)
        quint16 verify = DhcpClient::ipChecksum(pkt.constData() + 14, 20);
        QCOMPARE(verify, quint16(0));
    }

    void testIsActiveDefault()
    {
        DhcpClient client;
        QVERIFY(!client.isActive());
    }
};

QTEST_MAIN(TestDhcpClient)
#include "test_dhcpclient.moc"
