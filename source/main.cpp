#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("GT_labler");
    app.setApplicationVersion("1.0");

    MainWindow window;
    window.setWindowTitle("Ground Truth Labler");
    window.resize(1280, 720);
    window.show();

    return app.exec();
}
