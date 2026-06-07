#include "mainwindow.h"

#include "adminwindow.h"
#include "api_client.h"
#include "empresawindow.h"
#include "admindb.h"
#include "registerwindow.h"
#include "userwindow.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupLocalDatabase();
    QTimer::singleShot(200, this, &MainWindow::setupAutoLogin);
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(18);

    auto *loginCard = new QFrame(central);
    loginCard->setObjectName("card");
    auto *loginLayout = new QVBoxLayout(loginCard);
    loginLayout->setContentsMargins(28, 28, 28, 28);
    loginLayout->setSpacing(14);

    auto *titleLabel = new QLabel(tr("Iniciar sesión"), loginCard);
    titleLabel->setObjectName("titleLabel");

    m_localDbLabel = new QLabel(loginCard);
    m_localDbLabel->setObjectName("subtitleLabel");
    m_localDbLabel->setWordWrap(true);

    m_loginUserEdit = new QLineEdit(loginCard);
    m_loginUserEdit->setPlaceholderText(tr("Usuario"));

    m_loginPassEdit = new QLineEdit(loginCard);
    m_loginPassEdit->setPlaceholderText(tr("Contraseña"));
    m_loginPassEdit->setEchoMode(QLineEdit::Password);

    m_loginButton = new QPushButton(tr("Entrar"), loginCard);
    connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLogin);
    connect(m_loginPassEdit, &QLineEdit::returnPressed, this, &MainWindow::attemptLogin);

    m_openRegisterButton = new QPushButton(tr("Registrar"), loginCard);
    connect(m_openRegisterButton, &QPushButton::clicked, this, &MainWindow::openRegisterWindow);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(12);
    buttonRow->addWidget(m_loginButton);
    buttonRow->addWidget(m_openRegisterButton);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setStyleSheet("color: #F07178;");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);

    loginLayout->addWidget(titleLabel);
    loginLayout->addWidget(m_localDbLabel);
    loginLayout->addSpacing(10);
    loginLayout->addWidget(m_loginUserEdit);
    loginLayout->addWidget(m_loginPassEdit);
    loginLayout->addLayout(buttonRow);

    mainLayout->addWidget(loginCard);
    mainLayout->addWidget(m_statusLabel);

    setWindowTitle(tr("4eng Login - Qt"));
    resize(560, 480);
}

void MainWindow::setupLocalDatabase()
{
    QString error;
    if (!AdminDB::instance().initialize(&error)) {
        m_localDbLabel->setStyleSheet("color:#F07178;");
        m_localDbLabel->setText(tr("SQLite local no pudo iniciar: %1").arg(error));
        return;
    }

    m_localDbLabel->setStyleSheet("color:#A8E6CF;");
    m_localDbLabel->setText(tr("SQLite local activo: %1").arg(AdminDB::instance().databasePath()));
}

void MainWindow::setupAutoLogin()
{
    QJsonObject cachedUser;
    QString cachedToken;
    QString error;

    if (!AdminDB::instance().getActiveSession(&cachedUser, &cachedToken, &error)) {
        return;
    }

    if (cachedToken.isEmpty() || cachedUser.isEmpty()) {
        return;
    }

    ApiClient::instance().restoreSession(cachedToken, cachedUser);
    AdminDB::instance().updateLastActivity();
    AdminDB::instance().logAction(
        cachedUser.value("username").toString(),
        "auto_login",
        "Sesión recuperada desde SQLite local"
    );

    openWindowByRole(ApiClient::instance().role(), ApiClient::instance().displayName());
}

void MainWindow::showLoginError(const QString &message)
{
    m_statusLabel->setStyleSheet("color: #F07178;");
    m_statusLabel->setText(message);
}

void MainWindow::attemptLogin()
{
    const QString username = m_loginUserEdit->text().trimmed();
    const QString password = m_loginPassEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showLoginError(tr("Completa usuario y contraseña."));
        return;
    }

    QDateTime blockedUntil;
    QString localError;
    if (AdminDB::instance().isUserBlocked(username, &blockedUntil, &localError)) {
        const qint64 seconds = QDateTime::currentDateTimeUtc().secsTo(blockedUntil);
        const int minutes = qMax(1, static_cast<int>((seconds + 59) / 60));
        AdminDB::instance().logAction(username, "login_blocked", tr("Bloqueado por %1 minutos").arg(minutes));
        showLoginError(tr("Usuario bloqueado localmente por intentos fallidos. Probá de nuevo en %1 minutos.").arg(minutes));
        return;
    }

    m_loginButton->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #A8E6CF;");
    m_statusLabel->setText(tr("Conectando con la API..."));

    QString error;
    if (!ApiClient::instance().login(username, password, &error)) {
        AdminDB::instance().logLoginAttempt(username, false);

        int attemptCount = 0;
        QDateTime newBlockedUntil;
        AdminDB::instance().incrementFailedAttempts(username, &attemptCount, &newBlockedUntil);
        AdminDB::instance().logAction(username, "login_failed", error);

        m_loginButton->setEnabled(true);
        if (attemptCount >= 5 && newBlockedUntil.isValid()) {
            showLoginError(tr("No se pudo iniciar sesión: %1. Usuario bloqueado localmente por 15 minutos.").arg(error));
        } else {
            showLoginError(tr("No se pudo iniciar sesión: %1. Intentos fallidos: %2/5").arg(error).arg(attemptCount));
        }
        return;
    }

    AdminDB::instance().logLoginAttempt(username, true);
    AdminDB::instance().resetFailedAttempts(username);
    QString sessionError;
    if (!AdminDB::instance().saveSession(ApiClient::instance().currentUser(), ApiClient::instance().token(), &sessionError)) {
        AdminDB::instance().logAction(username, "session_save_failed", sessionError);
        showLoginError(tr("Login exitoso, pero no se pudo guardar sesión local: %1").arg(sessionError));
    }
    AdminDB::instance().logAction(username, "login", "Login exitoso contra VPS");

    const QString role = ApiClient::instance().role();
    const QString displayName = ApiClient::instance().displayName();
    openWindowByRole(role, displayName);
}

void MainWindow::openRegisterWindow()
{
    auto *registerWindow = new RegisterWindow(this);
    registerWindow->setAttribute(Qt::WA_DeleteOnClose);
    registerWindow->show();
}

void MainWindow::openWindowByRole(const QString &role, const QString &displayName)
{
    if (role == "Admin") {
        auto *adminWindow = new AdminWindow(displayName);
        adminWindow->setAttribute(Qt::WA_DeleteOnClose);
        adminWindow->show();
        close();
        return;
    }

    if (role == "Empresa") {
        auto *empresaWindow = new EmpresaWindow(displayName);
        empresaWindow->setAttribute(Qt::WA_DeleteOnClose);
        empresaWindow->show();
        close();
        return;
    }

    auto *userWindow = new UserWindow(displayName);
    userWindow->setAttribute(Qt::WA_DeleteOnClose);
    userWindow->show();
    close();
}
