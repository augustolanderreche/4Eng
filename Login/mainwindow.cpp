#include "mainwindow.h"
#include "registerwindow.h"
#include "empresawindow.h"
#include "userwindow.h"
#include "databaseconfig.h"
#include "localdbconfig.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), 
      m_currentUserId(-1),
      m_syncTimer(nullptr),
      m_activityTimer(nullptr)
{
    setupUi();
    setupDatabase();
    setupLocalDatabase();
    setupAutoLogin();
    setupBackgroundSync();
}

MainWindow::~MainWindow()
{
    if (m_syncTimer) {
        m_syncTimer->stop();
        m_syncTimer->deleteLater();
    }
    if (m_activityTimer) {
        m_activityTimer->stop();
        m_activityTimer->deleteLater();
    }
    
    // Sincronizar y cerrar sesión
    if (m_localDb.isOpen()) {
        syncOfflineCache();
        LocalDBConfig::closeSession(m_localDb);
    }
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
    auto *subtitleLabel = new QLabel(tr("Ingresa tus credenciales y accede."), loginCard);
    subtitleLabel->setObjectName("subtitleLabel");

    m_loginUserEdit = new QLineEdit(loginCard);
    m_loginUserEdit->setPlaceholderText(tr("Usuario"));
    m_loginPassEdit = new QLineEdit(loginCard);
    m_loginPassEdit->setPlaceholderText(tr("Contraseña"));
    m_loginPassEdit->setEchoMode(QLineEdit::Password);

    m_loginButton = new QPushButton(tr("Entrar"), loginCard);
    connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLogin);

    m_openRegisterButton = new QPushButton(tr("Registrar"), loginCard);
    connect(m_openRegisterButton, &QPushButton::clicked, this, &MainWindow::openRegisterWindow);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(12);
    buttonRow->addWidget(m_loginButton);
    buttonRow->addWidget(m_openRegisterButton);

    loginLayout->addWidget(titleLabel);
    loginLayout->addWidget(subtitleLabel);
    loginLayout->addSpacing(10);
    loginLayout->addWidget(m_loginUserEdit);
    loginLayout->addWidget(m_loginPassEdit);
    loginLayout->addLayout(buttonRow);
    loginCard->setLayout(loginLayout);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setStyleSheet("color: #F07178;");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(loginCard);
    mainLayout->addWidget(m_statusLabel);

    setWindowTitle(tr("Login moderno - QWidgets"));
    resize(520, 760);
}

void MainWindow::setupDatabase()
{
    m_db = DatabaseConfig::getLoginConnection();
}

bool MainWindow::validateCredentials(const QString &username, const QString &password)
{
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            m_statusLabel->setText(tr("No se pudo conectar a la base de datos: %1").arg(m_db.lastError().text()));
            return false;
        }
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :user AND password = :pass");
    query.bindValue(":user", username);
    query.bindValue(":pass", password);

    if (!query.exec()) {
        m_statusLabel->setText(tr("Error en consulta SQL: %1").arg(query.lastError().text()));
        return false;
    }

    if (query.next() && query.value(0).toInt() > 0) {
        return true;
    }

    m_statusLabel->setText(tr("Credenciales inválidas. Revisa usuario y contraseña."));
    return false;
}

QString MainWindow::getDisplayName(const QString &username)
{
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            return QString();
        }
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT display_name FROM users WHERE username = :user LIMIT 1");
    query.bindValue(":user", username);

    if (!query.exec() || !query.next()) {
        return QString();
    }

    return query.value(0).toString();
}

QString MainWindow::getRole(const QString &username)
{
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            return QString();
        }
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT role FROM users WHERE username = :user LIMIT 1");
    query.bindValue(":user", username);

    if (!query.exec() || !query.next()) {
        return QString();
    }

    return query.value(0).toString();
}

bool MainWindow::userExists(const QString &username)
{
    if (!m_db.isOpen() && !m_db.open()) {
        m_statusLabel->setText(tr("No se pudo conectar a la base de datos: %1").arg(m_db.lastError().text()));
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :user");
    query.bindValue(":user", username);

    if (!query.exec() || !query.next()) {
        m_statusLabel->setText(tr("Error al verificar usuario: %1").arg(query.lastError().text()));
        return false;
    }

    return query.value(0).toInt() > 0;
}

void MainWindow::attemptLogin()
{
    const QString username = m_loginUserEdit->text().trimmed();
    const QString password = m_loginPassEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText(tr("Completa todos los campos antes de continuar."));
        return;
    }

    // Verificar si el usuario está bloqueado localmente
    if (!checkAndHandleFailedAttempts(username)) {
        return;
    }

    // Intentar validar contra VPS
    if (validateCredentials(username, password)) {
        QString role = getRole(username);
        QString displayName = getDisplayName(username);
        if (displayName.isEmpty()) {
            displayName = username;
        }

        // Guardar sesión en SQLite local
        m_currentUserId = saveLoginToLocal(username, role, displayName);
        m_currentUsername = username;

        m_statusLabel->setText(tr("Iniciando sesión..."));

        if (role == tr("Empresa")) {
            openEmpresaWindow(displayName);
        } else {
            openUserWindow(displayName);
        }
    } else {
        // Login fallido: incrementar contador de intentos fallidos
        LocalDBConfig::incrementFailedAttempts(m_localDb, username);
        LocalDBConfig::logLoginAttempt(m_localDb, username, false);
        LocalDBConfig::logAction(m_localDb, username, "login_failed", "Intento de login fallido");
    }
}

void MainWindow::openRegisterWindow()
{
    auto *registerWindow = new RegisterWindow(this);
    registerWindow->setAttribute(Qt::WA_DeleteOnClose);
    registerWindow->show();
}

void MainWindow::openEmpresaWindow(const QString &displayName)
{
    auto *empresaWindow = new EmpresaWindow(displayName);
    empresaWindow->setAttribute(Qt::WA_DeleteOnClose);
    empresaWindow->show();
    close();
}

void MainWindow::openUserWindow(const QString &displayName)
{
    auto *userWindow = new UserWindow(displayName);
    userWindow->setAttribute(Qt::WA_DeleteOnClose);
    userWindow->show();
    close();
}

void MainWindow::setupLocalDatabase()
{
    m_localDb = LocalDBConfig::getLocalConnection();
    if (!m_localDb.isOpen()) {
        qWarning() << "Failed to open local database";
        m_statusLabel->setText(tr("Error: No se pudo abrir base de datos local."));
    }
}

void MainWindow::setupAutoLogin()
{
    // Intentar recuperar sesión activa
    QString username, role, displayName;
    if (LocalDBConfig::getActiveSession(m_localDb, username, role, displayName)) {
        m_currentUsername = username;
        qDebug() << "Auto-login successful for:" << username;
        
        if (role == "Empresa") {
            openEmpresaWindow(displayName);
        } else {
            openUserWindow(displayName);
        }
    }
}

void MainWindow::setupBackgroundSync()
{
    // Timer para sincronización cada 5 minutos
    m_syncTimer = new QTimer(this);
    connect(m_syncTimer, &QTimer::timeout, this, &MainWindow::performBackgroundSync);
    m_syncTimer->start(5 * 60 * 1000); // 5 minutos

    // Timer para rastrear inactividad (30 minutos)
    m_activityTimer = new QTimer(this);
    connect(m_activityTimer, &QTimer::timeout, this, [this]() {
        if (!m_currentUsername.isEmpty()) {
            qDebug() << "Sesión expirada por inactividad";
            LocalDBConfig::logAction(m_localDb, m_currentUsername, "session_timeout", "Sesión cerrada por inactividad");
            LocalDBConfig::closeSession(m_localDb);
            m_currentUsername = "";
        }
    });
}

void MainWindow::performBackgroundSync()
{
    if (!m_currentUsername.isEmpty()) {
        syncOfflineCache();
        updateLastActivity();
    }
}

void MainWindow::syncOfflineCache()
{
    // Esta función descargará datos del VPS y los guardará en caché local
    // Se ejecutará en background cada 5 minutos
    
    if (!m_db.isOpen()) {
        return;
    }

    // Aquí iría la lógica de sincronización
    // Por ahora es un placeholder
    qDebug() << "Sincronizando caché offline...";
}

void MainWindow::updateLastActivity()
{
    // Actualizar timestamp de última actividad
    QSqlQuery query(m_localDb);
    query.prepare("UPDATE sessions SET last_activity = datetime('now') WHERE is_active = 1");
    query.exec();
}

void MainWindow::checkAutoLogin()
{
    // Implementado en setupAutoLogin
}

int MainWindow::saveLoginToLocal(const QString &username, const QString &role, const QString &displayName)
{
    // Obtener ID del usuario desde el VPS
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM users WHERE username = :user LIMIT 1");
    query.bindValue(":user", username);

    int userId = -1;
    if (query.exec() && query.next()) {
        userId = query.value(0).toInt();
    }

    if (userId == -1) {
        qWarning() << "Could not get user ID for:" << username;
        return -1;
    }

    // Guardar en SQLite local
    if (LocalDBConfig::saveSession(m_localDb, userId, username, role, displayName)) {
        LocalDBConfig::logLoginAttempt(m_localDb, username, true);
        LocalDBConfig::resetFailedAttempts(m_localDb, username);
        LocalDBConfig::logAction(m_localDb, username, "login", "Login exitoso");
        return userId;
    }

    return -1;
}

bool MainWindow::checkAndHandleFailedAttempts(const QString &username)
{
    QString blockReason;
    if (LocalDBConfig::isUserBlocked(m_localDb, username, blockReason)) {
        m_statusLabel->setText(blockReason);
        LocalDBConfig::logAction(m_localDb, username, "login_blocked", blockReason);
        return false;
    }

    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Sincronizar datos antes de cerrar
    if (m_localDb.isOpen() && !m_currentUsername.isEmpty()) {
        syncOfflineCache();
        LocalDBConfig::logAction(m_localDb, m_currentUsername, "app_closed", "Aplicación cerrada");
        LocalDBConfig::closeSession(m_localDb);
    }

    event->accept();
}
