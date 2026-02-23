#include <QtTest>
#include "macgenerator.h"

class TestMACGenerator : public QObject {
    Q_OBJECT

private slots:
    void testGenerateRandom()
    {
        const QString mac = MACGenerator::generateRandom();
        QVERIFY(MACGenerator::isValid(mac));

        // Check locally administered bit is set and multicast bit is clear
        bool ok = false;
        uint firstOctet = mac.left(2).toUInt(&ok, 16);
        QVERIFY(ok);
        QVERIFY(firstOctet & 0x02);       // locally administered
        QVERIFY(!(firstOctet & 0x01));     // unicast
    }

    void testGenerateRandomUniqueness()
    {
        QSet<QString> macs;
        for (int i = 0; i < 100; ++i)
            macs.insert(MACGenerator::generateRandom());
        QVERIFY(macs.size() > 90); // should be nearly all unique
    }

    void testIsValid()
    {
        QVERIFY(MACGenerator::isValid("AA:BB:CC:DD:EE:FF"));
        QVERIFY(MACGenerator::isValid("aa:bb:cc:dd:ee:ff"));
        QVERIFY(MACGenerator::isValid("AA-BB-CC-DD-EE-FF"));
        QVERIFY(!MACGenerator::isValid("AA:BB:CC:DD:EE"));
        QVERIFY(!MACGenerator::isValid("not a mac"));
        QVERIFY(!MACGenerator::isValid("GG:HH:II:JJ:KK:LL"));
        QVERIFY(!MACGenerator::isValid(""));
    }

    void testNormalize()
    {
        QCOMPARE(MACGenerator::normalize("aa:bb:cc:dd:ee:ff"), "AA:BB:CC:DD:EE:FF");
        QCOMPARE(MACGenerator::normalize("AA-BB-CC-DD-EE-FF"), "AA:BB:CC:DD:EE:FF");
        QCOMPARE(MACGenerator::normalize("  aa:bb:cc:dd:ee:ff  "), "AA:BB:CC:DD:EE:FF");
    }

    void testFormatMac()
    {
        QByteArray bytes;
        bytes.append('\xAA');
        bytes.append('\xBB');
        bytes.append('\xCC');
        bytes.append('\x00');
        bytes.append('\x11');
        bytes.append('\xFF');
        QCOMPARE(MACGenerator::formatMac(bytes), "AA:BB:CC:00:11:FF");
    }

    void testGenerateWithPrefix()
    {
        const QString mac = MACGenerator::generateWithPrefix("AA:BB:CC");
        QVERIFY(MACGenerator::isValid(mac));
        QVERIFY(mac.startsWith("AA:BB:CC:"));
    }
};

QTEST_MAIN(TestMACGenerator)
#include "test_macgenerator.moc"
