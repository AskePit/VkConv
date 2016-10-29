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

typedef QList<QPair<QString, quint64>> Uid2NameMap;

class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(QProgressBar *mBar, QObject *parent = 0);
    virtual ~Downloader();

    //QString getBlankHtml();
    void downloadAttachments(const QString& token, const QString& peerId, const QString& peerName, ContentType contentType, QString startFrom = "0");
    Uid2NameMap getUsers(const QString& token);
    void downloadSavedPhotos(const QString& token, const QString& userId, int from = 0);
    void downloadMusic(const QString& token, const QString& userId, int from = 0);

private slots:
    void replyFinished();
    void slotError(QNetworkReply::NetworkError);
    void slotDownloadProgress(qint64, qint64);

private:
    QNetworkAccessManager *mNetwork;

    QString mToken;
    QString mPeerId;
    QString mUserId;
    QString mPeerName;
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
};

#endif // NETWORK_H
