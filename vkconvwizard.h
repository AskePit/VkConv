#ifndef VKCONVWIZARD_H
#define VKCONVWIZARD_H

#include <QWizard>

class QTextEdit;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QProgressBar;
class QRadioButton;
class QButtonGroup;

namespace Ui {
class VkConvWizard;
}

typedef QList<QPair<qulonglong, QString>> Uid2NameMap;

struct CommonData {
    Uid2NameMap uid2name;
    bool newToken;
    QString token;
    QString ownerId;

    CommonData() : newToken(true) {}
};

/////////////////////////////////////////////////
/// \brief The VkConvWizard class
///
class VkConvWizard : public QWizard
{
    Q_OBJECT

public:
    VkConvWizard(QWidget *parent = 0);
    enum { Page_Auth, Page_Menu, Page_Details, Page_Download };

protected:
    void accept();
    void done(int result);

    CommonData shared;
};

/////////////////////////////////////////////////
/// \brief The AuthPage class
///
class AuthPage : public QWizardPage
{
    Q_OBJECT

public:
    AuthPage(QWidget *parent = 0);

protected:
    void initializePage();

private:
    QTextEdit *authRequest;
    QTextEdit *authResponse;
};

/////////////////////////////////////////////////
/// \brief The MenuPage class
///
class MenuPage : public QWizardPage
{
    Q_OBJECT

public:
    MenuPage(QWidget *parent = 0);

protected:
    void initializePage();

private:
    QRadioButton *attachments;
    QRadioButton *savedPhotos;
    QRadioButton *music;
    QButtonGroup *group;
};

/////////////////////////////////////////////////
/// \brief The AttachmentsPage class
///
class DetailsPage : public QWizardPage
{
    Q_OBJECT

public:
    DetailsPage(CommonData &shared, QWidget *parent = 0);

protected:
    void initializePage();

private:
    QRadioButton *me;
    QRadioButton *notMe;
    QComboBox *peers;
    QCheckBox *photo;
    QCheckBox *audio;
    QCheckBox *docs;

    CommonData &shared;
};

/////////////////////////////////////////////////
/// \brief The DownloadPage class
///
class DownloadPage : public QWizardPage
{
    Q_OBJECT

public:
    DownloadPage(CommonData &shared, QWidget *parent = 0);

protected:
    void initializePage();

private:
    QProgressBar *bar;

    CommonData &shared;
};

#endif // VKCONVWIZARD_H
