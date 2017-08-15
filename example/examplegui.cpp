#include "examplegui.h"
#include "ui_examplegui.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

ExampleGui::ExampleGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExampleGui) {
    ui->setupUi(this);
    be = new QBERcon::Client(this);


#ifdef PASSWORD
    ui->passwordInput->setText(QUOTE(PASSWORD));
#endif
#ifdef SERVER_IP
    ui->hostInput->setText(QUOTE(SERVER_IP));
#endif
#ifdef SERVER_PORT
    ui->portInput->setValue(SERVER_PORT);
#endif


    connect(be, SIGNAL(connected()), this, SLOT(connected()));
    connect(be, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(be, SIGNAL(error(QBERcon::RconError)), this, SLOT(error(QBERcon::RconError)));
    connect(be, SIGNAL(messageReceived(QString&)), this, SLOT(messageReceived(QString&)));
    connect(be, SIGNAL(commandReceived(QString, quint8)), this, SLOT(commandReceived(QString, quint8)));
}

ExampleGui::~ExampleGui() {
    delete ui;
}


void ExampleGui::messageReceived(QString &message) {
    ui->logOutput->append(QString("<font color=\"#67983E\">[MSG]: %1</font>")
                              .arg(message));
    qDebug() << __PRETTY_FUNCTION__ << message;
}

void ExampleGui::commandReceived(QString message, quint8 seqNumber) {
    ui->logOutput->append(QString("<font color=\"#E19B3E\">[CMD ID=%1]: %2</font>")
                              .arg(QString::number(seqNumber), message));
    qDebug() << __PRETTY_FUNCTION__ << "number =" << seqNumber << "Data:" << message;
}

void ExampleGui::connected() {
    ui->logOutput->append(QString("<font color=\"#42B555\">[CONNECTED]</font>"));
    qDebug() << __PRETTY_FUNCTION__;
}

void ExampleGui::disconnected() {
    ui->logOutput->append(QString("<font color=\"#D82672\">[DISCONNECTED]</font>"));
    qDebug() << __PRETTY_FUNCTION__;
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
}

void ExampleGui::error(QBERcon::RconError err) {
    ui->logOutput->append(QString("<font color=\"#8E1717\">[ERR]: %1</font>")
                              .arg(QString::number(err)));
    qDebug() << __PRETTY_FUNCTION__ << err;
}

void ExampleGui::on_connectButton_clicked() {
    be->connectToServer(ui->passwordInput->text(), ui->hostInput->text(), ui->portInput->value());
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);
    ui->commandButton->setEnabled(true);
}

void ExampleGui::on_disconnectButton_clicked() {
    be->disconnectFromServer();
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
}

void ExampleGui::on_commandButton_clicked() {
    QString command = ui->commandInput->text();
    quint8 number = be->sendCommand(command);
    ui->logOutput->append(QString("<font color=\"#5D5815\">[CMD]: Sent %1 with ID=%2</font>")
                              .arg(command, QString::number(number)));
}
