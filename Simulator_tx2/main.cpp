#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    MainWindow timer;

    w.move(QApplication::desktop()->screen()->rect().center() - w.rect().center());
    w.show();
    return a.exec();
}
