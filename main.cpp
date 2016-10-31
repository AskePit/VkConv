#include <QApplication>
#include <QTranslator>
#include "vkconvwizard.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    if (translator.load("tr_" + QLocale::system().name().left(2), ":/i18n")) {
        a.installTranslator(&translator);
    }

    VkConvWizard w;
    w.show();

    return a.exec();
}
