#include "mainwindow.h"

#include "adminwindow.h"
#include "api_client.h"
#include "empresawindow.h"
#include "localdbmanager.h"
#include "registerwindow.h"
#include "userwindow.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonDocument>
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

    auto *subtitleLabel = new QLabel(
        tr("Conexión real a la API del VPS. Demo: user_demo/user123, empresa_demo/empresa123, admin_demo/admin123"),
        loginCard);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setWordWrap(true);

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
    loginLayout->addWidget(subtitleLabel);
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
    if (!LocalDbManager::instance().initialize(&error)) {
        m_localDbLabel->setStyleSheet("color:#F07178;");
        m_localDbLabel->setText(tr("SQLite local no pudo iniciar: %1").arg(error));
        return;
    }

    m_localDbLabel->setStyleSheet("color:#A8E6CF;");
    m_localDbLabel->setText(tr("SQLite local activo: %1").arg(LocalDbManager::instance().databasePath()));
}

void MainWindow::setupAutoLogin()
{
    QJsonObject cachedUser;
    QString cachedToken;
    QString error;

    if (!LocalDbManager::instance().getActiveSession(&cachedUser, &cachedToken, &error)) {
        return;
    }

    if (cachedToken.isEmpty() || cachedUser.isEmpty()) {
        return;
    }

    ApiClient::instance().restoreSession(cachedToken, cachedUser);

    // Validamos el token contra el VPS. Si venció, pedimos login normal.
    bool ok = false;
    const QJsonDocument doc = ApiClient::instance().get("/auth/me", &ok, &error);
    if (!ok || !doc.isObject()) {
        const QString lowered = error.toLower();
        const bool invalidToken = lowered.contains("401")
                                  || lowered.contains("403")
                                  || lowered.contains("unauthorized")
                                  || lowered.contains("forbidden")
                                  || lowered.contains("not authenticated")
                                  || lowered.contains("expired")
                                  || lowered.contains(QString::fromUtf8("venci"))
                                  || lowered.contains("token inval")
                                  || lowered.contains("invalid token");

        if (invalidToken) {
            LocalDbManager::instance().logAction(cachedUser.value("username").toString(),
                                                 "session_expired",
                                                 error);
            LocalDbManager::instance().closeActiveSession();
            ApiClient::instance().logout();
            showLoginError(tr("La sesión local existe, pero el token venció. Iniciá sesión nuevamente."));
            return;
        }

        // Si falla la red o el VPS no responde, conservamos la sesión local.
        LocalDbManager::instance().logAction(cachedUser.value("username").toString(),
                                             "auto_login_offline",
                                             error);
        LocalDbManager::instance().updateLastActivity();
        openWindowByRole(ApiClient::instance().role(), ApiClient::instance().displayName());
        return;
    }

    const QJsonObject apiUser = doc.object();
    QJsonObject userForSession = cachedUser;
    for (auto it = apiUser.begin(); it != apiUser.end(); ++it) {
        userForSession.insert(it.key(), it.value());
    }

    ApiClient::instance().restoreSession(cachedToken, userForSession);
    LocalDbManager::instance().updateLastActivity();
    LocalDbManager::instance().logAction(ApiClient::instance().username(), "auto_login", "Sesión recuperada desde SQLite local");

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
    if (LocalDbManager::instance().isUserBlocked(username, &blockedUntil, &localError)) {
        const qint64 seconds = QDateTime::currentDateTimeUtc().secsTo(blockedUntil);
        const int minutes = qMax(1, static_cast<int>((seconds + 59) / 60));
        LocalDbManager::instance().logAction(username, "login_blocked", tr("Bloqueado por %1 minutos").arg(minutes));
        showLoginError(tr("Usuario bloqueado localmente por intentos fallidos. Probá de nuevo en %1 minutos.").arg(minutes));
        return;
    }

    m_loginButton->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #A8E6CF;");
    m_statusLabel->setText(tr("Conectando con la API..."));

    QString error;
    if (!ApiClient::instance().login(username, password, &error)) {
        LocalDbManager::instance().logLoginAttempt(username, false);

        int attemptCount = 0;
        QDateTime newBlockedUntil;
        LocalDbManager::instance().incrementFailedAttempts(username, &attemptCount, &newBlockedUntil);
        LocalDbManager::instance().logAction(username, "login_failed", error);

        m_loginButton->setEnabled(true);
        if (attemptCount >= 5 && newBlockedUntil.isValid()) {
            showLoginError(tr("No se pudo iniciar sesión: %1. Usuario bloqueado localmente por 15 minutos.").arg(error));
        } else {
            showLoginError(tr("No se pudo iniciar sesión: %1. Intentos fallidos: %2/5").arg(error).arg(attemptCount));
        }
        return;
    }

    LocalDbManager::instance().logLoginAttempt(username, true);
    LocalDbManager::instance().resetFailedAttempts(username);

    QString sessionError;
    if (!LocalDbManager::instance().saveSession(ApiClient::instance().currentUser(),
                                                ApiClient::instance().token(),
                                                &sessionError)) {
        m_loginButton->setEnabled(true);
        showLoginError(tr("Login correcto, pero no se pudo guardar la sesión local SQLite: %1").arg(sessionError));
        return;
    }

    LocalDbManager::instance().logAction(username, "login", "Login exitoso contra VPS");

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
