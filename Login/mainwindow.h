#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void attemptLogin();
    void openRegisterWindow();

private:
    void setupUi();
    void setupLocalDatabase();
    void setupAutoLogin();
    void openWindowByRole(const QString &role, const QString &displayName);
    void showLoginError(const QString &message);

    QLabel *m_statusLabel;
    QLabel *m_localDbLabel;
    QLineEdit *m_loginUserEdit;
    QLineEdit *m_loginPassEdit;
    QPushButton *m_loginButton;
    QPushButton *m_openRegisterButton;
};

#endif // MAINWINDOW_H
