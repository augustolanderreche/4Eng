#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

class QLabel;
class QLineEdit;
class QPushButton;
class QFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void attemptLogin();
    void openRegisterWindow();

private:
    void setupUi();
    void setupDatabase();
    bool validateCredentials(const QString &username, const QString &password);
    QString getDisplayName(const QString &username);
    QString getRole(const QString &username);
    bool userExists(const QString &username);
    bool createUserRecord(const QString &role,
                          const QString &username,
                          const QString &password,
                          const QString &email,
                          const QString &displayName,
                          const QString &programmingLanguage = QString(),
                          int age = 0);
    void openEmpresaWindow(const QString &displayName);
    void openUserWindow(const QString &displayName);

    QSqlDatabase m_db;
    QLabel *m_statusLabel;

    QLineEdit *m_loginUserEdit;
    QLineEdit *m_loginPassEdit;
    QPushButton *m_loginButton;
    QPushButton *m_openRegisterButton;
};

#endif // MAINWINDOW_H
