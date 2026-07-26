#include <QApplication>
#include "gamewidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GameWidget w;
    w.setWindowTitle(QStringLiteral("Flappy Bird - Qt"));
    w.show();
    return app.exec();
}
