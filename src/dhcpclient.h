#pragma once
#include <QObject>
#include <QByteArray>
#include <pcap/pcap.h>

class QSocketNotifier;

class DhcpClient : public QObject {
    Q_OBJECT
public:
    explicit DhcpClient(QObject *parent = nullptr);
    ~DhcpClient() override;

    bool requestIp(const QString &device, const QString &spoofedMac);
    void cancel();
    bool isActive() const;

    // Public static for testability (pure functions)
    static QByteArray buildDhcpDiscover(const QByteArray &clientMac, quint32 xid);
    static QByteArray buildDhcpRequest(const QByteArray &clientMac, quint32 xid,
                                       quint32 offeredIp, quint32 serverIp);
    static quint16 ipChecksum(const char *data, int len);

signals:
    void ipObtained(const QString &device, const QString &ip);
    void error(const QString &device, const QString &message);

private slots:
    void onPacketReady();

private:
    enum class State { Idle, WaitingOffer, WaitingAck };

    static quint16 udpChecksum(const char *ipHeader, const char *udpPacket, int udpLen);
    bool parseDhcpResponse(const QByteArray &packet, quint8 &msgType,
                           quint32 &offeredIp, quint32 &serverIp);
    void cleanup();

    pcap_t *m_handle = nullptr;
    QSocketNotifier *m_notifier = nullptr;
    State m_state = State::Idle;
    QString m_device;
    QByteArray m_clientMac;
    quint32 m_xid = 0;
    quint32 m_offeredIp = 0;
    quint32 m_serverIp = 0;
};
