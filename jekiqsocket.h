#ifndef JEKIQSOCKET_H
#define JEKIQSOCKET_H

#include <QObject>
#include <QTcpSocket>

#include <botan/types.h>

#include <botan/tls_client.h>
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>

#include "credentials.h"

using namespace Botan;

class JekiQSocket : public QTcpSocket
{
    Q_OBJECT
public:
    JekiQSocket(QObject* parent = nullptr);
    ~JekiQSocket();

    virtual void connectToHost(const QString& hostname, quint16 port);

signals:
    void readyForApp();

private:
    void socketWrite(const byte buf[], size_t length);
    void alertReceived(TLS::Alert alert, const byte buf[], size_t length);
    bool handshakeComplete(const TLS::Session& session);
    void processData(const byte buf[], size_t size);

    TLS::Client* client;
    AutoSeeded_RNG* rng;
    TLS::Policy policy;
    Basic_Credentials_Manager* creds;

    TLS::Session_Manager_In_Memory* sessionManager;

    QString hostname;
    quint16 port;
    int index;

private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReceivedData();
    void onReadyForApp();

};

#endif // JEKIQSOCKET_H
