#include <QApplication>
#include <QTimer>
#include <examplegui.h>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ExampleGui *example = new ExampleGui();
    example->show();
    return a.exec();
}
