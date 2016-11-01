#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include "vkconvwizard.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //QString lang = "de";
    QString lang = QLocale::system().name().left(2);

    QTranslator translator;
    if (translator.load("tr_" + lang, ":/i18n")) {
        a.installTranslator(&translator);
    }

    QTranslator qtTranslator;
    if (qtTranslator.load("qtbase_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        a.installTranslator(&qtTranslator);
    }

    VkConvWizard w;
    w.show();

    return a.exec();
}
