#ifndef QTBERCON_H
#define QTBERCON_H

#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QDnsLookup> // QHostInfo::lookupHost slow for me some reason, so QT5

namespace QBERcon {
enum PacketType {
    PACKET_LOGIN = 0x00,
    PACKET_COMMAND = 0x01,
    PACKET_MESSAGE = 0x02,
};

enum RconError {
    ERROR_NONE = 0,
    ERROR_LOGIN_FAILED,
    ERROR_KEEPALIVE_EXCEEDED,
    ERROR_MISSING_LOGIN_DATA,
    ERROR_DNS_ERRROR,
    ERROR_SOCKET_ERRROR
};


class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    ~Client();
    void connectToServer(QString password, QString hostname, quint16 port = 2302);
    void disconnectFromServer();
    quint8 sendCommand(QString cmd);
    bool isConnected() const;
    void setKeepAliveInterval(int value);

signals:
    void messageReceived(QString &message);
    void commandReceived(QString message, quint8 seqNumber);
    void connected();
    void disconnected();
    void error(QBERcon::RconError err);

public slots:    
    void keepAliveTimerTimeout();
    void read();
    void socketConnected();
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError err);
    void hostLookupFinished();
private:
    void addHeaderToPacket(QByteArray &dst);
    void handleData(QByteArray &data);
    void sendPacket(QBERcon::PacketType type, QVariant data = QVariant());
    quint32 qcrc32(QByteArray data);

    QTimer *keepAliveTimer;
    QUdpSocket *socket;
    QDnsLookup *dns;

    QString password;
    quint16 port;
    QString hostname;
    QHostAddress host;

    quint8 commandSequenceNumber;    
    bool connectedToServer;
    int keepAliveInterval;
    bool keepAliveReceived;
    QString multipartMessageData;
};

}
#endif // QTBERCON_H
