#include "registerwindow.h"
#include "api_client.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

RegisterWindow::RegisterWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
}

void RegisterWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Registro de cuenta"), central);
    title->setObjectName("titleLabel");

    auto *subtitle = new QLabel(
        tr("El registro se realiza contra la API real del VPS. Luego se completa el perfil según el tipo de cuenta."),
        central);
    subtitle->setObjectName("subtitleLabel");
    subtitle->setWordWrap(true);

    m_tabs = new QTabWidget(central);

    auto *empresaPage = new QWidget(m_tabs);
    auto *empresaLayout = new QGridLayout(empresaPage);
    empresaLayout->setColumnStretch(1, 1);

    m_empresaNameEdit = new QLineEdit(empresaPage);
    m_empresaNameEdit->setPlaceholderText(tr("Nombre de la empresa"));
    m_empresaUserEdit = new QLineEdit(empresaPage);
    m_empresaUserEdit->setPlaceholderText(tr("Usuario"));
    m_empresaPassEdit = new QLineEdit(empresaPage);
    m_empresaPassEdit->setPlaceholderText(tr("Contraseña"));
    m_empresaPassEdit->setEchoMode(QLineEdit::Password);
    m_empresaEmailEdit = new QLineEdit(empresaPage);
    m_empresaEmailEdit->setPlaceholderText(tr("Email corporativo"));
    m_empresaContactPhoneEdit = new QLineEdit(empresaPage);
    m_empresaContactPhoneEdit->setPlaceholderText(tr("Teléfono de contacto"));
    m_empresaRegisterButton = new QPushButton(tr("Registrar empresa"), empresaPage);
    connect(m_empresaRegisterButton, &QPushButton::clicked, this, &RegisterWindow::registerEmpresa);

    empresaLayout->addWidget(new QLabel(tr("Empresa"), empresaPage), 0, 0);
    empresaLayout->addWidget(m_empresaNameEdit, 0, 1);
    empresaLayout->addWidget(new QLabel(tr("Usuario"), empresaPage), 1, 0);
    empresaLayout->addWidget(m_empresaUserEdit, 1, 1);
    empresaLayout->addWidget(new QLabel(tr("Contraseña"), empresaPage), 2, 0);
    empresaLayout->addWidget(m_empresaPassEdit, 2, 1);
    empresaLayout->addWidget(new QLabel(tr("Email"), empresaPage), 3, 0);
    empresaLayout->addWidget(m_empresaEmailEdit, 3, 1);
    empresaLayout->addWidget(new QLabel(tr("Teléfono"), empresaPage), 4, 0);
    empresaLayout->addWidget(m_empresaContactPhoneEdit, 4, 1);
    empresaLayout->addWidget(m_empresaRegisterButton, 5, 0, 1, 2);

    auto *userPage = new QWidget(m_tabs);
    auto *userLayout = new QGridLayout(userPage);
    userLayout->setColumnStretch(1, 1);

    m_userFirstNameEdit = new QLineEdit(userPage);
    m_userFirstNameEdit->setPlaceholderText(tr("Nombre"));
    m_userLastNameEdit = new QLineEdit(userPage);
    m_userLastNameEdit->setPlaceholderText(tr("Apellido"));
    m_userUsernameEdit = new QLineEdit(userPage);
    m_userUsernameEdit->setPlaceholderText(tr("Usuario"));
    m_userPassEdit = new QLineEdit(userPage);
    m_userPassEdit->setPlaceholderText(tr("Contraseña"));
    m_userPassEdit->setEchoMode(QLineEdit::Password);
    m_userEmailEdit = new QLineEdit(userPage);
    m_userEmailEdit->setPlaceholderText(tr("Email"));
    m_userAgeEdit = new QLineEdit(userPage);
    m_userAgeEdit->setPlaceholderText(tr("Edad"));
    m_userPhoneEdit = new QLineEdit(userPage);
    m_userPhoneEdit->setPlaceholderText(tr("Teléfono"));
    m_userCountryEdit = new QLineEdit(userPage);
    m_userCountryEdit->setPlaceholderText(tr("País"));
    m_userLanguageCombo = new QComboBox(userPage);
    m_userLanguageCombo->addItems({"C++", "Qt", "Python", "Java", "JavaScript", "SQL", "Otro"});
    m_userRegisterButton = new QPushButton(tr("Registrar usuario"), userPage);
    connect(m_userRegisterButton, &QPushButton::clicked, this, &RegisterWindow::registerUser);

    userLayout->addWidget(new QLabel(tr("Nombre"), userPage), 0, 0);
    userLayout->addWidget(m_userFirstNameEdit, 0, 1);
    userLayout->addWidget(new QLabel(tr("Apellido"), userPage), 1, 0);
    userLayout->addWidget(m_userLastNameEdit, 1, 1);
    userLayout->addWidget(new QLabel(tr("Usuario"), userPage), 2, 0);
    userLayout->addWidget(m_userUsernameEdit, 2, 1);
    userLayout->addWidget(new QLabel(tr("Contraseña"), userPage), 3, 0);
    userLayout->addWidget(m_userPassEdit, 3, 1);
    userLayout->addWidget(new QLabel(tr("Email"), userPage), 4, 0);
    userLayout->addWidget(m_userEmailEdit, 4, 1);
    userLayout->addWidget(new QLabel(tr("Edad"), userPage), 5, 0);
    userLayout->addWidget(m_userAgeEdit, 5, 1);
    userLayout->addWidget(new QLabel(tr("Teléfono"), userPage), 6, 0);
    userLayout->addWidget(m_userPhoneEdit, 6, 1);
    userLayout->addWidget(new QLabel(tr("País"), userPage), 7, 0);
    userLayout->addWidget(m_userCountryEdit, 7, 1);
    userLayout->addWidget(new QLabel(tr("Lenguaje principal"), userPage), 8, 0);
    userLayout->addWidget(m_userLanguageCombo, 8, 1);
    userLayout->addWidget(m_userRegisterButton, 9, 0, 1, 2);

    m_tabs->addTab(userPage, tr("Usuario"));
    m_tabs->addTab(empresaPage, tr("Empresa"));

    m_statusLabel = new QLabel(central);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color:#F07178;");

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(m_tabs);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Registro"));
    resize(620, 600);
}

void RegisterWindow::showStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
}

void RegisterWindow::registerEmpresa()
{
    const QString companyName = m_empresaNameEdit->text().trimmed();
    const QString username = m_empresaUserEdit->text().trimmed();
    const QString password = m_empresaPassEdit->text();
    const QString email = m_empresaEmailEdit->text().trimmed();
    const QString phone = m_empresaContactPhoneEdit->text().trimmed();

    if (companyName.isEmpty() || username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        showStatus(tr("Completa empresa, usuario, contraseña y email."));
        return;
    }
    if (!email.contains('@')) {
        showStatus(tr("Ingresa un email válido."));
        return;
    }

    m_empresaRegisterButton->setEnabled(false);
    QString error;

    if (!ApiClient::instance().registerUser("Empresa", username, password, email, companyName, &error)) {
        showStatus(tr("No se pudo registrar la empresa: %1").arg(error));
        m_empresaRegisterButton->setEnabled(true);
        return;
    }

    if (!ApiClient::instance().login(username, password, &error)) {
        showStatus(tr("Empresa creada, pero no se pudo iniciar sesión para completar perfil: %1").arg(error), true);
        m_empresaRegisterButton->setEnabled(true);
        return;
    }

    if (!ApiClient::instance().createCompanyProfile(companyName, phone, &error)) {
        showStatus(tr("Empresa creada, pero falló el perfil: %1").arg(error));
        m_empresaRegisterButton->setEnabled(true);
        return;
    }

    ApiClient::instance().logout();
    showStatus(tr("Empresa registrada correctamente. Ya podés iniciar sesión."), true);
    m_empresaNameEdit->clear();
    m_empresaUserEdit->clear();
    m_empresaPassEdit->clear();
    m_empresaEmailEdit->clear();
    m_empresaContactPhoneEdit->clear();
    m_empresaRegisterButton->setEnabled(true);
}

void RegisterWindow::registerUser()
{
    const QString firstName = m_userFirstNameEdit->text().trimmed();
    const QString lastName = m_userLastNameEdit->text().trimmed();
    const QString username = m_userUsernameEdit->text().trimmed();
    const QString password = m_userPassEdit->text();
    const QString email = m_userEmailEdit->text().trimmed();
    const QString language = m_userLanguageCombo->currentText();
    const QString phone = m_userPhoneEdit->text().trimmed();
    const QString country = m_userCountryEdit->text().trimmed();

    bool ageOk = false;
    const int age = m_userAgeEdit->text().toInt(&ageOk);

    if (firstName.isEmpty() || lastName.isEmpty() || username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        showStatus(tr("Completa nombre, apellido, usuario, contraseña y email."));
        return;
    }
    if (!email.contains('@')) {
        showStatus(tr("Ingresa un email válido."));
        return;
    }
    if (!ageOk || age < 10 || age > 120) {
        showStatus(tr("Ingresa una edad válida."));
        return;
    }

    m_userRegisterButton->setEnabled(false);
    const QString displayName = QString("%1 %2").arg(firstName, lastName);
    QString error;

    if (!ApiClient::instance().registerUser("Usuario", username, password, email, displayName, &error)) {
        showStatus(tr("No se pudo registrar el usuario: %1").arg(error));
        m_userRegisterButton->setEnabled(true);
        return;
    }

    if (!ApiClient::instance().login(username, password, &error)) {
        showStatus(tr("Usuario creado, pero no se pudo iniciar sesión para completar perfil: %1").arg(error), true);
        m_userRegisterButton->setEnabled(true);
        return;
    }

    if (!ApiClient::instance().createEngineerProfile(firstName, lastName, age, phone, country, language, &error)) {
        showStatus(tr("Usuario creado, pero falló el perfil: %1").arg(error));
        m_userRegisterButton->setEnabled(true);
        return;
    }

    ApiClient::instance().logout();
    showStatus(tr("Usuario registrado correctamente. Ya podés iniciar sesión."), true);
    m_userFirstNameEdit->clear();
    m_userLastNameEdit->clear();
    m_userUsernameEdit->clear();
    m_userPassEdit->clear();
    m_userEmailEdit->clear();
    m_userAgeEdit->clear();
    m_userPhoneEdit->clear();
    m_userCountryEdit->clear();
    m_userLanguageCombo->setCurrentIndex(0);
    m_userRegisterButton->setEnabled(true);
}
