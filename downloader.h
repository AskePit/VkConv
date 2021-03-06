#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QNetworkReply>
#include <QHash>

class QNetworkAccessManager;
class QProgressBar;

enum class ContentType {
    Photo,
    Video,
    Audio,
    Doc,
    Link
};

typedef QList<QPair<qulonglong, QString>> Uid2NameMap;

using CStringRef = const QString &;

class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(CStringRef token, CStringRef ownerId, CStringRef downloadFolder = ".", QProgressBar *bar = 0, QObject *parent = 0);
    virtual ~Downloader();

    Uid2NameMap getPeers();
    void downloadAttachments(CStringRef peerId, ContentType contentType, CStringRef startFrom = "0");
    void downloadSavedPhotos(CStringRef userId, int from = 0);
    void downloadMusic(CStringRef userId, int from = 0);

private slots:
    void replyFinished();
    void slotError(QNetworkReply::NetworkError);
    void slotDownloadProgress(qint64, qint64);

private:
    QNetworkAccessManager *mNetwork;

    QString mDownloadFolder;

    QString mToken;

    QString mOwnerId;
    QString mOwnerName;

    bool mPeersMapCached;
    Uid2NameMap mPeersMap;
    QString mUserId;
    QString mUserName;

    ContentType mContentType;

    QProgressBar *mBar;

    QHash<QNetworkReply*, ContentType> mAttachmentsMap;          // reply -> AttachmentsType
    QHash<QNetworkReply*, int> mSavedPhotosMap;                  // reply -> "from" parameter
    QHash<QNetworkReply*, int> mMusicMap;                        // reply -> "from" parameter
    QHash<QNetworkReply*, QString> mFileMap;                     // reply -> fileName
    QHash<QNetworkReply*, QPair<quint64, quint64>> mProgressMap; // reply -> (received, total) bytes

    QNetworkReply *getReply(const QUrl &url);
    void _downloadAttachments(QNetworkReply* reply);
    void _downloadSavedPhotos(QNetworkReply* reply);
    void _downloadMusic(QNetworkReply* reply);
    void downloadFile(QNetworkReply* reply);

    Uid2NameMap getUids2Names(const QList<qulonglong> &uids);
    QString uid2Name(CStringRef uid);
    void setUserName();
};

#endif // NETWORK_H
