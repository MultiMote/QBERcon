#ifndef EXAMPLEGUI_H
#define EXAMPLEGUI_H

#include "QBERcon.h"

#include <QWidget>

namespace Ui {
class ExampleGui;
}

class ExampleGui : public QWidget
{
    Q_OBJECT

public:
    explicit ExampleGui(QWidget *parent = 0);
    ~ExampleGui();


public slots:
    void messageReceived(QString &message);
    void commandReceived(QString message, quint8 seqNumber);
    void connected();
    void disconnected();
    void error(QBERcon::RconError err);

private slots:
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    void on_commandButton_clicked();

private:
    QBERcon::Client *be;
    Ui::ExampleGui *ui;
};

#endif // EXAMPLEGUI_H
