#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>

class QUdpSocket;

class Network : public QObject
{
    Q_OBJECT
public:
    explicit Network(QObject *parent = nullptr);

signals:

};

#endif // NETWORK_H
