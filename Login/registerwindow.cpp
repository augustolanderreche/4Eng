#include "registerwindow.h"
#include "databaseconfig.h"

#include <QComboBox>
#include <QFormLayout>
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
    m_db = DatabaseConfig::getLoginConnection();
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
    m_empresaNameEdit = new QLineEdit(empresaPage);
    m_empresaUserEdit = new QLineEdit(empresaPage);
    m_empresaPassEdit = new QLineEdit(empresaPage);
    m_empresaPassEdit->setEchoMode(QLineEdit::Password);
    m_empresaEmailEdit = new QLineEdit(empresaPage);
    m_empresaEmailEdit->setPlaceholderText(tr("empresa@empresa.com"));
    m_empresaContactPhoneEdit = new QLineEdit(empresaPage);
    m_empresaRegisterButton = new QPushButton(tr("Registrar empresa"), empresaPage);
    connect(m_empresaRegisterButton, &QPushButton::clicked, this, &RegisterWindow::registerEmpresa);
    empresaForm->addRow(tr("Empresa:"), m_empresaNameEdit);
    empresaForm->addRow(tr("Usuario:"), m_empresaUserEdit);
    empresaForm->addRow(tr("Contraseña:"), m_empresaPassEdit);
    empresaForm->addRow(tr("Mail corporativo:"), m_empresaEmailEdit);
    empresaForm->addRow(tr("Teléfono contacto:"), m_empresaContactPhoneEdit);
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
    m_userPhoneEdit = new QLineEdit(userPage);
    m_userCountryEdit = new QLineEdit(userPage);
    m_userRegisterButton = new QPushButton(tr("Registrar usuario"), userPage);
    connect(m_userRegisterButton, &QPushButton::clicked, this, &RegisterWindow::registerUser);
    userForm->addRow(tr("Nombre:"), m_userFirstNameEdit);
    userForm->addRow(tr("Apellido:"), m_userLastNameEdit);
    userForm->addRow(tr("Usuario:"), m_userUsernameEdit);
    userForm->addRow(tr("Contraseña:"), m_userPassEdit);
    userForm->addRow(tr("Lenguaje:"), m_userLanguageCombo);
    userForm->addRow(tr("Mail:"), m_userEmailEdit);
    userForm->addRow(tr("Edad:"), m_userAgeEdit);
    userForm->addRow(tr("Teléfono:"), m_userPhoneEdit);
    userForm->addRow(tr("País:"), m_userCountryEdit);
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
    resize(640, 760);
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

bool RegisterWindow::createEmpresaRecord(const QString &username,
                                         const QString &password,
                                         const QString &email,
                                         const QString &displayName,
                                         const QString &companyName,
                                         const QString &contactPhone)
{
    if (!m_db.isOpen() && !m_db.open()) {
        m_statusLabel->setText(tr("No se pudo conectar a la base de datos: %1").arg(m_db.lastError().text()));
        return false;
    }

    if (!m_db.transaction()) {
        m_statusLabel->setText(tr("No se pudo iniciar transacción: %1").arg(m_db.lastError().text()));
        return false;
    }

    QSqlQuery userQuery(m_db);
    userQuery.prepare("INSERT INTO users (role, username, email, password_hash, display_name, is_active, is_online) "
                      "VALUES (:role, :username, :email, :password_hash, :display_name, 1, 0)");
    userQuery.bindValue(":role", "Empresa");
    userQuery.bindValue(":username", username);
    userQuery.bindValue(":email", email);
    userQuery.bindValue(":password_hash", password);
    userQuery.bindValue(":display_name", displayName);

    if (!userQuery.exec()) {
        m_db.rollback();
        m_statusLabel->setText(tr("Error al crear usuario empresa: %1").arg(userQuery.lastError().text()));
        return false;
    }

    const qulonglong userId = userQuery.lastInsertId().toULongLong();
    QSqlQuery profileQuery(m_db);
    profileQuery.prepare("INSERT INTO company_profiles (user_id, company_name, contact_phone) "
                         "VALUES (:user_id, :company_name, :contact_phone)");
    profileQuery.bindValue(":user_id", static_cast<qlonglong>(userId));
    profileQuery.bindValue(":company_name", companyName);
    profileQuery.bindValue(":contact_phone", contactPhone.isEmpty() ? QVariant(QVariant::String) : contactPhone);

    if (!profileQuery.exec()) {
        m_db.rollback();
        m_statusLabel->setText(tr("Error al crear perfil de empresa: %1").arg(profileQuery.lastError().text()));
        return false;
    }

    if (!m_db.commit()) {
        m_statusLabel->setText(tr("No se pudo confirmar la transacción: %1").arg(m_db.lastError().text()));
        return false;
    }

    return true;
}

bool RegisterWindow::createEngineerRecord(const QString &username,
                                          const QString &password,
                                          const QString &email,
                                          const QString &displayName,
                                          const QString &firstName,
                                          const QString &lastName,
                                          int age,
                                          const QString &phone,
                                          const QString &country,
                                          const QString &programmingLanguage)
{
    if (!m_db.isOpen() && !m_db.open()) {
        m_statusLabel->setText(tr("No se pudo conectar a la base de datos: %1").arg(m_db.lastError().text()));
        return false;
    }

    if (!m_db.transaction()) {
        m_statusLabel->setText(tr("No se pudo iniciar transacción: %1").arg(m_db.lastError().text()));
        return false;
    }

    QSqlQuery userQuery(m_db);
    userQuery.prepare("INSERT INTO users (role, username, email, password_hash, display_name, is_active, is_online) "
                      "VALUES (:role, :username, :email, :password_hash, :display_name, 1, 0)");
    userQuery.bindValue(":role", "Usuario");
    userQuery.bindValue(":username", username);
    userQuery.bindValue(":email", email);
    userQuery.bindValue(":password_hash", password);
    userQuery.bindValue(":display_name", displayName);

    if (!userQuery.exec()) {
        m_db.rollback();
        m_statusLabel->setText(tr("Error al crear usuario ingeniero: %1").arg(userQuery.lastError().text()));
        return false;
    }

    const qulonglong userId = userQuery.lastInsertId().toULongLong();
    QSqlQuery profileQuery(m_db);
    profileQuery.prepare("INSERT INTO engineer_profiles "
                         "(user_id, first_name, last_name, age, phone, country, main_programming_language) "
                         "VALUES (:user_id, :first_name, :last_name, :age, :phone, :country, :language)");
    profileQuery.bindValue(":user_id", static_cast<qlonglong>(userId));
    profileQuery.bindValue(":first_name", firstName);
    profileQuery.bindValue(":last_name", lastName);
    profileQuery.bindValue(":age", age);
    profileQuery.bindValue(":phone", phone.isEmpty() ? QVariant(QVariant::String) : phone);
    profileQuery.bindValue(":country", country.isEmpty() ? QVariant(QVariant::String) : country);
    profileQuery.bindValue(":language", programmingLanguage.isEmpty() ? QVariant(QVariant::String) : programmingLanguage);

    if (!profileQuery.exec()) {
        m_db.rollback();
        m_statusLabel->setText(tr("Error al crear perfil de ingeniero: %1").arg(profileQuery.lastError().text()));
        return false;
    }

    if (!m_db.commit()) {
        m_statusLabel->setText(tr("No se pudo confirmar la transacción: %1").arg(m_db.lastError().text()));
        return false;
    }

    return true;
}

void RegisterWindow::registerEmpresa()
{
    const QString companyName = m_empresaNameEdit->text().trimmed();
    const QString username = m_empresaUserEdit->text().trimmed();
    const QString password = m_empresaPassEdit->text();
    const QString email = m_empresaEmailEdit->text().trimmed();
    const QString contactPhone = m_empresaContactPhoneEdit->text().trimmed();

    if (companyName.isEmpty() || username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        m_statusLabel->setText(tr("Completa empresa, usuario, contraseña y mail corporativo."));
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

    if (createEmpresaRecord(username, password, email, companyName, companyName, contactPhone)) {
        m_statusLabel->setText(tr("Empresa registrada según esquema simplificado. Usuario y perfil creados."));
        m_empresaNameEdit->clear();
        m_empresaUserEdit->clear();
        m_empresaPassEdit->clear();
        m_empresaEmailEdit->clear();
        m_empresaContactPhoneEdit->clear();
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
    const QString phone = m_userPhoneEdit->text().trimmed();
    const QString country = m_userCountryEdit->text().trimmed();

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
    if (createEngineerRecord(username, password, email, displayName, firstName, lastName, age, phone, country, language)) {
        m_statusLabel->setText(tr("Usuario registrado según esquema simplificado. Usuario y perfil creados."));
        m_userFirstNameEdit->clear();
        m_userLastNameEdit->clear();
        m_userUsernameEdit->clear();
        m_userPassEdit->clear();
        m_userEmailEdit->clear();
        m_userAgeEdit->clear();
        m_userPhoneEdit->clear();
        m_userCountryEdit->clear();
        m_userLanguageCombo->setCurrentIndex(0);
    }
}
