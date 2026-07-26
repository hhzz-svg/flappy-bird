#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>
#include "gamewidget.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WASM
    QSettings::setDefaultFormat(QSettings::WebLocalStorageFormat);
#endif

    QApplication app(argc, argv);
#ifdef Q_OS_WASM
    const int fontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/NotoSansSC-UI-Subset.otf"));
    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (fontId < 0 || families.isEmpty()) {
        qCritical() << "Failed to load the bundled WASM UI font";
        return 1;
    }
    app.setFont(QFont(families.first()));
#endif

    GameWidget w;
    w.setWindowTitle(QStringLiteral("Flappy Bird - Qt"));
    w.show();
    return app.exec();
}
