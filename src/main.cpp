#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Video Merger Pro");
    app.setOrganizationName("MediaTools");

    MainWindow window;
    window.show();

    return app.exec();
}