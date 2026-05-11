#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QTabWidget;

class RegisterWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RegisterWindow(QWidget *parent = nullptr);

private slots:
    void registerEmpresa();
    void registerUser();

private:
    void setupUi();
    bool userExists(const QString &username);
    bool createUserRecord(const QString &role,
                          const QString &username,
                          const QString &password,
                          const QString &email,
                          const QString &displayName,
                          const QString &programmingLanguage = QString(),
                          int age = 0);

    QSqlDatabase m_db;
    QLabel *m_statusLabel;

    QTabWidget *m_tabs;
    QLineEdit *m_empresaUserEdit;
    QLineEdit *m_empresaPassEdit;
    QLineEdit *m_empresaEmailEdit;
    QPushButton *m_empresaRegisterButton;

    QLineEdit *m_userFirstNameEdit;
    QLineEdit *m_userLastNameEdit;
    QLineEdit *m_userUsernameEdit;
    QLineEdit *m_userPassEdit;
    QComboBox *m_userLanguageCombo;
    QLineEdit *m_userEmailEdit;
    QLineEdit *m_userAgeEdit;
    QPushButton *m_userRegisterButton;
};

#endif // REGISTERWINDOW_H
