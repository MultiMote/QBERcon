QBERcon - BattlEye Rcon connector for Qt5 C++
=============================================

## Public functions:
```c++
    void connectToServer(QString password, QString hostname, quint16 port = 2302);
    void disconnectFromServer();
    bool isConnected() const;
    void setKeepAliveInterval(int value);   // In milliseconds. Default interval is 5 seconds
    quint8 sendCommand(QString cmd);        // Send command and return command sequence number    
```

## Signals:
```c++
    void messageReceived(QString &message); // Emitted when server broadcasts message
    void commandReceived(QString message, quint8 seqNumber); // Emitted when server replies command with sequence number
    void connected();                       // Emitted after successful login
    void disconnected();                    // Emitted after disconnect, timeout, etc.
    void error(QBERcon::RconError err);     // Emitted when error thrown
```

## Errors:
```c++
    QBERcon::ERROR_LOGIN_FAILED             // Wrong password
    QBERcon::ERROR_KEEPALIVE_EXCEEDED       // Timeout
    QBERcon::ERROR_MISSING_LOGIN_DATA       // No login/password specified
```

Not fully tested. Use at your own risk. 