#include "downloader.h"

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QProgressBar>
#include <QEventLoop>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

#include <QDebug>

Downloader::Downloader(const QString &token, const QString &ownerId, const QString &downloadFolder, QProgressBar *bar, QObject *parent)
    : QObject(parent)
    , mDownloadFolder(downloadFolder)
    , mToken(token)
    , mOwnerId(ownerId)
    , mPeersMapCached(false)
    , mBar(bar)

{
    mNetwork = new QNetworkAccessManager(this);
    mOwnerName = uid2Name(mOwnerId);
}

Downloader::~Downloader()
{
    mNetwork->deleteLater();
}

static QString ContentTypeString(ContentType type) {
    switch(type) {
        case ContentType::Photo: return "photo";
        case ContentType::Video: return "video";
        case ContentType::Audio: return "audio";
        case ContentType::Doc: return "doc";
        case ContentType::Link: return "link";
        default: return QString();
    }
}

#define SENDER qobject_cast<QNetworkReply*>(QObject::sender())

#define OBJ .toObject()
#define ARR .toArray()
#define STR .toString()
#define INT .toInt()

QNetworkReply *Downloader::getReply(const QUrl &url)
{
    QNetworkReply *reply = mNetwork->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(slotDownloadProgress(qint64, qint64)));

    return reply;
}

static QJsonDocument getJsonDoc(QNetworkReply* reply)
{
    auto data = reply->readAll();
    //qDebug() << data;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) {
        qWarning() << QString("JSON parse error at ") + QString::number(error.offset) + " char: " + error.errorString();
        qWarning() << QString(data);
        return document;
    }

    if(document.isEmpty() || document.isNull()) {
        qWarning() << "JSON parse error: document is empty or incorrect";
    }

    return document;
}

static void waitForFinished(QNetworkReply *reply)
{
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

Uid2NameMap Downloader::getUids2Names(const QList<qulonglong> &uids)
{
    QUrlQuery usersQ;
    QString userIds;
    for(const auto &uid : uids) {
        userIds += QString::number(uid) + ",";
    }
    userIds.truncate(userIds.length()-1);

    usersQ.addQueryItem("user_ids", userIds);
    usersQ.addQueryItem("access_token", mToken);

    QUrl usersUrl("https://api.vk.com/method/users.get");
    usersUrl.setQuery(usersQ);

    QNetworkReply *usersReply = mNetwork->get(QNetworkRequest(usersUrl));
    waitForFinished(usersReply);

    QJsonDocument usersDoc = getJsonDoc(usersReply);
    QJsonArray usersAnswer = usersDoc.object()["response"]ARR;

    Uid2NameMap uids2names;
    for(const auto &user : usersAnswer) {
        if(!user.isObject()) {
            continue;
        }

        int uid = user OBJ["uid"]INT;
        QString firstName = user OBJ["first_name"]STR;
        QString lastName = user OBJ["last_name"]STR;

        QPair<qulonglong, QString> pair(uid, QString("%1 %2").arg(firstName, lastName));
        uids2names << pair;
    }

    return uids2names;
}

QString Downloader::uid2Name(const QString &uid)
{
    auto l = getUids2Names( QList<qulonglong>{uid.toULongLong()} );
    if(l.isEmpty()) {
        return QString();
    }

    return l[0].second;
}

Uid2NameMap Downloader::getPeers()
{
    if(mPeersMapCached) {
        return mPeersMap;
    }

    QUrlQuery dialogsQ;
    dialogsQ.addQueryItem("count", "200");
    dialogsQ.addQueryItem("preview_length", "1");
    dialogsQ.addQueryItem("access_token", mToken);

    QUrl dialogsUrl("https://api.vk.com/method/messages.getDialogs");
    dialogsUrl.setQuery(dialogsQ);

    QNetworkReply *dialogsReply = mNetwork->get(QNetworkRequest(dialogsUrl));
    waitForFinished(dialogsReply);

    QJsonDocument dialogsDoc = getJsonDoc(dialogsReply);

    QJsonArray dialogsAnswer = dialogsDoc.object()["response"]ARR;
    QList<qulonglong> uids;
    for(const auto &user : dialogsAnswer) {
        if(!user.isObject()) {
            continue;
        }
        uids << user OBJ["uid"]INT;
    }

    mPeersMapCached = true;

    return getUids2Names(uids);
}

void Downloader::setUserName()
{
    auto peersMap = getPeers();
    for(const auto &p : peersMap) {
        if(QString::number(p.first) == mUserId) {
            mUserName = p.second;
            break;
        }
    }
}

void Downloader::downloadAttachments(const QString& peerId, ContentType type, QString startFrom)
{
    mUserId = peerId;
    mContentType = type;
    setUserName();

    QUrlQuery query;
    query.addQueryItem("peer_id", peerId);
    query.addQueryItem("media_type", ContentTypeString(type));
    query.addQueryItem("start_from", startFrom);
    query.addQueryItem("count", "200");
    query.addQueryItem("photo_sizes", "1");
    query.addQueryItem("access_token", mToken);

    QUrl url("https://api.vk.com/method/messages.getHistoryAttachments");
    url.setQuery(query);

    mAttachmentsMap[getReply(url)] = type;
}


void Downloader::replyFinished()
{
    QNetworkReply* reply = SENDER;

    if(mAttachmentsMap.keys().contains(reply)) {
        _downloadAttachments(reply);
    } else if(mSavedPhotosMap.contains(reply)) {
        _downloadSavedPhotos(reply);
    } else if(mMusicMap.contains(reply)) {
        _downloadMusic(reply);
    } else if(mFileMap.keys().contains(reply)) {
        downloadFile(reply);
    }
}

static QString getPicUrl(const QJsonArray &sizes)
{
    int idx = 0;
    int width = 0;
    for(int i = 0; i<sizes.size(); ++i) {
        const auto& s = sizes[i]OBJ;
        int w = s["width"]INT;
        if(w > width) {
            width = w;
            idx = i;
        }
    }

    QString url = sizes[idx]OBJ["src"]STR;

    if(url.startsWith("https://pp.vk.me/")) {
        return url;
    }

    // http://cs323126.vk.me/v323126704/58cb/P1sQv5Vhypo.jpg
    // to
    // https://pp.vk.me/c323126/v323126704/58cb/P1sQv5Vhypo.jpg

    QString prefix = url.mid(7, url.indexOf('.')-7);
    prefix.remove(1, 1);
    url = QString("https://pp.vk.me/") + prefix + url.mid(url.indexOf("vk.me") + 5);

    return url;
}

void Downloader::_downloadAttachments(QNetworkReply* reply)
{
    QJsonDocument document = getJsonDoc(reply);
    if(document.isEmpty() || document.isNull()) {
        return;
    }

    QJsonObject answer = document.object();
    QJsonValueRef rawResponse = answer["response"];
    QJsonObject response;

    if(rawResponse.isArray()) {
        QJsonArray array = rawResponse.toArray();
        for(int i = 0; i<array.size(); ++i) {
            response[QString::number(i)] = array[i];
        }
    } else if(rawResponse.isObject()) {
        response = rawResponse OBJ;
    } else {
        qWarning() << "JSON response is not array or object!";
        return;
    }

    ContentType contentType = mAttachmentsMap[reply];
    QString userDirName = (mOwnerId == mUserId) ? "_Me" : QString("%1 %2").arg(mUserName, mUserId);
    QString dirName(QString("%1/%2 %3/%4/%5/%6/").arg(mDownloadFolder, mOwnerName, mOwnerId, userDirName, "attachments", ContentTypeString(contentType)));
    QDir d;
    d.mkpath(dirName);

    for(const auto& key : response.keys()) {
        if(key == "0") continue;
        if(key == "next_from") {
            downloadAttachments(mUserId, mContentType, response["next_from"]STR);
            continue;
        }

        QString url;
        QString fileName = dirName;
        switch(contentType) {
            case ContentType::Photo: {
                QJsonObject photo = response[key]OBJ["photo"]OBJ;
                QJsonArray sizes = photo["sizes"]ARR;

                url = getPicUrl(sizes);

                fileName += QString::number(photo["created"]INT) + ".jpg";
            } break;
            case ContentType::Doc: {
                QJsonObject doc = response[key]OBJ["doc"]OBJ;
                url = doc["url"]STR;
                int did = doc["did"]INT;
                QString title = doc["title"]STR;

                fileName += QString("%1_%2").arg(QString::number(did)).arg(title);
            } break;
            case ContentType::Audio: {
                QJsonObject audio = response[key]OBJ["audio"]OBJ;
                url = audio["url"]STR;
                QString artist = audio["artist"]STR;
                QString title = audio["title"]STR;
                fileName += QString("%1 - %2.mp3").arg(artist).arg(title);
            } break;
            default: continue;
        }

        QFileInfo info(fileName);
        if(info.exists()) {
            continue;
        }

        if(url == "") {
            continue;
        }

        url.replace("http://", "https://");
        qDebug() << url;

        mFileMap[getReply(url)] = fileName;
    }

    mAttachmentsMap.remove(reply);
    reply->deleteLater();
}

void Downloader::downloadFile(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->error();
        return;
    }

    QString fileName(mFileMap[reply]);

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(statusCode >= 300 && statusCode < 400) {
        qDebug() << "status code" << statusCode;

        QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        qDebug() << "redirect" << redirect;

        mFileMap.remove(reply);
        reply->deleteLater();

        mFileMap[getReply(redirect)] = fileName;
        return;
    }

    int bytes = reply->bytesAvailable();

    mProgressMap[reply] = QPair<quint64, quint64>(bytes, bytes);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open file " << fileName;
        return;
    }
    file.write(reply->readAll());
    file.close();

    mFileMap.remove(reply);
    reply->deleteLater();
}

void Downloader::slotError(QNetworkReply::NetworkError error)
{
    if(error != QNetworkReply::NoError) {
        qDebug() << "error" << error;
    }
}

void Downloader::slotDownloadProgress(qint64 received, qint64 total)
{
    auto reply = SENDER;
    mProgressMap[reply] = QPair<quint64, quint64>(received, total);

    quint64 rec = 0;
    quint64 tot = 0;
    for(const auto &p : mProgressMap) {
        rec += p.first;
        tot += p.second;
    }

    if(tot <= 0) {
        return;
    }
    mBar->setValue(rec/(float)tot*100.);
}

void Downloader::downloadSavedPhotos(const QString& userId, int from)
{
    mUserId = userId;
    mContentType = ContentType::Photo;
    setUserName();

    QUrlQuery query;
    query.addQueryItem("owner_id", mUserId);
    query.addQueryItem("album_id", "saved");
    query.addQueryItem("photo_sizes", "1");
    query.addQueryItem("offset", QString::number(from));
    query.addQueryItem("count", "1000");
    query.addQueryItem("access_token", mToken);

    QUrl url("https://api.vk.com/method/photos.get");
    url.setQuery(query);

    mSavedPhotosMap[getReply(url)] = from;
}

void Downloader::_downloadSavedPhotos(QNetworkReply* reply)
{
    QJsonDocument document = getJsonDoc(reply);
    if(document.isEmpty() || document.isNull()) {
        return;
    }

    QString userDirName = (mOwnerId == mUserId) ? "_Me" : QString("%1 %2").arg(mUserName, mUserId);
    QString dirName(QString("%1/%2 %3/%4/%5").arg(mDownloadFolder, mOwnerName, mOwnerId, userDirName, "saved photos"));
    QDir d;
    d.mkpath(dirName);

    QJsonObject answer = document.object();
    QJsonArray response = answer["response"]ARR;

    for(const auto& elem : response) {
        QJsonObject photo = elem OBJ;
        QJsonArray sizes = photo["sizes"]ARR;

        QString fileName = QString("%1/%2.jpg").arg(dirName, QString::number(photo["pid"]INT));

        QString url = getPicUrl(sizes);
        if(url == "") {
            continue;
        }

        qDebug() << url;

        QFileInfo info(fileName);
        if(info.exists()) {
            continue;
        }

        mFileMap[getReply(url)] = fileName;
    }

    if(response.count()) {
        downloadSavedPhotos(mUserId, mSavedPhotosMap[reply]+1000);
    }

    mSavedPhotosMap.remove(reply);
    reply->deleteLater();
}

void Downloader::downloadMusic(const QString& userId, int from)
{
    mUserId = userId;
    mContentType = ContentType::Audio;
    setUserName();

    QUrlQuery query;
    query.addQueryItem("owner_id", userId);
    query.addQueryItem("offset", QString::number(from));
    query.addQueryItem("count", "1000");
    query.addQueryItem("access_token", mToken);

    QUrl url("https://api.vk.com/method/audio.get");
    url.setQuery(query);

    mMusicMap[getReply(url)] = from;
}

void Downloader::_downloadMusic(QNetworkReply* reply)
{
    QJsonDocument document = getJsonDoc(reply);
    if(document.isEmpty() || document.isNull()) {
        return;
    }

    QString userDirName = (mOwnerId == mUserId) ? "_Me" : QString("%1 %2").arg(mUserName, mUserId);
    QString dirName(QString("%1/%2 %3/%4/%5").arg(mDownloadFolder, mOwnerName, mOwnerId, userDirName, "music"));
    QDir d;
    d.mkpath(dirName);

    QJsonObject answer = document.object();
    QJsonArray response = answer["response"]ARR;

    for(const auto& elem : response) {
        auto audio = elem OBJ;
        QString url = audio["url"]STR;
        QString artist = audio["artist"]STR;
        QString title = audio["title"]STR;

        QString fileName = QString("%1/%2 - %3.mp3").arg(dirName, artist, title);
        mFileMap[getReply(url)] = fileName;
    }

    if(response.count()) {
        downloadMusic(mOwnerId, mMusicMap[reply]+1000);
    }

    mMusicMap.remove(reply);
    reply->deleteLater();
}
