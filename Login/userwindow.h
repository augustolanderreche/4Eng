#ifndef USERWINDOW_H
#define USERWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QSet>

class QFrame;
class QLabel;
class QListWidget;
class QStackedWidget;
class QTextEdit;
class QPushButton;
class QLineEdit;
class QTimer;

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
    void pollNotifications();
    void toggleNotificationPopup();
    void hideNotificationPopup();
    void handleLogout();
    void sendChatMessage();
    void chooseChatPdf();

private:
    void setupUi(const QString &displayName);
    QWidget *createProfileTab();
    QWidget *createCvTab();
    QWidget *createJobsTab();
    QWidget *createApplicationsTab();
    QWidget *createNotificationsTab();
    QWidget *createChatTab();
    void setStatus(const QString &message, bool ok = true);
    void refreshNotifications(bool showPopupForNew);
    void updateNotificationBell(int unreadCount);
    void showNotificationPopup(bool autoHide);
    QString notificationText(const QJsonObject &notification) const;
    QString formatCvAnalysis(const QJsonObject &response) const;

    QLabel *m_welcomeLabel;
    QLabel *m_statusLabel;
    QPushButton *m_bellButton;
    QPushButton *m_logoutButton;
    QFrame *m_notificationPopup;
    QLabel *m_profileLabel;
    QLabel *m_jobDetailLabel;
    QLabel *m_applicationDetailLabel;
    QListWidget *m_menuList;
    QListWidget *m_cvList;
    QListWidget *m_jobsList;
    QListWidget *m_applicationsList;
    QListWidget *m_notificationsList;
    QListWidget *m_notificationPopupList;
    QStackedWidget *m_contentStack;
    QTextEdit *m_chatHistory;
    QLineEdit *m_chatInput;
    QLabel *m_chatPdfLabel;
    QLineEdit *m_cvTargetEdit;
    QTextEdit *m_cvAnalysisText;
    QTimer *m_notificationPollTimer;
    QTimer *m_notificationHideTimer;
    QSet<qint64> m_seenNotificationIds;
    int m_unreadNotifications;
    qint64 m_chatConversationId = 0;
    QString m_chatPdfPath;
};

#endif // USERWINDOW_H
