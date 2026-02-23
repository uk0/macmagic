#pragma once
#include <QObject>
#include <QMap>
#include <QTimer>
#include <QByteArray>
#include <pcap/pcap.h>

class ArpSpoofer : public QObject {
    Q_OBJECT
public:
    explicit ArpSpoofer(QObject *parent = nullptr);
    ~ArpSpoofer() override;

    bool start(const QString &device, const QString &spoofedMac);
    void stop(const QString &device, bool sendCorrective = true);
    void stopAll(bool sendCorrective = true);
    bool isActive(const QString &device) const;
    QStringList activeDevices() const;
    QString spoofedMac(const QString &device) const;

    // Public static: pure function that builds a raw ARP reply packet (42 bytes)
    static QByteArray buildArpReply(const QByteArray &senderMac, const QByteArray &senderIp,
                                     const QByteArray &targetMac, const QByteArray &targetIp);

signals:
    void started(const QString &device);
    void stopped(const QString &device);
    void error(const QString &device, const QString &message);

private:
    struct SpoofSession {
        pcap_t *handle = nullptr;
        QTimer *timer = nullptr;
        QString device;
        QString spoofedMac;
        QString realMac;
        QString ourIp;
        QString gatewayIp;
        QString gatewayMac;
    };

    bool sendPacket(pcap_t *handle, const QByteArray &packet);
    void sendSpoofPacket(const SpoofSession &session);
    void sendCorrectivePacket(const SpoofSession &session);

    QMap<QString, SpoofSession> m_sessions;
};
