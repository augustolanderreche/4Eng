#include "mainwindow.h"
#include "registerwindow.h"
#include "empresawindow.h"
#include "userwindow.h"
#include "databaseconfig.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupDatabase();
}

MainWindow::~MainWindow() = default;

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

    if (validateCredentials(username, password)) {
        QString role = getRole(username);
        QString displayName = getDisplayName(username);
        if (displayName.isEmpty()) {
            displayName = username;
        }

        if (role == tr("Empresa")) {
            openEmpresaWindow(displayName);
        } else {
            openUserWindow(displayName);
        }
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
