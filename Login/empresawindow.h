#ifndef EMPRESAWINDOW_H
#define EMPRESAWINDOW_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QSet>

class QFrame;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class QTimer;

class EmpresaWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EmpresaWindow(const QString &displayName, QWidget *parent = nullptr);

private slots:
    void loadProfile();
    void loadJobs();
    void loadApplications();
    void loadApplicationsForSelectedJob();
    void rankCandidatesForSelectedJob();

    void createJob();
    void acceptSelectedApplication();
    void rejectSelectedApplication();
    void requestCvUpdate();
    void requestMeeting();
    void sendCustomMessage();
    void sendChatMessage();
    void chooseChatPdf();
    void pollNotifications();
    void toggleNotificationPopup();
    void hideNotificationPopup();
    void markSelectedNotificationRead();
    void handleLogout();

private:
    void setupUi(const QString &displayName);

    QWidget *createPerfilPage();
    QWidget *createPublicacionesPage();
    QWidget *createPostulacionesPage();
    QWidget *createNuevaPublicacionPage();
    QWidget *createChatPage();

    void setStatus(const QString &message, bool ok = true);
    void refreshNotifications(bool showPopupForNew);
    void updateNotificationBell(int unreadCount);
    void showNotificationPopup(bool autoHide);
    QString notificationText(const QJsonObject &notification) const;

    qint64 selectedJobId() const;
    qint64 selectedApplicationId() const;

    void populateApplications(const QJsonArray &applications);
    QString formatJobDetail(const QJsonObject &job) const;
    QString formatApplicationDetail(const QJsonObject &application) const;
    QString formatRankingResult(const QJsonObject &response) const;

    void updateSelectedApplicationStatus(const QString &status, const QString &defaultMessage);
    void sendCandidateRequest(const QString &requestType,
                              const QString &defaultTitle,
                              const QString &defaultDetails,
                              const QString &meetingAt = QString());

    QLabel *m_welcomeLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_bellButton = nullptr;
    QFrame *m_notificationPopup = nullptr;
    QListWidget *m_notificationPopupList = nullptr;
    QTimer *m_notificationPollTimer = nullptr;
    QTimer *m_notificationHideTimer = nullptr;
    QSet<qint64> m_seenNotificationIds;
    int m_unreadNotifications = 0;
    QLabel *m_profileLabel = nullptr;
    QLabel *m_publicacionDetailLabel = nullptr;
    QLabel *m_postulacionDetailLabel = nullptr;

    QListWidget *m_menuList = nullptr;
    QListWidget *m_publicacionesList = nullptr;
    QListWidget *m_postulacionesList = nullptr;

    QStackedWidget *m_contentStack = nullptr;

    QTextEdit *m_chatHistory = nullptr;
    QLineEdit *m_chatInput = nullptr;
    QLabel *m_chatPdfLabel = nullptr;
    qint64 m_chatConversationId = 0;
    QString m_chatPdfPath;

    QTextEdit *m_rankingResultText = nullptr;

    QLineEdit *m_newPostTitleEdit = nullptr;
    QLineEdit *m_newPostSkillsEdit = nullptr;
    QLineEdit *m_newPostCityEdit = nullptr;
    QLineEdit *m_newPostCountryEdit = nullptr;
    QLineEdit *m_newPostExperienceEdit = nullptr;
    QComboBox *m_newPostModeCombo = nullptr;
    QTextEdit *m_newPostDescriptionEdit = nullptr;
};

#endif // EMPRESAWINDOW_H
