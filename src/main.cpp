#include "app/applicationcontroller.h"
#include "ui/mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[]) {
    // ApplicationController performs the device shutdown sequence on scope exit.
    QApplication app(argc, argv);
    QFile style(QStringLiteral(":/styles/default.qss"));
    if (style.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(style.readAll()));
    ApplicationController controller;
    MainWindow window(&controller);
    window.show();
    return app.exec();
}
