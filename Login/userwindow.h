#ifndef USERWINDOW_H
#define USERWINDOW_H

#include <QMainWindow>

class QLabel;
class QListWidget;
class QStackedWidget;
class QTextEdit;
class QPushButton;
class QLineEdit;

class UserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UserWindow(const QString &displayName, QWidget *parent = nullptr);

private slots:
    void loadProfile();
    void loadJobs();
    void applyToSelectedJob();
    void uploadCv();
    void analyzeCv();
    void loadApplications();
    void loadNotifications();
    void markSelectedNotificationRead();
    void sendChatMessage();
    void handleLogout();

private:
    void setupUi(const QString &displayName);
    QWidget *createProfileTab();
    QWidget *createCvTab();
    QWidget *createJobsTab();
    QWidget *createApplicationsTab();
    QWidget *createNotificationsTab();
    QWidget *createChatTab();
    void setStatus(const QString &message, bool ok = true);
    QString formatCvAnalysis(const QJsonObject &response) const;

    QLabel *m_welcomeLabel;
    QLabel *m_statusLabel;
    QLabel *m_profileLabel;
    QLabel *m_jobDetailLabel;
    QLabel *m_applicationDetailLabel;
    QListWidget *m_menuList;
    QListWidget *m_cvList;
    QListWidget *m_jobsList;
    QListWidget *m_applicationsList;
    QListWidget *m_notificationsList;
    QStackedWidget *m_contentStack;
    QTextEdit *m_chatHistory;
    QLineEdit *m_chatInput;
    QLineEdit *m_cvTargetEdit;
    QTextEdit *m_cvAnalysisText;
};

#endif // USERWINDOW_H
