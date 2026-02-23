#include <QtTest>
#include "networkinterface.h"

class TestNetworkInterface : public QObject {
    Q_OBJECT

private slots:
    void testListInterfaces()
    {
        NetworkInterface ni;
        const auto interfaces = ni.listInterfaces();
        QVERIFY(!interfaces.isEmpty());

        // Should find at least Wi-Fi (en0)
        bool foundWifi = false;
        for (const auto &iface : interfaces) {
            QVERIFY(!iface.device.isEmpty());
            QVERIFY(!iface.currentMac.isEmpty());
            QVERIFY(!iface.originalMac.isEmpty());
            if (iface.hardwarePort.contains("Wi-Fi"))
                foundWifi = true;
        }
        QVERIFY(foundWifi);
    }

    void testGetCurrentMac()
    {
        NetworkInterface ni;
        const QString mac = ni.getCurrentMac("en0");
        QVERIFY(!mac.isEmpty());
        // Should be in format xx:xx:xx:xx:xx:xx
        QCOMPARE(mac.count(':'), 5);
    }

    void testGetOriginalMac()
    {
        NetworkInterface ni;
        const auto interfaces = ni.listInterfaces();
        if (!interfaces.isEmpty()) {
            const QString orig = ni.getOriginalMac(interfaces.first().device);
            QVERIFY(!orig.isEmpty());
        }
    }

    void testRefreshPreservesOriginal()
    {
        NetworkInterface ni;
        const auto before = ni.listInterfaces();
        ni.refresh();
        const auto after = ni.listInterfaces();

        for (const auto &a : after) {
            for (const auto &b : before) {
                if (a.device == b.device) {
                    QCOMPARE(a.originalMac, b.originalMac);
                }
            }
        }
    }

    void testInitialNotModified()
    {
        NetworkInterface ni;
        for (const auto &iface : ni.listInterfaces()) {
            QVERIFY(!iface.isModified());
        }
    }
};

QTEST_MAIN(TestNetworkInterface)
#include "test_networkinterface.moc"
