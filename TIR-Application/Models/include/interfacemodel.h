#pragma once

#include <QAbstractListModel>
#include <QtQml/qqml.h>

class InterfaceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InterfaceModel)
public:
    enum RoleNames {
        InterfaceName = Qt::DisplayRole,
        InterfacePath = Qt::UserRole + 1
    };

    explicit InterfaceModel(QObject *parent = nullptr);

    Q_INVOKABLE void reset();

public: // interface QAbstractListModel
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

protected: // interface QAbstractListModel
    virtual QHash<int, QByteArray> roleNames() const { return m_roleNames; };

private:
    QHash<int, QByteArray> m_roleNames{
        {InterfaceName, "interfaceName"},
        {InterfacePath, "interfacePath"}
    };

    QList<QString> interfaceNames;
};

