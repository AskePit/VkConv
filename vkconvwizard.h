#ifndef VKCONVWIZARD_H
#define VKCONVWIZARD_H

#include <QWizard>
#include <QSettings>

class QTextEdit;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QProgressBar;
class QRadioButton;
class QButtonGroup;
class QLabel;

using CString = const QString &;

enum class Field {
    TokenResponse,
    Attachments,
    SavedPhotos,
    Music,
    DownloadFolder,
    Me,
    PeerId,
    PhotoAttachments,
    AudioAttachments,
    DocsAttachments,
};

static QString fromField(Field field) {
#define CASE(X) case Field::X: return #X
    switch(field) {
        CASE(TokenResponse);
        CASE(Attachments);
        CASE(SavedPhotos);
        CASE(Music);
        CASE(DownloadFolder);
        CASE(Me);
        CASE(PeerId);
        CASE(PhotoAttachments);
        CASE(AudioAttachments);
        CASE(DocsAttachments);
    }
#undef CASE
    return QString::null;
}

class Registry
{
public:
    static QString String(Field key, CString def = QString::null) { return reg.value(fromField(key), def).toString(); }
    static int Int(Field key, int def = 0)                        { return reg.value(fromField(key), def).toInt(); }
    static qulonglong ULL(Field key, qulonglong def = 0)          { return reg.value(fromField(key), def).toULongLong(); }
    static bool Bool(Field key, bool def = false)                 { return reg.value(fromField(key), def).toBool(); }

    static void set(Field key, QVariant val) { reg.setValue(fromField(key), val); }

private:
    static QSettings reg;

    Registry();
    Registry(const Registry &) = delete;
    operator=(const Registry &) = delete;
};

typedef QList<QPair<qulonglong, QString>> Uid2NameMap;

struct CommonData {
    Uid2NameMap uid2name;
    bool newToken;
    QString token;
    QString ownerId;

    CommonData() : newToken(true) {}
};

namespace Ui {
class VkConvWizard;
}

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

class VkConvPage : public QWizardPage
{
    Q_OBJECT
public:
    VkConvPage(QWidget *parent = 0) : QWizardPage(parent) {}

    QString stringField(Field key) { return field(fromField(key)).toString(); }
    int intField(Field key)        { return field(fromField(key)).toInt(); }
    qulonglong ullField(Field key) { return field(fromField(key)).toULongLong(); }
    bool boolField(Field key)      { return field(fromField(key)).toBool(); }

    void declareField(Field key, QWidget *w, const char *prop = nullptr) { registerField(fromField(key), w, prop); }

    void field2Registry(Field f) {
        Registry::set(f, field(fromField(f)));
    }
};

/////////////////////////////////////////////////
/// \brief The AuthPage class
///
class AuthPage : public VkConvPage
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
class MenuPage : public VkConvPage
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
class DetailsPage : public VkConvPage
{
    Q_OBJECT

public:
    DetailsPage(CommonData &shared, QWidget *parent = 0);

protected:
    void initializePage();

private slots:
    void chooseDownloadDir();

private:
    QString downloadDir;

    QLineEdit *downloadDirPath;
    QRadioButton *me;
    QRadioButton *notMe;
    QComboBox *peers;
    QLabel *contentLabel;
    QCheckBox *photo;
    QCheckBox *audio;
    QCheckBox *docs;

    CommonData &shared;
};

/////////////////////////////////////////////////
/// \brief The DownloadPage class
///
class DownloadPage : public VkConvPage
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
