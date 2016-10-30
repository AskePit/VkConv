#include "vkconvwizard.h"
#include "downloader.h"

#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QSettings>

#include <time.h>

/////////////////////////////////////////////////
/// \brief VkConvWizard::VkConvWizard
/// \param parent
///
VkConvWizard::VkConvWizard(QWidget *parent)
    : QWizard(parent)
{
    setButtonText(QWizard::FinishButton, "New");
    setButtonText(QWizard::CancelButton, "Exit");

    setPage(Page_Auth, new AuthPage);
    setPage(Page_Menu, new MenuPage);
    setPage(Page_Details, new DetailsPage(shared));
    setPage(Page_Download, new DownloadPage(shared));

    setWindowTitle("VK Conv");
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

    QLabel *label = new QLabel("Copy authorization request, paste it to web-browser "
                          "and copy URL on which you'll be redirected");
    label->setWordWrap(true);

    const int textEditHeight = 50;

    QLabel *requestLabel = new QLabel("Request:");
    authRequest = new QTextEdit("https://oauth.vk.com/authorize?"
                                "client_id=5066618&"
                                "display=page&"
                                "redirect_uri=https://oauth.vk.com/blank.html&"
                                "scope=messages,audio,photos&"
                                "response_type=token&"
                                "v=5.53");
    authRequest->setMaximumHeight(textEditHeight);
    authRequest->setFocus();
    authRequest->selectAll();

    QLabel *responseLabel = new QLabel("Response:");
    authResponse = new QTextEdit;
    authResponse->setMaximumHeight(textEditHeight);

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
    setTitle(tr("Choose action"));

    QLabel *label = new QLabel("Choose content you want to download");
    label->setWordWrap(true);

    attachments = new QRadioButton("Attachments");
    savedPhotos = new QRadioButton("Saved photos");
    music = new QRadioButton("Music");
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

int MenuPage::nextId() const
{
    /*if(attachments->isChecked()) {
        return VkConvWizard::Page_Details;
    } else if(savedPhotos->isChecked()) {
        return VkConvWizard::Page_Download;
    } else if(music->isChecked()) {
        return VkConvWizard::Page_Download;
    } else {
        return -1;
    }*/

    return VkConvWizard::Page_Details;
}

/////////////////////////////////////////////////
/// \brief AttachmentsPage::AttachmentsPage
/// \param shared
/// \param parent
///
DetailsPage::DetailsPage(CommonData &shared, QWidget *parent)
    : QWizardPage(parent)
    , shared(shared)
{
    setTitle(tr("Details"));

    me = new QRadioButton("Me");
    notMe = new QRadioButton();
    peers = new QComboBox();

    me->setChecked(true);
    peers->setDisabled(true);

    connect(me, &QRadioButton::toggled, [&](bool checked){ peers->setDisabled(checked); });

    photo = new QCheckBox("Photo");
    audio = new QCheckBox("Audio");
    docs = new QCheckBox("Docs");

    photo->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addStretch(1);
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
    layout->addWidget(photo);
    layout->addWidget(audio);
    layout->addWidget(docs);
    layout->addStretch(1);
    setLayout(layout);

    registerField("me", me);
    registerField("peerName", peers, "currentText");
    registerField("photoAttachments", photo);
    registerField("audioAttachments", audio);
    registerField("docsAttachments", docs);
}

void DetailsPage::initializePage()
{
    QString authResponse = field("authResponse").toString();
    authResponse = authResponse.mid(authResponse.indexOf('#')+1);
    QUrlQuery authUrl(authResponse);
    shared.token = authUrl.queryItemValue("access_token");
    shared.ownerId = authUrl.queryItemValue("user_id");

    Downloader d(shared.token, shared.ownerId, nullptr);
    shared.uid2name = d.getPeers();

    for(const auto &u : shared.uid2name) {
        peers->addItem(u.second);
    }

    QSettings settings("PitM", "VkConv");
    QString savedPeer = settings.value("peerName", "").toString();
    if(!savedPeer.isEmpty()) {
        peers->setCurrentText(savedPeer);
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
        photo->hide();
        audio->hide();
        docs->hide();
    }

    if(!downloadAttachments) {
        bool meBool = settings.value("me", true).toBool();
        me->setChecked(meBool);
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
    layout->addWidget(bar);
    setLayout(layout);

    show();
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

    QString peer = field("peerName").toString();
    settings.setValue("peerName", peer);

    bool me = field("me").toBool();
    settings.setValue("me", me);

    QString peerId;
    for(const auto &u : shared.uid2name) {
        if(u.second == peer) {
            peerId = QString::number(u.first);
            break;
        }
    }

    const QString &token = shared.token;
    const QString &userId = me ? shared.ownerId : peerId;

    bool downloadAttachments = field("downloadAttachments").toBool();
    bool downloadSavedPhotos = field("downloadSavedPhotos").toBool();
    bool downloadMusic = field("downloadMusic").toBool();

    Downloader *d = new Downloader(token, shared.ownerId, bar);

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
