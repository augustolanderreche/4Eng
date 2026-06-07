#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QJsonDocument>

class QLabel;
class QLineEdit;
class QTableWidget;
class QTabWidget;
class QTextEdit;
class QCloseEvent;

class AdminWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminWindow(const QString &displayName, QWidget *parent = nullptr);

private slots:
    void loadDashboard();
    void loadUsers();
    void loadCompanies();
    void loadAccepted();
    void loadAiSettings();
    void loadApplications();
    void loadCvs();
    void loadSystemSummary();
    void loadActivity();
    void saveAiSetting();
    void sendChatMessage();
    void chooseChatPdf();
    void showSelectedUserDetail();
    void showSelectedCompanyDetail();

private:
    void closeEvent(QCloseEvent *event) override;
    void setupUi(const QString &displayName);
    QWidget *createDashboardTab();
    QWidget *createUsersTab();
    QWidget *createCompaniesTab();
    QWidget *createAcceptedTab();
    QWidget *createApplicationsTab();
    QWidget *createCvsTab();
    QWidget *createIaTab();
    QWidget *createChatTab();
    QWidget *createActivityTab();
    QWidget *createSystemTab();

    void setStatus(const QString &message, bool ok = true);
    int selectedId(QTableWidget *table) const;
    void showJsonDialog(const QString &title, const QJsonDocument &document);
    void fillSimpleKeyValueTable(QTableWidget *table, const QJsonObject &object);

    QLabel *m_welcomeLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTabWidget *m_tabs = nullptr;

    QTableWidget *m_dashboardTable = nullptr;
    QTableWidget *m_usersTable = nullptr;
    QTableWidget *m_companiesTable = nullptr;
    QTableWidget *m_acceptedTable = nullptr;
    QTableWidget *m_applicationsTable = nullptr;
    QTableWidget *m_cvsTable = nullptr;
    QTableWidget *m_aiTable = nullptr;
    QTableWidget *m_systemTable = nullptr;
    QTableWidget *m_notificationsTable = nullptr;
    QTableWidget *m_chatTable = nullptr;

    QLineEdit *m_aiKeyEdit = nullptr;
    QTextEdit *m_aiValueEdit = nullptr;

    QTextEdit *m_chatHistory = nullptr;
    QLineEdit *m_chatInput = nullptr;
    QLabel *m_chatPdfLabel = nullptr;
    qint64 m_chatConversationId = 0;
    QString m_chatPdfPath;
};

#endif // ADMINWINDOW_H
