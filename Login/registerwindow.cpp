#include "registerwindow.h"

#include <QComboBox>
#include <QFrame>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

RegisterWindow::RegisterWindow(QWidget *parent)
    : QMainWindow(parent)
{
    if (QSqlDatabase::contains("login_connection")) {
        m_db = QSqlDatabase::database("login_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QMYSQL", "login_connection");
        m_db.setHostName("TU_IP_VPS");
        m_db.setPort(3306);
        m_db.setDatabaseName("nombre_base");
        m_db.setUserName("usuario_db");
        m_db.setPassword("clave_db");
    }
    setupUi();
}

void RegisterWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    auto *titleLabel = new QLabel(tr("Registro de cuentas"), central);
    titleLabel->setObjectName("titleLabel");
    auto *subtitleLabel = new QLabel(tr("Selecciona tu tipo de cuenta y completa tus datos."), central);
    subtitleLabel->setObjectName("subtitleLabel");

    m_tabs = new QTabWidget(central);

    auto *empresaPage = new QWidget;
    auto *empresaForm = new QFormLayout(empresaPage);
    empresaForm->setSpacing(14);
    m_empresaUserEdit = new QLineEdit(empresaPage);
    m_empresaPassEdit = new QLineEdit(empresaPage);
    m_empresaPassEdit->setEchoMode(QLineEdit::Password);
    m_empresaEmailEdit = new QLineEdit(empresaPage);
    m_empresaEmailEdit->setPlaceholderText(tr("empresa@empresa.com"));
    m_empresaRegisterButton = new QPushButton(tr("Registrar empresa"), empresaPage);
    connect(m_empresaRegisterButton, &QPushButton::clicked, this, &RegisterWindow::registerEmpresa);
    empresaForm->addRow(tr("Usuario:"), m_empresaUserEdit);
    empresaForm->addRow(tr("Contraseña:"), m_empresaPassEdit);
    empresaForm->addRow(tr("Mail corporativo:"), m_empresaEmailEdit);
    empresaForm->addRow(QString(), m_empresaRegisterButton);
    empresaPage->setLayout(empresaForm);

    auto *userPage = new QWidget;
    auto *userForm = new QFormLayout(userPage);
    userForm->setSpacing(14);
    m_userFirstNameEdit = new QLineEdit(userPage);
    m_userLastNameEdit = new QLineEdit(userPage);
    m_userUsernameEdit = new QLineEdit(userPage);
    m_userPassEdit = new QLineEdit(userPage);
    m_userPassEdit->setEchoMode(QLineEdit::Password);
    m_userLanguageCombo = new QComboBox(userPage);
    m_userLanguageCombo->addItems({tr("C++"), tr("Python"), tr("Java"), tr("JavaScript"), tr("Rust"), tr("Go"), tr("Otro")});
    m_userEmailEdit = new QLineEdit(userPage);
    m_userEmailEdit->setPlaceholderText(tr("usuario@mail.com"));
    m_userAgeEdit = new QLineEdit(userPage);
    m_userAgeEdit->setValidator(new QIntValidator(10, 120, this));
    m_userRegisterButton = new QPushButton(tr("Registrar usuario"), userPage);
    connect(m_userRegisterButton, &QPushButton::clicked, this, &RegisterWindow::registerUser);
    userForm->addRow(tr("Nombre:"), m_userFirstNameEdit);
    userForm->addRow(tr("Apellido:"), m_userLastNameEdit);
    userForm->addRow(tr("Usuario:"), m_userUsernameEdit);
    userForm->addRow(tr("Contraseña:"), m_userPassEdit);
    userForm->addRow(tr("Lenguaje:"), m_userLanguageCombo);
    userForm->addRow(tr("Mail:"), m_userEmailEdit);
    userForm->addRow(tr("Edad:"), m_userAgeEdit);
    userForm->addRow(QString(), m_userRegisterButton);
    userPage->setLayout(userForm);

    m_tabs->addTab(empresaPage, tr("Empresa"));
    m_tabs->addTab(userPage, tr("Usuario"));

    m_statusLabel = new QLabel(central);
    m_statusLabel->setStyleSheet("color: #F07178;");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(m_tabs);
    mainLayout->addWidget(m_statusLabel);

    setWindowTitle(tr("Registro"));
    resize(520, 620);
}

bool RegisterWindow::userExists(const QString &username)
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

bool RegisterWindow::createUserRecord(const QString &role,
                                      const QString &username,
                                      const QString &password,
                                      const QString &email,
                                      const QString &displayName,
                                      const QString &programmingLanguage,
                                      int age)
{
    if (!m_db.isOpen() && !m_db.open()) {
        m_statusLabel->setText(tr("No se pudo conectar a la base de datos: %1").arg(m_db.lastError().text()));
        return false;
    }

    QSqlQuery query(m_db);
    if (programmingLanguage.isEmpty()) {
        query.prepare("INSERT INTO users (role, username, password, email, display_name) VALUES (:role, :username, :password, :email, :display_name)");
        query.bindValue(":role", role);
        query.bindValue(":username", username);
        query.bindValue(":password", password);
        query.bindValue(":email", email);
        query.bindValue(":display_name", displayName);
    } else {
        query.prepare("INSERT INTO users (role, username, password, email, display_name, programming_language, age) VALUES (:role, :username, :password, :email, :display_name, :programming_language, :age)");
        query.bindValue(":role", role);
        query.bindValue(":username", username);
        query.bindValue(":password", password);
        query.bindValue(":email", email);
        query.bindValue(":display_name", displayName);
        query.bindValue(":programming_language", programmingLanguage);
        query.bindValue(":age", age);
    }

    if (!query.exec()) {
        m_statusLabel->setText(tr("Error al crear usuario: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

void RegisterWindow::registerEmpresa()
{
    const QString username = m_empresaUserEdit->text().trimmed();
    const QString password = m_empresaPassEdit->text();
    const QString email = m_empresaEmailEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        m_statusLabel->setText(tr("Completa todos los campos del registro de empresa."));
        return;
    }
    if (!email.contains('@')) {
        m_statusLabel->setText(tr("Ingresa un correo corporativo válido."));
        return;
    }
    if (userExists(username)) {
        m_statusLabel->setText(tr("El nombre de usuario ya está en uso."));
        return;
    }

    if (createUserRecord(tr("Empresa"), username, password, email, username)) {
        m_statusLabel->setText(tr("Empresa registrada con éxito. Ahora puedes iniciar sesión."));
        m_empresaUserEdit->clear();
        m_empresaPassEdit->clear();
        m_empresaEmailEdit->clear();
    }
}

void RegisterWindow::registerUser()
{
    const QString firstName = m_userFirstNameEdit->text().trimmed();
    const QString lastName = m_userLastNameEdit->text().trimmed();
    const QString username = m_userUsernameEdit->text().trimmed();
    const QString password = m_userPassEdit->text();
    const QString email = m_userEmailEdit->text().trimmed();
    const QString language = m_userLanguageCombo->currentText();
    const int age = m_userAgeEdit->text().toInt();

    if (firstName.isEmpty() || lastName.isEmpty() || username.isEmpty() || password.isEmpty() || email.isEmpty() || m_userAgeEdit->text().isEmpty()) {
        m_statusLabel->setText(tr("Completa todos los campos del registro de usuario."));
        return;
    }
    if (!email.contains('@')) {
        m_statusLabel->setText(tr("Ingresa un correo válido."));
        return;
    }
    if (age < 10 || age > 120) {
        m_statusLabel->setText(tr("Ingresa una edad válida."));
        return;
    }
    if (userExists(username)) {
        m_statusLabel->setText(tr("El nombre de usuario ya está en uso."));
        return;
    }

    const QString displayName = QString("%1 %2").arg(firstName, lastName);
    if (createUserRecord(tr("Usuario"), username, password, email, displayName, language, age)) {
        m_statusLabel->setText(tr("Usuario registrado con éxito. Ahora puedes iniciar sesión."));
        m_userFirstNameEdit->clear();
        m_userLastNameEdit->clear();
        m_userUsernameEdit->clear();
        m_userPassEdit->clear();
        m_userEmailEdit->clear();
        m_userAgeEdit->clear();
        m_userLanguageCombo->setCurrentIndex(0);
    }
}
