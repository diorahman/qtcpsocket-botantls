#include "jekiqsocket.h"
#include <functional>

#include <QDebug>
#include <QTimer>

using namespace std::placeholders;

using namespace Botan;

JekiQSocket::JekiQSocket(QObject* parent)
    : QTcpSocket{parent}, index{0}
{
    void (QAbstractSocket:: *signature)(QAbstractSocket::SocketError) = &QAbstractSocket::error;
    connect(this, &JekiQSocket::connected, this, &JekiQSocket::onConnected);
    connect(this, signature, this, &JekiQSocket::onError);
    connect(this, &JekiQSocket::readyForApp, this, &JekiQSocket::onReadyForApp);
    connect(this, &JekiQSocket::readyRead, this, &JekiQSocket::onReceivedData);

    rng = new AutoSeeded_RNG();
    sessionManager = new TLS::Session_Manager_In_Memory(*rng);
    creds = new Basic_Credentials_Manager();
}


JekiQSocket::~JekiQSocket()
{
    qDebug() << "Delete";
    if (this->client) {
        this->client->close();
        delete this->client;
    }

    if (this->sessionManager) {
        delete sessionManager;
        delete rng;
        delete creds;
    }
}

void JekiQSocket::connectToHost(const QString &hostname, quint16 port)
{
    this->hostname = hostname;
    this->port = port;

    QTcpSocket::connectToHost(hostname, port);
}

void JekiQSocket::socketWrite(const byte buf[], size_t length)
{
    this->write(reinterpret_cast<const char*>(buf), length);
}

void JekiQSocket::alertReceived(TLS::Alert alert, const byte buf[], size_t length)
{
    Q_UNUSED(alert);
    Q_UNUSED(buf);
    Q_UNUSED(length);
    qDebug() << "Alert";
}

bool JekiQSocket::handshakeComplete(const TLS::Session &session)
{
    std::cout << "Handshake complete, " << session.version().to_string()
              << " using " << session.ciphersuite().to_string() << std::endl;

    if(!session.session_id().empty()) {
        std::cout << "Session ID " << hex_encode(session.session_id()) << std::endl;
    }

    if(!session.session_ticket().empty()) {
        std::cout << "Session ticket " << hex_encode(session.session_ticket()) << std::endl;
    }

    // the next tick after return
    QTimer::singleShot(0, this, &JekiQSocket::readyForApp);

    return true;
}

void JekiQSocket::processData(const byte buf[], size_t size)
{
    Q_UNUSED(buf);
    Q_UNUSED(size);

    for(size_t i = 0; i != size; ++i) {
        std::cout << buf[i];
    }
    this->client->send("GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: keep-alive\r\n\r\n");
}

void JekiQSocket::onConnected()
{
    const std::vector<std::string> protocolsToOffer = { "test/9.9", "http/1.1", "echo/9.1" };
    auto version = policy.latest_supported_version(false);

    auto tlsSocketWrite = std::bind(&JekiQSocket::socketWrite, this, _1, _2);
    auto tlsHandshakeComplete = std::bind(&JekiQSocket::handshakeComplete, this, _1);
    auto tlsAlertReceived = std::bind(&JekiQSocket::alertReceived, this, _1, _2, _3);
    auto tlsProcessData = std::bind(&JekiQSocket::processData, this, _1, _2);

    this->client = new TLS::Client(tlsSocketWrite,
                tlsProcessData,
                tlsAlertReceived,
                tlsHandshakeComplete,
                *sessionManager,
                *creds,
                policy,
                *rng,
                TLS::Server_Information(this->hostname.toStdString(), this->port),
                version,
                protocolsToOffer);
}

void JekiQSocket::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    qDebug() << socketError;
}

void JekiQSocket::onReceivedData()
{
    QByteArray rawData = this->readAll();
    this->client->received_data(reinterpret_cast<const byte*>(rawData.data()), rawData.length());
}

void JekiQSocket::onReadyForApp()
{
    qDebug() << "Client active?" << this->client->is_active();
    qDebug() << "Client closed?" << this->client->is_closed();
    std::cout << "Server choose protocol: " << this->client->application_protocol() << std::endl;
    this->client->send("GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: keep-alive\r\n\r\n");
}

