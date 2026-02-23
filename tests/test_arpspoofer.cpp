#include <QtTest>
#include "arpspoofer.h"

class TestArpSpoofer : public QObject {
    Q_OBJECT
private slots:
    void testBuildArpReplySize();
    void testBuildArpReplyEtherType();
    void testBuildArpReplyOpcode();
    void testBuildArpReplyFields();
    void testIsActiveDefault();
    void testActiveDevicesEmpty();
    void testStartNonExistentDevice();
};

void TestArpSpoofer::testBuildArpReplySize()
{
    QByteArray senderMac(6, '\x01');
    QByteArray senderIp(4, '\x0a');
    QByteArray targetMac(6, '\x02');
    QByteArray targetIp(4, '\x0b');

    QByteArray packet = ArpSpoofer::buildArpReply(senderMac, senderIp, targetMac, targetIp);
    QCOMPARE(packet.size(), 42);
}

void TestArpSpoofer::testBuildArpReplyEtherType()
{
    QByteArray senderMac(6, '\x01');
    QByteArray senderIp(4, '\x0a');
    QByteArray targetMac(6, '\x02');
    QByteArray targetIp(4, '\x0b');

    QByteArray packet = ArpSpoofer::buildArpReply(senderMac, senderIp, targetMac, targetIp);
    QCOMPARE(static_cast<uint8_t>(packet[12]), 0x08);
    QCOMPARE(static_cast<uint8_t>(packet[13]), 0x06);
}

void TestArpSpoofer::testBuildArpReplyOpcode()
{
    QByteArray senderMac(6, '\x01');
    QByteArray senderIp(4, '\x0a');
    QByteArray targetMac(6, '\x02');
    QByteArray targetIp(4, '\x0b');

    QByteArray packet = ArpSpoofer::buildArpReply(senderMac, senderIp, targetMac, targetIp);
    // Opcode at bytes [20-21] should be 0x0002 (ARP Reply)
    QCOMPARE(static_cast<uint8_t>(packet[20]), 0x00);
    QCOMPARE(static_cast<uint8_t>(packet[21]), 0x02);
}

void TestArpSpoofer::testBuildArpReplyFields()
{
    // Known sender: MAC aa:bb:cc:dd:ee:ff, IP 192.168.1.100
    QByteArray senderMac = QByteArray::fromHex("aabbccddeeff");
    QByteArray senderIp  = QByteArray::fromHex("c0a80164");  // 192.168.1.100
    // Known target: MAC 11:22:33:44:55:66, IP 192.168.1.1
    QByteArray targetMac = QByteArray::fromHex("112233445566");
    QByteArray targetIp  = QByteArray::fromHex("c0a80101");  // 192.168.1.1

    QByteArray packet = ArpSpoofer::buildArpReply(senderMac, senderIp, targetMac, targetIp);

    // Ethernet dst = target MAC
    QCOMPARE(packet.mid(0, 6).toHex(), QByteArray("112233445566"));
    // Ethernet src = sender MAC
    QCOMPARE(packet.mid(6, 6).toHex(), QByteArray("aabbccddeeff"));

    // ARP hardware type = 0x0001
    QCOMPARE(static_cast<uint8_t>(packet[14]), 0x00);
    QCOMPARE(static_cast<uint8_t>(packet[15]), 0x01);
    // ARP protocol type = 0x0800
    QCOMPARE(static_cast<uint8_t>(packet[16]), 0x08);
    QCOMPARE(static_cast<uint8_t>(packet[17]), 0x00);
    // Hardware size = 6, Protocol size = 4
    QCOMPARE(static_cast<uint8_t>(packet[18]), 0x06);
    QCOMPARE(static_cast<uint8_t>(packet[19]), 0x04);

    // ARP sender MAC at offset 22
    QCOMPARE(packet.mid(22, 6).toHex(), QByteArray("aabbccddeeff"));
    // ARP sender IP at offset 28
    QCOMPARE(packet.mid(28, 4).toHex(), QByteArray("c0a80164"));
    // ARP target MAC at offset 32
    QCOMPARE(packet.mid(32, 6).toHex(), QByteArray("112233445566"));
    // ARP target IP at offset 38
    QCOMPARE(packet.mid(38, 4).toHex(), QByteArray("c0a80101"));
}

void TestArpSpoofer::testIsActiveDefault()
{
    ArpSpoofer spoofer;
    QVERIFY(!spoofer.isActive("en0"));
    QVERIFY(!spoofer.isActive("en1"));
    QVERIFY(!spoofer.isActive(""));
}

void TestArpSpoofer::testActiveDevicesEmpty()
{
    ArpSpoofer spoofer;
    QVERIFY(spoofer.activeDevices().isEmpty());
}

void TestArpSpoofer::testStartNonExistentDevice()
{
    ArpSpoofer spoofer;
    QSignalSpy errorSpy(&spoofer, &ArpSpoofer::error);

    bool result = spoofer.start("nonexistent_device_xyz", "aa:bb:cc:dd:ee:ff");
    QVERIFY(!result);
    QVERIFY(!spoofer.isActive("nonexistent_device_xyz"));
    QVERIFY(errorSpy.count() > 0);
}

QTEST_MAIN(TestArpSpoofer)
#include "test_arpspoofer.moc"
