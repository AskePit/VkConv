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
#include <QDir>
#include <QFileDialog>

QSettings Registry::reg("PitM", "VkConv");

/////////////////////////////////////////////////
/// \brief VkConvWizard::VkConvWizard
/// \param parent
///
VkConvWizard::VkConvWizard(QWidget *parent)
    : QWizard(parent)
{
    setButtonText(QWizard::FinishButton, tr("New"));
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
    : VkConvPage(parent)
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

    declareField(Field::TokenResponse, authResponse, "plainText");
}

void AuthPage::initializePage()
{
    QString savedToken = Registry::String(Field::TokenResponse);
    if(!savedToken.isEmpty()) {
        authResponse->setText(savedToken);
    }
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
    : VkConvPage(parent)
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

    declareField(Field::Attachments, attachments);
    declareField(Field::SavedPhotos, savedPhotos);
    declareField(Field::Music, music);
}

void MenuPage::initializePage()
{
    attachments->setChecked(Registry::Bool(Field::Attachments));
    savedPhotos->setChecked(Registry::Bool(Field::SavedPhotos));
    music->setChecked(Registry::Bool(Field::Music));

    field2Registry(Field::TokenResponse);
}

/////////////////////////////////////////////////
/// \brief AttachmentsPage::AttachmentsPage
/// \param shared
/// \param parent
///
DetailsPage::DetailsPage(CommonData &shared, QWidget *parent)
    : VkConvPage(parent)
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

    declareField(Field::DownloadFolder, downloadDirPath);
    declareField(Field::Me, me);
    declareField(Field::PeerId, peers, "currentData");
    declareField(Field::PhotoAttachments, photo);
    declareField(Field::AudioAttachments, audio);
    declareField(Field::DocsAttachments, docs);
}

void DetailsPage::chooseDownloadDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select download folder"), downloadDir);
    if(!dir.isEmpty()) {
        downloadDir = dir;
        downloadDirPath->setText(dir);
        Registry::set(Field::DownloadFolder, downloadDir);
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

    QString savedDownloadFolder = Registry::String(Field::DownloadFolder);
    if(!savedDownloadFolder.isEmpty()) {
        downloadDir = savedDownloadFolder;
    }

    downloadDirPath->setText(downloadDir);

    QString authResponse = stringField(Field::TokenResponse);
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

    qulonglong savedPeer = Registry::ULL(Field::PeerId);
    for(int i = 0; i<peers->count(); ++i) {
        const qulonglong id = peers->itemData(i).toULongLong();
        if(id == savedPeer) {
            peers->setCurrentIndex(i);
            break;
        }
    }

    bool downloadAttachments = boolField(Field::Attachments);
    bool downloadSavedPhotos = boolField(Field::SavedPhotos);
    bool downloadMusic = boolField(Field::Music);

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
        bool meBool = Registry::Bool(Field::Me, true);
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
    : VkConvPage(parent)
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
    bool downloadAttachments = boolField(Field::Attachments);
    bool downloadSavedPhotos = boolField(Field::SavedPhotos);
    bool downloadMusic = boolField(Field::Music);

    Registry::set(Field::Attachments, downloadAttachments);
    Registry::set(Field::SavedPhotos, downloadSavedPhotos);
    Registry::set(Field::Music, downloadMusic);

    field2Registry(Field::PeerId);
    field2Registry(Field::Me);

    bool me = boolField(Field::Me);
    qulonglong peer = ullField(Field::PeerId);

    QString peerId = QString::number(peer);

    const QString &token = shared.token;
    const QString &userId = me ? shared.ownerId : peerId;

    QString downloadFolder = stringField(Field::DownloadFolder);

    Downloader *d = new Downloader(token, shared.ownerId, downloadFolder, bar);

    if(downloadAttachments) {
        bool photo = boolField(Field::PhotoAttachments);
        bool audio = boolField(Field::AudioAttachments);
        bool docs = boolField(Field::DocsAttachments);

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
