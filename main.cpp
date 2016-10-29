#include <QApplication>
#include <QNetworkReply>
#include "vkconvwizard.h"
#include "downloader.h"

/*
static void waitForFinished(QNetworkReply *reply)
{
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}
*/

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    VkConvWizard w;
    w.show();

    /*Downloader d(nullptr);
    qDebug() << qPrintable(d.getBlankHtml());*/

    /*QNetworkAccessManager manager;
    QUrl url("https://oauth.vk.com/authorize?client_id=5066618&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=messages,audio&response_type=token&v=5.53");
    QNetworkReply *reply = manager.get(QNetworkRequest(url));
    waitForFinished(reply);

    qDebug() << qPrintable(reply->readAll());
    qDebug() << qPrintable(reply->header(QNetworkRequest::ServerHeader).toString());
    qDebug() << qPrintable(reply->url().toString());

    QUrl url2("https://oauth.vk.com/blank.html");
    QNetworkReply *reply2 = manager.get(QNetworkRequest(url2));
    waitForFinished(reply2);

    qDebug() << qPrintable(reply2->readAll());
    qDebug() << qPrintable(reply2->header(QNetworkRequest::ServerHeader).toString());
    qDebug() << qPrintable(reply2->url().toString());*/

    return a.exec();
}
