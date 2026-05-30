#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QTextEdit;

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
    void saveAiSetting();

private:
    void setupUi(const QString &displayName);
    QWidget *createDashboardTab();
    QWidget *createUsersTab();
    QWidget *createCompaniesTab();
    QWidget *createAcceptedTab();
    QWidget *createIaTab();
    QWidget *createSystemTab();
    void setStatus(const QString &message, bool ok = true);

    QLabel *m_welcomeLabel;
    QLabel *m_statusLabel;
    QTabWidget *m_tabs;

    QTableWidget *m_dashboardTable;
    QTableWidget *m_usersTable;
    QTableWidget *m_companiesTable;
    QTableWidget *m_acceptedTable;
    QTableWidget *m_aiTable;
    QLineEdit *m_aiKeyEdit;
    QTextEdit *m_aiValueEdit;
};

#endif // ADMINWINDOW_H
