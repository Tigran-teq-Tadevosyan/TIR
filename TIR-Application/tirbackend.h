#pragma once

#include <pcap/pcap.h>

#include <QtQml/qqml.h>
#include <QTimer>
#include <QDateTime>

void packet_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

class TIRBackend : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TIRBackend)
    QML_SINGLETON

    Q_PROPERTY(QString interface MEMBER m_interface NOTIFY interfaceChanged)
    Q_PROPERTY(bool connected MEMBER m_connected NOTIFY connectedChanged)
    Q_PROPERTY(QString hostMAC MEMBER m_hostMAC  NOTIFY hostMACChanged)

public:
    TIRBackend(QObject* parent = nullptr);

    Q_INVOKABLE void connectDisconnect();

signals:
    void interfaceChanged();
    void connectedChanged();
    void hostMACChanged();

    void appendLine(QString str);

public slots:
    void dispatch();

private:
    const QString HOST_MAC_PATH;
    __time_t firstPktTime = 0;
    long pktNumber = 0;
    QString sample_line;

    QString m_interface = "";
    QString m_hostMAC = "";
    bool m_connected = false;

    QTimer timer;

    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    void fetchHostMACAddress();

    friend void packet_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
};

