#include "vkconvwizard.h"
#include "downloader.h"

#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QSettings>
#include <QDir>
#include <QFileDialog>

#include <time.h>

/////////////////////////////////////////////////
/// \brief VkConvWizard::VkConvWizard
/// \param parent
///
VkConvWizard::VkConvWizard(QWidget *parent)
    : QWizard(parent)
{
    setButtonText(QWizard::FinishButton, tr("New"));
    setButtonText(QWizard::NextButton, tr("Next"));
    setButtonText(QWizard::BackButton, tr("Back"));
    setButtonText(QWizard::CancelButton, tr("Exit"));

    setPage(Page_Auth, new AuthPage);
    setPage(Page_Menu, new MenuPage);
    setPage(Page_Details, new DetailsPage(shared));
    setPage(Page_Download, new DownloadPage(shared));

    setWindowTitle(tr("VK Conv"));
}

void VkConvWizard::accept()
{
    QDialog::accept();
}

void VkConvWizard::done(int result)
{
    switch(result) {
        case 0: QDialog::done(result); break;
        case 1: restart(); break;
    }
}

/////////////////////////////////////////////////
/// \brief AuthPage::AuthPage
/// \param parent
///
AuthPage::AuthPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Authorization"));

    QLabel *label = new QLabel(tr("Copy authorization request, paste it to web-browser "
                                  "and copy URL on which you'll be redirected"));
    label->setWordWrap(true);

    const int textEditHeight = 50;

    QLabel *requestLabel = new QLabel(tr("Request:"));
    authRequest = new QTextEdit("https://oauth.vk.com/authorize?"
                                "client_id=5066618&"
                                "display=page&"
                                "redirect_uri=https://oauth.vk.com/blank.html&"
                                "scope=messages,audio,photos&"
                                "response_type=token&"
                                "v=5.53");
    authRequest->setMaximumHeight(textEditHeight);
    authRequest->setReadOnly(true);
    authRequest->setFocus();
    authRequest->selectAll();

    QLabel *responseLabel = new QLabel(tr("Response:"));
    authResponse = new QTextEdit;
    authResponse->setMaximumHeight(textEditHeight);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(requestLabel);
    layout->addWidget(authRequest);
    layout->addWidget(responseLabel);
    layout->addWidget(authResponse);
    setLayout(layout);

    registerField("authResponse", authResponse, "plainText");
}

void AuthPage::initializePage()
{
    QSettings settings("PitM", "VkConv");
    QString savedtokenResponse = settings.value("tokenResponse", "").toString();
    if(!savedtokenResponse.isEmpty()) {
        authResponse->setText(savedtokenResponse);
    }

    /*qulonglong currTime = time(0);
    qulonglong expireTime = settings.value("expireTime", 0).toULongLong();
    if(currTime < expireTime) {
        wizard()->next();
    }*/
}

enum class MenuItem {
    Attachments = 0,
    SavedPhotos = 1,
    Music = 2,
};

/////////////////////////////////////////////////
/// \brief MenuPage::MenuPage
/// \param parent
///
MenuPage::MenuPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Menu"));

    QLabel *label = new QLabel(tr("Choose content you want to download"));
    label->setWordWrap(true);

    attachments = new QRadioButton(tr("Dialog Attachments"));
    savedPhotos = new QRadioButton(tr("Saved photos"));
    music = new QRadioButton(tr("Music"));
    group = new QButtonGroup();
    group->addButton(attachments);
    group->addButton(savedPhotos);
    group->addButton(music);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(attachments);
    layout->addWidget(savedPhotos);
    layout->addWidget(music);
    layout->addStretch(1);
    setLayout(layout);

    registerField("downloadAttachments", attachments);
    registerField("downloadSavedPhotos", savedPhotos);
    registerField("downloadMusic", music);
}

void MenuPage::initializePage()
{
    QSettings settings("PitM", "VkConv");
    MenuItem item = static_cast<MenuItem>( settings.value("menuItem", 0).toInt() );
    switch(item) {
        case MenuItem::Attachments: attachments->setChecked(true); break;
        case MenuItem::SavedPhotos: savedPhotos->setChecked(true); break;
        case MenuItem::Music: music->setChecked(true); break;
    }

    QString authResponse = field("authResponse").toString();
    settings.setValue("tokenResponse", authResponse);
}

/////////////////////////////////////////////////
/// \brief AttachmentsPage::AttachmentsPage
/// \param shared
/// \param parent
///
DetailsPage::DetailsPage(CommonData &shared, QWidget *parent)
    : QWizardPage(parent)
    , downloadDir(QDir("VkDownload").absolutePath())
    , shared(shared)
{
    setTitle(tr("Details"));

    me = new QRadioButton(tr("Me"));
    notMe = new QRadioButton();
    peers = new QComboBox();

    me->setChecked(true);
    peers->setDisabled(true);

    connect(me, &QRadioButton::toggled, [&](bool checked){ peers->setDisabled(checked); });

    photo = new QCheckBox(tr("Photo"));
    audio = new QCheckBox(tr("Audio"));
    docs = new QCheckBox(tr("Docs"));

    photo->setChecked(true);

    QLabel *dirLabel = new QLabel(tr("Download folder:"));
    downloadDirPath = new QLineEdit();
    downloadDirPath->setDisabled(true);
    QPushButton *browseButton = new QPushButton(tr("Browse..."));
    connect(browseButton, SIGNAL(clicked(bool)), this, SLOT(chooseDownloadDir()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(dirLabel);
    QHBoxLayout *h2Layout = new QHBoxLayout;
    QWidget *h2LayoutWidget = new QWidget;
    h2Layout->setContentsMargins(0, 0, 0, 0);
    h2Layout->addWidget(downloadDirPath);
    h2Layout->addWidget(browseButton);
    h2LayoutWidget->setLayout(h2Layout);
    layout->addWidget(h2LayoutWidget);
    layout->addStretch(1);
    QLabel *peerLabel = new QLabel(tr("Peer to download:"));
    layout->addWidget(peerLabel);
    QHBoxLayout *hLayout = new QHBoxLayout;
    QWidget *hLayoutWidget = new QWidget;
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(me);
    hLayout->addWidget(notMe);
    hLayout->addWidget(peers);
    hLayout->addStretch(1);
    hLayoutWidget->setLayout(hLayout);
    layout->addWidget(hLayoutWidget);
    layout->addStretch(1);
    contentLabel = new QLabel(tr("Content to download:"));
    layout->addWidget(contentLabel);
    layout->addWidget(photo);
    layout->addWidget(audio);
    layout->addWidget(docs);
    setLayout(layout);

    registerField("downloadFolder", downloadDirPath);
    registerField("me", me);
    registerField("peerId", peers, "currentData");
    registerField("photoAttachments", photo);
    registerField("audioAttachments", audio);
    registerField("docsAttachments", docs);
}

void DetailsPage::chooseDownloadDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select download folder"), downloadDir);
    if(!dir.isEmpty()) {
        downloadDir = dir;
        downloadDirPath->setText(dir);
        QSettings settings("PitM", "VkConv");
        settings.setValue("downloadFolder", downloadDir);
    }
}

void DetailsPage::initializePage()
{
    contentLabel->show();
    photo->show();
    audio->show();
    docs->show();
    me->show();
    notMe->show();

    QSettings settings("PitM", "VkConv");
    QString savedDownloadFolder = settings.value("downloadFolder").toString();
    if(!savedDownloadFolder.isEmpty()) {
        downloadDir = savedDownloadFolder;
    }

    downloadDirPath->setText(downloadDir);

    QString authResponse = field("authResponse").toString();
    authResponse = authResponse.mid(authResponse.indexOf('#')+1);
    QUrlQuery authUrl(authResponse);
    shared.token = authUrl.queryItemValue("access_token");
    shared.ownerId = authUrl.queryItemValue("user_id");

    Downloader d(shared.token, shared.ownerId);
    shared.uid2name = d.getPeers();

    peers->clear();
    for(const auto &u : shared.uid2name) {
        peers->addItem(u.second, u.first);
    }

    qulonglong savedPeer = settings.value("peerId").toULongLong();
    for(int i = 0; i<peers->count(); ++i) {
        const qulonglong id = peers->itemData(i).toULongLong();
        if(id == savedPeer) {
            peers->setCurrentIndex(i);
            break;
        }
    }

    bool downloadAttachments = field("downloadAttachments").toBool();
    bool downloadSavedPhotos = field("downloadSavedPhotos").toBool();
    bool downloadMusic = field("downloadMusic").toBool();

    if(downloadAttachments) {
        notMe->setChecked(true);
        me->hide();
        notMe->hide();
    }

    if(downloadSavedPhotos || downloadMusic) {
        contentLabel->hide();
        photo->hide();
        audio->hide();
        docs->hide();
    }

    if(!downloadAttachments) {
        bool meBool = settings.value("me", true).toBool();
        me->setChecked(meBool);
        notMe->setChecked(!meBool);
    }

}

/////////////////////////////////////////////////
/// \brief DownloadPage::DownloadPage
/// \param shared
/// \param parent
///
DownloadPage::DownloadPage(CommonData &shared, QWidget *parent)
    : QWizardPage(parent)
    , shared(shared)
{
    setTitle(tr("Downloading"));

    bar = new QProgressBar();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addStretch(1);
    layout->addWidget(bar);
    layout->addStretch(2);
    setLayout(layout);
}

void DownloadPage::initializePage()
{
    QSettings settings("PitM", "VkConv");

    if(field("downloadAttachments").toBool()) {
        settings.setValue("menuItem", static_cast<int>(MenuItem::Attachments));
    } else if(field("downloadSavedPhotos").toBool()) {
        settings.setValue("menuItem", static_cast<int>(MenuItem::SavedPhotos));
    } else if(field("downloadMusic").toBool()) {
        settings.setValue("menuItem", static_cast<int>(MenuItem::Music));
    }

    qulonglong peer = field("peerId").toULongLong();
    settings.setValue("peerId", peer);

    bool me = field("me").toBool();
    settings.setValue("me", me);

    QString peerId = QString::number(peer);

    const QString &token = shared.token;
    const QString &userId = me ? shared.ownerId : peerId;

    bool downloadAttachments = field("downloadAttachments").toBool();
    bool downloadSavedPhotos = field("downloadSavedPhotos").toBool();
    bool downloadMusic = field("downloadMusic").toBool();

    QString downloadFolder = field("downloadFolder").toString();

    Downloader *d = new Downloader(token, shared.ownerId, downloadFolder, bar);

    if(downloadAttachments) {
        bool photo = field("photoAttachments").toBool();
        bool audio = field("audioAttachments").toBool();
        bool docs = field("docsAttachments").toBool();

        if(photo) {
            d->downloadAttachments(peerId, ContentType::Photo);
        }

        if(audio) {
            d->downloadAttachments(peerId, ContentType::Audio);
        }

        if(docs) {
            d->downloadAttachments(peerId, ContentType::Doc);
        }

    } else if(downloadSavedPhotos) {
        d->downloadSavedPhotos(userId);

    } else if(downloadMusic) {
        d->downloadMusic(userId);
    }
}
