#include <QApplication>
#include <QSettings>
#include "gamewidget.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WASM
    QSettings::setDefaultFormat(QSettings::WebLocalStorageFormat);
#endif

    QApplication app(argc, argv);
    GameWidget w;
    w.setWindowTitle(QStringLiteral("Flappy Bird - Qt"));
    w.show();
    return app.exec();
}
