#include <QtTest>
#include "networkinfo.h"

class TestNetworkInfo : public QObject {
    Q_OBJECT

private slots:
    void testMacToBytes()
    {
        const QByteArray bytes = NetworkInfo::macToBytes("AA:BB:CC:DD:EE:FF");
        QCOMPARE(bytes.size(), 6);
        QCOMPARE(static_cast<quint8>(bytes[0]), quint8(0xAA));
        QCOMPARE(static_cast<quint8>(bytes[1]), quint8(0xBB));
        QCOMPARE(static_cast<quint8>(bytes[2]), quint8(0xCC));
        QCOMPARE(static_cast<quint8>(bytes[3]), quint8(0xDD));
        QCOMPARE(static_cast<quint8>(bytes[4]), quint8(0xEE));
        QCOMPARE(static_cast<quint8>(bytes[5]), quint8(0xFF));
    }

    void testMacToBytesInvalid()
    {
        QVERIFY(NetworkInfo::macToBytes("invalid").isEmpty());
        QVERIFY(NetworkInfo::macToBytes("AA:BB:CC:DD:EE").isEmpty());
        QVERIFY(NetworkInfo::macToBytes("").isEmpty());
        QVERIFY(NetworkInfo::macToBytes("GG:HH:II:JJ:KK:LL").isEmpty());
    }

    void testIpToBytes()
    {
        const QByteArray bytes = NetworkInfo::ipToBytes("192.168.1.1");
        QCOMPARE(bytes.size(), 4);
        QCOMPARE(static_cast<quint8>(bytes[0]), quint8(0xC0));
        QCOMPARE(static_cast<quint8>(bytes[1]), quint8(0xA8));
        QCOMPARE(static_cast<quint8>(bytes[2]), quint8(0x01));
        QCOMPARE(static_cast<quint8>(bytes[3]), quint8(0x01));
    }

    void testIpToBytesInvalid()
    {
        QVERIFY(NetworkInfo::ipToBytes("invalid").isEmpty());
        QVERIFY(NetworkInfo::ipToBytes("192.168.1").isEmpty());
        QVERIFY(NetworkInfo::ipToBytes("").isEmpty());
        QVERIFY(NetworkInfo::ipToBytes("256.0.0.1").isEmpty());
    }

    void testGetInterfaceIp()
    {
        const QString ip = NetworkInfo::getInterfaceIp("en0");
        // May be empty if en0 is not connected, but if non-empty must look like an IP
        if (!ip.isEmpty()) {
            QVERIFY(ip.contains('.'));
            QVERIFY(!ip.startsWith("127."));
        }
    }

    void testGetGatewayIp()
    {
        const QString gw = NetworkInfo::getGatewayIp("en0");
        if (!gw.isEmpty()) {
            QVERIFY(gw.contains('.'));
            const QStringList parts = gw.split('.');
            QCOMPARE(parts.size(), 4);
        }
    }

    void testGetGatewayMac()
    {
        const QString gwIp = NetworkInfo::getGatewayIp("en0");
        if (gwIp.isEmpty())
            QSKIP("No gateway found, skipping MAC lookup");

        const QString gwMac = NetworkInfo::getGatewayMac(gwIp);
        if (!gwMac.isEmpty()) {
            // Verify format XX:XX:XX:XX:XX:XX
            const QStringList parts = gwMac.split(':');
            QCOMPARE(parts.size(), 6);
            for (const QString &part : parts) {
                QVERIFY(part.length() <= 2);
                bool ok = false;
                part.toUInt(&ok, 16);
                QVERIFY(ok);
            }
        }
    }

    void testIsBpfAvailable()
    {
        // Just verify it returns without crashing; result depends on permissions
        const bool result = NetworkInfo::isBpfAvailable();
        Q_UNUSED(result)
        QVERIFY(true);
    }
};

QTEST_MAIN(TestNetworkInfo)
#include "test_networkinfo.moc"
