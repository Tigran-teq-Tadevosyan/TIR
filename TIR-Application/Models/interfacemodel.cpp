#include "interfacemodel.h"

#include <pcap.h>

InterfaceModel::InterfaceModel(QObject *parent) :
    QAbstractListModel(parent)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevsp = nullptr;
    int status = pcap_findalldevs(&alldevsp, errbuf);
    pcap_if_t *device;

    if (status != 0 || alldevsp == NULL) {
        qDebug()<<"Couldn't find default device: "<<errbuf;
        return;
    }

    device = alldevsp; // We set the first device (default capture device)

    while(device != NULL && (device->flags & PCAP_IF_RUNNING)) {
        interfaceNames.append(device->name);
        device = device->next;
    }

    pcap_freealldevs(alldevsp);
}

int InterfaceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return interfaceNames.count();
}

QVariant InterfaceModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if(row < 0 || row >= interfaceNames.count()) {
        return QVariant();
    }

    switch(role) {
        case InterfaceName:
            return interfaceNames.at(row);
        case InterfacePath:
            return interfaceNames.at(row);
    }

    return QVariant();
}

void InterfaceModel::reset(){
    beginRemoveRows(QModelIndex(), 0, interfaceNames.count() - 1);
    endRemoveRows();

    interfaceNames.clear();

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevsp = nullptr;
    int status = pcap_findalldevs(&alldevsp, errbuf);
    pcap_if_t *device;

    if (status != 0 || alldevsp == NULL) {
        qDebug()<<"Couldn't find default device: "<<errbuf;
        return;
    }

    device = alldevsp; // We set the first device (default capture device)

    while(device != NULL && (device->flags & PCAP_IF_RUNNING)) {
        interfaceNames.append(device->name);
        device = device->next;
    }

    pcap_freealldevs(alldevsp);

    beginInsertRows(QModelIndex(), 0, interfaceNames.count() - 1);
    endInsertRows();
}
