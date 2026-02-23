#include <QApplication>
#include "app.h"

int main(int argc, char *argv[])
{
    // LSUIElement=true in Info.plist hides the dock icon on macOS

    QApplication a(argc, argv);
    a.setApplicationName("MacMagic");
    a.setQuitOnLastWindowClosed(false);

    App app;

    return a.exec();
}
