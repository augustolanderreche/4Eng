#ifndef USERWINDOW_H
#define USERWINDOW_H

#include <QMainWindow>

class QLabel;
class QListWidget;
class QStackedWidget;
class QTextEdit;
class QPushButton;
class QFrame;
class QToolButton;
class QWidget;
class QLineEdit;

class UserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UserWindow(const QString &displayName, QWidget *parent = nullptr);

private:
    void setupUi(const QString &displayName);
    QWidget *createProfileTab();
    QWidget *createCvTab();
    QWidget *createJobsTab();
    QWidget *createApplicationsTab();
    QWidget *createRequestsTab();
    QWidget *createRecommendationsTab();
    QWidget *createChatTab();
    QFrame *createNotificationPopup();
    void showNotificationsPopup();
    void pushNotification(const QString &text);

    QLabel *m_welcomeLabel;
    QLabel *m_jobDetailLabel;
    QLabel *m_applicationDetailLabel;
    QListWidget *m_menuList;
    QListWidget *m_cvList;
    QListWidget *m_jobsList;
    QListWidget *m_applicationsList;
    QStackedWidget *m_contentStack;
    QTextEdit *m_chatHistory;
    QLineEdit *m_chatInput;
    QToolButton *m_bellButton;
    QFrame *m_notificationsPopup;
    QListWidget *m_notificationsList;
};

#endif