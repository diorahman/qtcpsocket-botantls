#include <QCoreApplication>

#include "jekiqsocket.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    JekiQSocket * socket = new JekiQSocket(qApp);
    socket->connectToHost("www.google.com", 443);

    return a.exec();
}

