#include "empresawindow.h"
#include "api_client.h"
#include "localdbmanager.h"
#include "mainwindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

static QString toText(const QJsonObject &obj, const QString &key, const QString &fallback = "-")
{
    const QJsonValue value = obj.value(key);
    if (value.isString()) {
        return value.toString().isEmpty() ? fallback : value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toVariant().toLongLong());
    }
    if (value.isBool()) {
        return value.toBool() ? "Sí" : "No";
    }
    if (value.isArray() || value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.isArray() ? QJsonDocument(value.toArray()) : QJsonDocument(value.toObject())).toJson(QJsonDocument::Compact));
    }
    return fallback;
}

EmpresaWindow::EmpresaWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
    loadProfile();
    loadJobs();
    loadApplications();
}

void EmpresaWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    m_welcomeLabel = new QLabel(tr("Panel Empresa - %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #FFFFFF;");

    auto *logoutButton = new QPushButton(tr("Cerrar sesión"), central);
    logoutButton->setObjectName("logoutButton");
    connect(logoutButton, &QPushButton::clicked, this, &EmpresaWindow::handleLogout);

    auto *headerRow = new QHBoxLayout;
    headerRow->addWidget(m_welcomeLabel, 1);
    headerRow->addWidget(logoutButton);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    auto *contentRow = new QHBoxLayout;
    m_menuList = new QListWidget(central);
    m_menuList->addItems({tr("Perfil"), tr("Publicaciones"), tr("Postulaciones"), tr("Nueva publicación"), tr("Chat IA")});
    m_menuList->setCurrentRow(0);
    m_menuList->setFixedWidth(220);

    m_contentStack = new QStackedWidget(central);
    m_contentStack->addWidget(createPerfilPage());
    m_contentStack->addWidget(createPublicacionesPage());
    m_contentStack->addWidget(createPostulacionesPage());
    m_contentStack->addWidget(createNuevaPublicacionPage());
    m_contentStack->addWidget(createChatPage());

    connect(m_menuList, &QListWidget::currentRowChanged, m_contentStack, &QStackedWidget::setCurrentIndex);

    contentRow->addWidget(m_menuList);
    contentRow->addWidget(m_contentStack, 1);

    layout->addLayout(headerRow);
    layout->addLayout(contentRow, 1);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Panel empresa"));
    resize(1080, 720);
}

void EmpresaWindow::setStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
}

QWidget *EmpresaWindow::createPerfilPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar perfil"), page);
    connect(reload, &QPushButton::clicked, this, &EmpresaWindow::loadProfile);
    m_profileLabel = new QLabel(page);
    m_profileLabel->setWordWrap(true);
    layout->addWidget(reload);
    layout->addWidget(m_profileLabel);
    layout->addStretch();
    return page;
}

QWidget *EmpresaWindow::createPublicacionesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar publicaciones"), page);
    connect(reload, &QPushButton::clicked, this, &EmpresaWindow::loadJobs);
    m_publicacionesList = new QListWidget(page);
    m_publicacionDetailLabel = new QLabel(tr("Seleccioná una publicación para ver detalle."), page);
    m_publicacionDetailLabel->setWordWrap(true);

    connect(m_publicacionesList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (current) {
            m_publicacionDetailLabel->setText(current->data(Qt::UserRole + 1).toString());
        }
    });

    layout->addWidget(reload);
    layout->addWidget(m_publicacionesList, 1);
    layout->addWidget(m_publicacionDetailLabel);
    return page;
}

QWidget *EmpresaWindow::createPostulacionesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar postulaciones"), page);
    connect(reload, &QPushButton::clicked, this, &EmpresaWindow::loadApplications);
    m_postulacionesList = new QListWidget(page);
    m_postulacionDetailLabel = new QLabel(tr("Seleccioná una postulación."), page);
    m_postulacionDetailLabel->setWordWrap(true);

    connect(m_postulacionesList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (current) {
            m_postulacionDetailLabel->setText(current->data(Qt::UserRole + 1).toString());
        }
    });

    layout->addWidget(reload);
    layout->addWidget(m_postulacionesList, 1);
    layout->addWidget(m_postulacionDetailLabel);
    return page;
}

QWidget *EmpresaWindow::createNuevaPublicacionPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    m_newPostTitleEdit = new QLineEdit(page);
    m_newPostTitleEdit->setPlaceholderText(tr("Título del puesto"));
    m_newPostSkillsEdit = new QLineEdit(page);
    m_newPostSkillsEdit->setPlaceholderText(tr("Skills separadas por coma. Ej: C++, Qt, SQL"));
    m_newPostExperienceEdit = new QLineEdit(page);
    m_newPostExperienceEdit->setPlaceholderText(tr("Años mínimos de experiencia. Ej: 1.5"));
    m_newPostModeCombo = new QComboBox(page);
    m_newPostModeCombo->addItems({"Remoto", "Hibrido", "Presencial"});
    m_newPostCityEdit = new QLineEdit(page);
    m_newPostCityEdit->setPlaceholderText(tr("Ciudad"));
    m_newPostCountryEdit = new QLineEdit(page);
    m_newPostCountryEdit->setPlaceholderText(tr("País"));
    m_newPostDescriptionEdit = new QTextEdit(page);
    m_newPostDescriptionEdit->setPlaceholderText(tr("Descripción del puesto"));

    auto *create = new QPushButton(tr("Crear publicación real"), page);
    connect(create, &QPushButton::clicked, this, &EmpresaWindow::createJob);

    layout->addWidget(m_newPostTitleEdit);
    layout->addWidget(m_newPostSkillsEdit);
    layout->addWidget(m_newPostExperienceEdit);
    layout->addWidget(m_newPostModeCombo);
    layout->addWidget(m_newPostCityEdit);
    layout->addWidget(m_newPostCountryEdit);
    layout->addWidget(m_newPostDescriptionEdit, 1);
    layout->addWidget(create);
    return page;
}

QWidget *EmpresaWindow::createChatPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_chatHistory = new QTextEdit(page);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setPlainText(tr("Chat IA visual. Falta endpoint de chat en FastAPI para hacerlo real."));
    m_chatInput = new QLineEdit(page);
    m_chatInput->setPlaceholderText(tr("Mensaje..."));
    auto *send = new QPushButton(tr("Enviar"), page);
    connect(send, &QPushButton::clicked, this, &EmpresaWindow::sendChatMessage);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &EmpresaWindow::sendChatMessage);
    auto *row = new QHBoxLayout;
    row->addWidget(m_chatInput, 1);
    row->addWidget(send);
    layout->addWidget(m_chatHistory, 1);
    layout->addLayout(row);
    return page;
}

void EmpresaWindow::handleLogout()
{
    QString error;
    const QString username = ApiClient::instance().username();

    if (!username.isEmpty()) {
        LocalDbManager::instance().logAction(
            username,
            QStringLiteral("logout"),
            QStringLiteral("Cierre de sesión manual desde panel Empresa"),
            &error
        );
    }

    LocalDbManager::instance().closeActiveSession(&error);
    ApiClient::instance().logout();

    auto *loginWindow = new MainWindow();
    loginWindow->show();
    close();
}

void EmpresaWindow::loadProfile()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/users/profile", &ok, &error);
    if (!ok || !doc.isObject()) {
        setStatus(tr("Perfil: %1").arg(error), false);
        return;
    }
    const QJsonObject o = doc.object();
    m_profileLabel->setText(tr("ID: %1\nUsuario: %2\nNombre visible: %3\nRol: %4\nEmail: %5\nÚltimo login: %6")
                            .arg(toText(o, "id"),
                                 toText(o, "username"),
                                 toText(o, "display_name"),
                                 toText(o, "role"),
                                 toText(o, "email"),
                                 toText(o, "last_login")));
    setStatus(tr("Perfil de empresa actualizado desde la API."));
}

void EmpresaWindow::loadJobs()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/jobs/list", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Publicaciones: %1").arg(error), false);
        return;
    }

    m_publicacionesList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        auto *item = new QListWidgetItem(tr("%1\n%2 | %3 | %4")
                                             .arg(toText(o, "title"), toText(o, "location_mode"), toText(o, "country"), toText(o, "status")));
        item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)));
        m_publicacionesList->addItem(item);
    }
    setStatus(tr("Publicaciones actualizadas desde la API."));
}

void EmpresaWindow::loadApplications()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/company/applications/my", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Postulaciones empresa: %1").arg(error), false);
        return;
    }

    m_postulacionesList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        auto *item = new QListWidgetItem(tr("Postulación #%1\nPuesto ID: %2\nUsuario ID: %3\nEstado: %4")
                                             .arg(toText(o, "id"), toText(o, "job_post_id"), toText(o, "engineer_user_id"), toText(o, "status")));
        item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)));
        m_postulacionesList->addItem(item);
    }
    setStatus(tr("Postulaciones actualizadas desde la API."));
}

void EmpresaWindow::createJob()
{
    const QString title = m_newPostTitleEdit->text().trimmed();
    const QString description = m_newPostDescriptionEdit->toPlainText().trimmed();

    if (title.isEmpty() || description.isEmpty()) {
        setStatus(tr("Completá título y descripción."), false);
        return;
    }

    QJsonArray skills;
    for (const QString &skill : m_newPostSkillsEdit->text().split(',', Qt::SkipEmptyParts)) {
        skills.append(skill.trimmed());
    }

    QJsonObject payload;
    payload.insert("title", title);
    payload.insert("description", description);
    payload.insert("required_skills", skills);
    payload.insert("min_years_experience", m_newPostExperienceEdit->text().toDouble());
    payload.insert("location_mode", m_newPostModeCombo->currentText());
    payload.insert("city", m_newPostCityEdit->text().trimmed());
    payload.insert("country", m_newPostCountryEdit->text().trimmed());

    bool ok = false;
    QString error;
    ApiClient::instance().post("/jobs/create", payload, &ok, &error);
    if (!ok) {
        setStatus(tr("No se pudo crear la publicación: %1").arg(error), false);
        return;
    }

    setStatus(tr("Publicación creada correctamente."));
    m_newPostTitleEdit->clear();
    m_newPostSkillsEdit->clear();
    m_newPostExperienceEdit->clear();
    m_newPostCityEdit->clear();
    m_newPostCountryEdit->clear();
    m_newPostDescriptionEdit->clear();
    loadJobs();
}

void EmpresaWindow::sendChatMessage()
{
    const QString text = m_chatInput->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    m_chatHistory->append(tr("Empresa: %1").arg(text));
    m_chatHistory->append(tr("Sistema: Chat visual. Falta endpoint real en backend."));
    m_chatInput->clear();
}
