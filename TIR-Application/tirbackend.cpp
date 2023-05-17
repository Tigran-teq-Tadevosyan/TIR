#include "tirbackend.h"

#include <QFile>


extern "C" {
    #include "Network/Ethernet.h"
}
//#include <arpa/inet.h>
//#include <net/if.h>
//#include <net/ethernet.h>
//#include <netinet/if_ether.h>
//#include <sys/ioctl.h>
//#include <qnetworkinterface.h>

//struct sniff_ethernet {
//    u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
//    u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
//    u_short ether_type; /* IP? ARP? RARP? etc */
//};


TIRBackend::TIRBackend(QObject* parent) :
    QObject(parent),
    HOST_MAC_PATH("/sys/class/net/%1/address"),
    sample_line("N %1: %2 sec: %3"),
    timer(this)
{
    connect(&timer, &QTimer::timeout, this, &TIRBackend::dispatch);
}

void TIRBackend::connectDisconnect()
{
    if(m_connected) {
        timer.stop();
        pcap_close(handle);

        m_connected = false;
        emit connectedChanged();
    } else {
        if(m_interface == "") return;
       m_interface.data();
        handle = pcap_open_live(m_interface.toStdString().c_str(), BUFSIZ, 1, 1000, errbuf);

        if (handle == NULL) {
            qDebug()<<"Couldn't open device " << m_interface.toStdString().c_str() <<": "<<errbuf;
            return;
        }

        int nonblock_ret_status = pcap_setnonblock(handle, 1, errbuf);

        if(nonblock_ret_status == PCAP_ERROR) {
            qDebug()<<"Exit with error: "<<errbuf;
            return;
        }

        firstPktTime = 0;
        pktNumber = 0;
        fetchHostMACAddress();

        m_connected = true;
        emit connectedChanged();

        timer.start(1000);
        timer.start(1000);
    }

}

void TIRBackend::dispatch() {
    pcap_dispatch(this->handle, -1, packet_callback, (u_char*)this);
}

void TIRBackend::fetchHostMACAddress() {
    QFile file(this->HOST_MAC_PATH.arg(m_interface.toStdString().c_str()));

    if(!file.open(QIODevice::ReadOnly)) {
        m_hostMAC = "";
        emit hostMACChanged();
        return;
    }
    QTextStream in(&file);

    m_hostMAC = in.readLine();
    emit hostMACChanged();
}

void packet_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    TIRBackend *backend = (TIRBackend*)args;
    if(backend->firstPktTime == 0)
        backend->firstPktTime = header->ts.tv_sec;

    if(header->caplen != header->len) {

        emit backend->appendLine(
                        backend->sample_line.arg(++backend->pktNumber).arg(header->ts.tv_sec - backend->firstPktTime).arg("Received incomplete packet, ignoring.")
                    );
        return;
    }

    EthHeader *ethHeader = (EthHeader*)packet;

    QString etherText = QStringLiteral("Source: %1, Destination: %2, Type: %3.") \
                                        .arg(macAddrToString(&ethHeader->srcAddr, NULL)) \
                                        .arg(macAddrToString(&ethHeader->destAddr, NULL)) \
                                        .arg(ntohs(ethHeader->type));

    emit backend->appendLine(
                    backend->sample_line.arg(++backend->pktNumber)
                                        .arg(header->ts.tv_sec - backend->firstPktTime)
                                        .arg(etherText)
                );


//    const struct sniff_ethernet *ethernet = (struct sniff_ethernet*)(packet);

//    qDebug()<<"-------New Package-------";
//    qDebug()<<"Secs: "<<header->ts.tv_sec;
//    qDebug()<<"Caplen: "<<header->caplen;
//    qDebug()<<"Len: "<<header->len;

//    int addr_str_len = 18;
//    char *dest_addr = (char*)malloc(sizeof(*dest_addr) * addr_str_len);
//    char *source_addr = (char*)malloc(sizeof(*source_addr) * addr_str_len);
//    snprintf(dest_addr, addr_str_len, "%02X:%02X:%02X:%02X:%02X:%02X", ethernet->ether_dhost[0], ethernet->ether_dhost[1], ethernet->ether_dhost[2], ethernet->ether_dhost[3], ethernet->ether_dhost[4], ethernet->ether_dhost[5]);
//    snprintf(source_addr, addr_str_len, "%02X:%02X:%02X:%02X:%02X:%02X", ethernet->ether_shost[0], ethernet->ether_shost[1], ethernet->ether_shost[2], ethernet->ether_shost[3], ethernet->ether_shost[4], ethernet->ether_shost[5]);


//    qDebug()<<"Dest addr: "<<dest_addr;
//    qDebug()<<"Sorce addr: "<<source_addr;
//    qDebug()<<"Ether type: "<<ethernet->ether_type;

}
