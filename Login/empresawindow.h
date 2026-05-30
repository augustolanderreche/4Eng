#ifndef EMPRESAWINDOW_H
#define EMPRESAWINDOW_H

#include <QMainWindow>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLineEdit;
class QTextEdit;
class QComboBox;

class EmpresaWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EmpresaWindow(const QString &displayName, QWidget *parent = nullptr);

private slots:
    void loadProfile();
    void loadJobs();
    void loadApplications();
    void createJob();
    void sendChatMessage();
    void handleLogout();

private:
    void setupUi(const QString &displayName);
    QWidget *createPerfilPage();
    QWidget *createPublicacionesPage();
    QWidget *createPostulacionesPage();
    QWidget *createNuevaPublicacionPage();
    QWidget *createChatPage();
    void setStatus(const QString &message, bool ok = true);

    QLabel *m_welcomeLabel;
    QLabel *m_statusLabel;
    QLabel *m_profileLabel;
    QLabel *m_publicacionDetailLabel;
    QLabel *m_postulacionDetailLabel;
    QListWidget *m_menuList;
    QListWidget *m_publicacionesList;
    QListWidget *m_postulacionesList;
    QStackedWidget *m_contentStack;
    QTextEdit *m_chatHistory;
    QLineEdit *m_chatInput;

    QLineEdit *m_newPostTitleEdit;
    QLineEdit *m_newPostSkillsEdit;
    QLineEdit *m_newPostCityEdit;
    QLineEdit *m_newPostCountryEdit;
    QLineEdit *m_newPostExperienceEdit;
    QComboBox *m_newPostModeCombo;
    QTextEdit *m_newPostDescriptionEdit;
};

#endif // EMPRESAWINDOW_H
