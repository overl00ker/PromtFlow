#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("PromtFlow");
    app.setApplicationName("PromtFlow");

    MainWindow window;
    window.setWindowTitle("PromtFlow");
    window.setWindowIcon(QIcon(":/app_icon.png"));
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
