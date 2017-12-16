#include "QBERcon.h"
#include <QDebug>

QBERcon::Client::Client(QObject *parent) : QObject(parent) {
    this->port = 2302;
    commandSequenceNumber = 0;
    keepAliveInterval = 5000;
    keepAliveTimer = new QTimer(this);
    socket = new QUdpSocket(this);

    dns = new QDnsLookup();
    dns->setType(QDnsLookup::A);

    connect(socket, SIGNAL(connected()), SLOT(socketConnected()));
    connect(socket, SIGNAL(disconnected()), SLOT(socketDisconnected()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(readyRead()), SLOT(read()));
    connect(keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAliveTimerTimeout()));
    connect(dns, SIGNAL(finished()), this, SLOT(hostLookupFinished()));
}

QBERcon::Client::~Client() {
    if(isConnected())
        disconnectFromServer();
}

void QBERcon::Client::connectToServer(QString password, QString hostname, quint16 port) {
    if(hostname.isEmpty() || password.isEmpty()) {
        emit error(QBERcon::ERROR_MISSING_LOGIN_DATA);
        return;
    }
    connecting = true;

    this->hostname = hostname;
    this->port = port;
    this->password = password;

    dns->setName(hostname);
    dns->lookup();
}

void QBERcon::Client::hostLookupFinished() {
    if (dns->error() != QDnsLookup::NoError) {
        qDebug() << "QBERcon: DNS Lookup failed" << dns->error() << dns->errorString();
        connecting = false;
        emit error(QBERcon::ERROR_DNS_ERROR);
        return;
    }
    if(dns->hostAddressRecords().size() > 0) {
        host = dns->hostAddressRecords().first().value();
        socket->connectToHost(host, port, QAbstractSocket::ReadWrite);
    } else {
        connecting = false;
        qDebug() << "QBERcon: DNS Lookup failed, no records";
    }
}

void QBERcon::Client::keepAliveTimerTimeout() {
    if(keepAliveReceived) {
        keepAliveReceived = false;
        sendCommand("");
    } else {
        emit error(QBERcon::ERROR_KEEPALIVE_EXCEEDED);
        disconnectFromServer();
    }
}

void QBERcon::Client::read() {
    qint64 size = socket->pendingDatagramSize();
    QByteArray data;
    data.resize(size);
    socket->readDatagram(data.data(), data.size());

    if(size < 1)
        return;
    handleData(data);
}

void QBERcon::Client::socketConnected() {
    commandSequenceNumber = 0;
    keepAliveReceived = true;
    keepAliveTimer->start(keepAliveInterval);
    sendPacket(QBERcon::PACKET_LOGIN);
}

void QBERcon::Client::disconnectFromServer() {
    connectedToServer = false;
    connecting = false;
    keepAliveTimer->stop();
    socket->disconnectFromHost();
}

quint8 QBERcon::Client::sendCommand(QString cmd) {
    if(!connectedToServer) return 0;
    sendPacket(QBERcon::PACKET_COMMAND, cmd);
    return commandSequenceNumber - 1;
}

void QBERcon::Client::socketDisconnected() {
    connectedToServer = false;
    connecting = false;
    emit disconnected();
}

void QBERcon::Client::socketError(QAbstractSocket::SocketError err) {
    qDebug() << "QBERcon:" << err;
    emit error(QBERcon::ERROR_SOCKET_ERROR);
    disconnectFromServer();
}

void QBERcon::Client::addHeaderToPacket(QByteArray &dst) {
    QByteArray result;
    quint32 crc = qcrc32(dst);
    result.append("BE");
    for(int i = 0; i < 4; ++i) {
        result.append((quint8)((crc >> (i * 8)) & 0xFF));
    }
    result.append(dst);
    dst = result;

}

void QBERcon::Client::handleData(QByteArray &data) {
    if(!(data.size() > 7 && data.at(0) == 'B' && data.at(1) == 'E')) {
        qDebug() << "Not a BE packet, ignoring";
        return;
    }

    quint32 crc_msg = 0;
    for(int i = 0; i < 4; ++i) {
        quint8 b = data.at(2 + i);
        crc_msg |= b << (i * 8);
    }

    data.remove(0, 6); // cut header and keep payload

    quint32 crc_computed = qcrc32(data);

    if(crc_msg != crc_computed) {
        qDebug() << "Packet CRC missmatch, ignoring";
        return;
    }

    quint8 packetType = data.at(1);
    //qDebug() << "Packet type" << packetType;

    switch (packetType) {
    case QBERcon::PACKET_LOGIN: {
        quint8 result = data.at(2);
        if(result == 0x01) {
            connectedToServer = true;
            connecting = false;
            emit connected();
        } else {
            emit error(QBERcon::ERROR_LOGIN_FAILED);
            disconnectFromServer();
        }
        break;
    }
    case QBERcon::PACKET_COMMAND: {
        quint8 seqNumber = data.at(2);
        if(data.length() < 4) { // ACK
            keepAliveReceived = true;
        } else {
            //  3                   4                                        5
            // 0x00 | number of packets for this response | 0-based index of the current packet
            if(data.at(3) == 0x00) { // multipart
                //qDebug() << "Multipart";
                quint8 messages_total = data.at(4);
                quint8 message_current = data.at(5);
                if(message_current == 0x00) {
                    multipartMessageData.clear();
                }
                if(messages_total > message_current) {
                    multipartMessageData.append(QString::fromUtf8(data.remove(0, 6)));
                    if((messages_total - 1) == message_current) {
                        emit commandReceived(multipartMessageData, seqNumber);
                        multipartMessageData.clear();
                    }
                }
            } else {
                QString msg = QString::fromUtf8(data.remove(0, 3));
                emit commandReceived(msg, seqNumber);
            }
        }
        break;
    }
    case QBERcon::PACKET_MESSAGE: {
        quint8 ret = data.at(2);
        QString msg = QString::fromUtf8(data.remove(0, 3));
        emit messageReceived(msg);
        sendPacket(QBERcon::PACKET_MESSAGE, ret);
        break;
    }
    default:
        break;
    }
}

void QBERcon::Client::sendPacket(QBERcon::PacketType type, QVariant data) {
    QByteArray p;
    p.append(0xFF);
    p.append(type);
    //qDebug() << "Sending packet type" << type;
    switch (type) {
    case QBERcon::PACKET_LOGIN:
        p.append(password);
        break;
    case QBERcon::PACKET_MESSAGE:
        p.append(data.toChar());
        break;
    case QBERcon::PACKET_COMMAND:
        p.append(commandSequenceNumber++);
        p.append(data.toByteArray());
        break;
    default:
        break;
    }
    addHeaderToPacket(p);
    socket->writeDatagram(p, host, port);
}

bool QBERcon::Client::isConnected() const {
    return connectedToServer;
}

bool QBERcon::Client::isConnecting() const {
    return connecting;
}

void QBERcon::Client::setKeepAliveInterval(int value) {
    keepAliveInterval = value;
}


// http://www.hackersdelight.org/hdcodetxt/crc.c.txt
/* This is derived from crc32b but does table lookup. First the table
itself is calculated, if it has not yet been set up.
Not counting the table setup (which would probably be a separate
function), when compiled to Cyclops with GCC, this function executes in
7 + 13n instructions, where n is the number of bytes in the input
message. It should be doable in 4 + 9n instructions. In any case, two
of the 13 or 9 instrucions are load byte.
   This is Figure 14-7 in the text. */
quint32 QBERcon::Client::qcrc32(QByteArray data) {
    int j;
    quint32 byte, crc, mask;
    static quint32 table[256];
    /* Set up the table, if necessary. */
    if (table[1] == 0) {
        for (byte = 0; byte <= 255; byte++) {
            crc = byte;
            for (j = 7; j >= 0; j--) {    // Do eight times.
                mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
            table[byte] = crc;
        }
    }
    /* Through with table setup, now calculate the CRC. */
    crc = 0xFFFFFFFF;
    QByteArray::Iterator it = data.begin();
    while (it != data.end()) {
        byte = *it;
        crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];
        ++it;
    }
    return ~crc;
}
