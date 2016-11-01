#include <QApplication>
#include <QTranslator>
#include "vkconvwizard.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;

    //QString lang = "de";
    QString lang = QLocale::system().name().left(2);
    if (translator.load("tr_" + lang, ":/i18n")) {
        a.installTranslator(&translator);
    }

    VkConvWizard w;
    w.show();

    return a.exec();
}
