#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

class QLabel;
class QLineEdit;
class QPushButton;
class QFrame;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void attemptLogin();
    void openRegisterWindow();
    void performBackgroundSync();
    void checkAutoLogin();

private:
    void setupUi();
    void setupDatabase();
    void setupLocalDatabase();
    void setupAutoLogin();
    void setupBackgroundSync();
    void syncOfflineCache();
    
    bool validateCredentials(const QString &username, const QString &password);
    bool validateHardcodedDemoLogin(const QString &username, const QString &password,
                                    QString &role, QString &displayName);
    QString getDisplayName(const QString &username);
    QString getRole(const QString &username);
    bool userExists(const QString &username);
    void openAdminWindow(const QString &displayName);
    void openEmpresaWindow(const QString &displayName);
    void openUserWindow(const QString &displayName);

    // Local DB
    int saveLoginToLocal(const QString &username, const QString &role, const QString &displayName);
    bool checkAndHandleFailedAttempts(const QString &username);
    void updateLastActivity();

    QSqlDatabase m_db;           // VPS MySQL
    QSqlDatabase m_localDb;      // Local SQLite
    QLabel *m_statusLabel;

    QLineEdit *m_loginUserEdit;
    QLineEdit *m_loginPassEdit;
    QPushButton *m_loginButton;
    QPushButton *m_openRegisterButton;

    QString m_currentUsername;   // Usuario actualmente logueado
    int m_currentUserId;         // ID del usuario en VPS
    
    QTimer *m_syncTimer;         // Timer para sincronización automática
    QTimer *m_activityTimer;     // Timer para rastrear inactividad
};

#endif // MAINWINDOW_H
