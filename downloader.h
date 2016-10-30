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

typedef QList<QPair<quint64, QString>> Uid2NameMap;

class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(const QString &token, const QString &ownerId, QProgressBar *mBar, QObject *parent = 0);
    virtual ~Downloader();

    Uid2NameMap getPeers();
    void downloadAttachments(const QString& peerId, ContentType contentType, QString startFrom = "0");
    void downloadSavedPhotos(const QString& userId, int from = 0);
    void downloadMusic(const QString& userId, int from = 0);

private slots:
    void replyFinished();
    void slotError(QNetworkReply::NetworkError);
    void slotDownloadProgress(qint64, qint64);

private:
    QNetworkAccessManager *mNetwork;

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

    Uid2NameMap getUids2Names(const QList<quint64> &uids);
    QString uid2Name(const QString &uid);
    void setUserName();
};

#endif // NETWORK_H
