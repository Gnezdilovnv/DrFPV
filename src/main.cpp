#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include "MainWindow.h"
int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    app.setApplicationName("DrFPV");
    app.setOrganizationName("DrFPV");
    MainWindow win;
    win.show();
    return app.exec();
}
