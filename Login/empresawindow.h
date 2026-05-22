#ifndef EMPRESAWINDOW_H
#define EMPRESAWINDOW_H

#include <QMainWindow>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLineEdit;
class QTextEdit;

class EmpresaWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EmpresaWindow(const QString &displayName, QWidget *parent = nullptr);

private:
    void setupUi(const QString &displayName);
    QWidget *createPerfilPage();
    QWidget *createPublicacionesPage();
    QWidget *createPostulacionesPage();
    QWidget *createCvsPage();
    QWidget *createSolicitudesPage();
    QWidget *createChatPage();
    QWidget *createNuevaPublicacionPage();
    void hideNewPublicationMenu();

    QLabel *m_welcomeLabel;
    QLabel *m_publicacionDetailLabel;
    QLabel *m_postulacionDetailLabel;
    QLabel *m_cvDetailLabel;
    QLabel *m_solicitudDetailLabel;
    QListWidget *m_menuList;
    QListWidgetItem *m_newPostMenuItem;
    QListWidget *m_publicacionesList;
    QListWidget *m_postulacionesList;
    QListWidget *m_cvsList;
    QListWidget *m_solicitudesList;
    QStackedWidget *m_contentStack;
    QTextEdit *m_chatHistory;
    QLineEdit *m_chatInput;

    QLineEdit *m_newPostTitleEdit;
    QLineEdit *m_newPostSkillsEdit;
    QLineEdit *m_newPostLocationEdit;
    QLineEdit *m_newPostModeEdit;
    QTextEdit *m_newPostDescriptionEdit;
    int m_newPostPageIndex;
};

#endif // EMPRESAWINDOW_H