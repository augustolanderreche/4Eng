#include "adminwindow.h"
#include "api_client.h"
#include "empresawindow.h"
#include "userwindow.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QList>
#include <QPair>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

static QString valueToText(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool() ? "Sí" : "No";
    }
    if (value.isDouble()) {
        return QString::number(value.toVariant().toLongLong());
    }
    if (value.isString()) {
        return value.toString();
    }
    if (value.isArray() || value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.isArray() ? QJsonDocument(value.toArray()) : QJsonDocument(value.toObject())).toJson(QJsonDocument::Compact));
    }
    return "-";
}

AdminWindow::AdminWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
    loadDashboard();
    loadUsers();
    loadCompanies();
    loadAccepted();
    loadAiSettings();
}

void AdminWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    m_welcomeLabel = new QLabel(tr("Panel Admin - %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #FFFFFF;");
    m_welcomeLabel->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    m_tabs = new QTabWidget(central);
    m_tabs->addTab(createDashboardTab(), tr("Dashboard"));
    m_tabs->addTab(createUsersTab(), tr("Usuarios"));
    m_tabs->addTab(createCompaniesTab(), tr("Empresas"));
    m_tabs->addTab(createAcceptedTab(), tr("Aceptaciones"));
    m_tabs->addTab(createIaTab(), tr("Gestión IA"));
    m_tabs->addTab(createSystemTab(), tr("Sistema"));

    layout->addWidget(m_welcomeLabel);
    layout->addWidget(m_tabs, 1);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Panel admin"));
    resize(1180, 780);
}

void AdminWindow::setStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
}

QWidget *AdminWindow::createDashboardTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *reload = new QPushButton(tr("Actualizar dashboard"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadDashboard);

    m_dashboardTable = new QTableWidget(0, 2, page);
    m_dashboardTable->setHorizontalHeaderLabels({tr("Métrica"), tr("Valor")});
    m_dashboardTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_dashboardTable->verticalHeader()->setVisible(false);
    m_dashboardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *switchRow = new QHBoxLayout;
    auto *userModeButton = new QPushButton(tr("Ver modo Usuario"), page);
    auto *empresaModeButton = new QPushButton(tr("Ver modo Empresa"), page);
    connect(userModeButton, &QPushButton::clicked, this, [this]() {
        auto *userWindow = new UserWindow("Vista desde Admin");
        userWindow->setAttribute(Qt::WA_DeleteOnClose);
        userWindow->show();
    });
    connect(empresaModeButton, &QPushButton::clicked, this, [this]() {
        auto *empresaWindow = new EmpresaWindow("Vista desde Admin");
        empresaWindow->setAttribute(Qt::WA_DeleteOnClose);
        empresaWindow->show();
    });
    switchRow->addWidget(userModeButton);
    switchRow->addWidget(empresaModeButton);

    layout->addWidget(reload);
    layout->addWidget(m_dashboardTable, 1);
    layout->addLayout(switchRow);
    return page;
}

QWidget *AdminWindow::createUsersTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar usuarios"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadUsers);

    m_usersTable = new QTableWidget(0, 7, page);
    m_usersTable->setHorizontalHeaderLabels({tr("ID"), tr("Usuario"), tr("Rol"), tr("Email"), tr("Nombre"), tr("Online"), tr("Último acceso")});
    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->verticalHeader()->setVisible(false);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(reload);
    layout->addWidget(m_usersTable, 1);
    return page;
}

QWidget *AdminWindow::createCompaniesTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar empresas"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadCompanies);

    m_companiesTable = new QTableWidget(0, 4, page);
    m_companiesTable->setHorizontalHeaderLabels({tr("ID"), tr("Empresa"), tr("Email"), tr("Puestos activos")});
    m_companiesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_companiesTable->verticalHeader()->setVisible(false);
    m_companiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(reload);
    layout->addWidget(m_companiesTable, 1);
    return page;
}

QWidget *AdminWindow::createAcceptedTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar aceptaciones"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadAccepted);

    m_acceptedTable = new QTableWidget(0, 5, page);
    m_acceptedTable->setHorizontalHeaderLabels({tr("ID"), tr("Empresa"), tr("Puesto"), tr("Candidato"), tr("Fecha selección")});
    m_acceptedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_acceptedTable->verticalHeader()->setVisible(false);
    m_acceptedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(reload);
    layout->addWidget(m_acceptedTable, 1);
    return page;
}

QWidget *AdminWindow::createIaTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar configuración IA"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadAiSettings);

    m_aiTable = new QTableWidget(0, 5, page);
    m_aiTable->setHorizontalHeaderLabels({tr("Key"), tr("Valor"), tr("Grupo"), tr("Descripción"), tr("Actualizado")});
    m_aiTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_aiTable->verticalHeader()->setVisible(false);
    m_aiTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *form = new QHBoxLayout;
    m_aiKeyEdit = new QLineEdit(page);
    m_aiKeyEdit->setPlaceholderText(tr("setting_key"));
    m_aiValueEdit = new QTextEdit(page);
    m_aiValueEdit->setPlaceholderText(tr("setting_value"));
    m_aiValueEdit->setFixedHeight(70);
    auto *save = new QPushButton(tr("Guardar"), page);
    connect(save, &QPushButton::clicked, this, &AdminWindow::saveAiSetting);
    form->addWidget(m_aiKeyEdit);
    form->addWidget(m_aiValueEdit, 1);
    form->addWidget(save);

    connect(m_aiTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (row < 0) {
            return;
        }
        m_aiKeyEdit->setText(m_aiTable->item(row, 0)->text());
        m_aiValueEdit->setPlainText(m_aiTable->item(row, 1)->text());
    });

    layout->addWidget(reload);
    layout->addWidget(m_aiTable, 1);
    layout->addLayout(form);
    return page;
}

QWidget *AdminWindow::createSystemTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *info = new QLabel(page);
    info->setWordWrap(true);
    info->setText(tr("Sistema conectado a API: %1\nUsuario: %2\nRol: %3\nToken JWT: activo\n\nNota: eliminar usuarios, cambiar estados de postulaciones y auditoría completa requieren endpoints adicionales en FastAPI.")
                      .arg(ApiClient::instance().apiBaseUrl(),
                           ApiClient::instance().username(),
                           ApiClient::instance().role()));
    layout->addWidget(info);
    layout->addStretch();
    return page;
}

void AdminWindow::loadDashboard()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/dashboard", &ok, &error);
    if (!ok || !doc.isObject()) {
        setStatus(tr("Dashboard: %1").arg(error), false);
        return;
    }

    const QJsonObject obj = doc.object();
    const QList<QPair<QString, QString>> rows = {
        {"Usuarios registrados", valueToText(obj.value("registered_users"))},
        {"Empresas registradas", valueToText(obj.value("registered_companies"))},
        {"Usuarios online", valueToText(obj.value("online_users"))},
        {"Aceptados últimos 30 días", valueToText(obj.value("accepted_last_30_days"))}
    };

    m_dashboardTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        m_dashboardTable->setItem(i, 0, new QTableWidgetItem(rows.at(i).first));
        m_dashboardTable->setItem(i, 1, new QTableWidgetItem(rows.at(i).second));
    }
    setStatus(tr("Dashboard actualizado desde la API."));
}

void AdminWindow::loadUsers()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/users", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Usuarios: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_usersTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_usersTable->setItem(i, 0, new QTableWidgetItem(valueToText(o.value("id"))));
        m_usersTable->setItem(i, 1, new QTableWidgetItem(valueToText(o.value("username"))));
        m_usersTable->setItem(i, 2, new QTableWidgetItem(valueToText(o.value("role"))));
        m_usersTable->setItem(i, 3, new QTableWidgetItem(valueToText(o.value("email"))));
        m_usersTable->setItem(i, 4, new QTableWidgetItem(valueToText(o.value("display_name"))));
        m_usersTable->setItem(i, 5, new QTableWidgetItem(valueToText(o.value("is_online"))));
        m_usersTable->setItem(i, 6, new QTableWidgetItem(valueToText(o.value("last_login"))));
    }
    setStatus(tr("Usuarios actualizados desde la API."));
}

void AdminWindow::loadCompanies()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/companies", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Empresas: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_companiesTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_companiesTable->setItem(i, 0, new QTableWidgetItem(valueToText(o.value("id"))));
        m_companiesTable->setItem(i, 1, new QTableWidgetItem(valueToText(o.value("company_name"))));
        m_companiesTable->setItem(i, 2, new QTableWidgetItem(valueToText(o.value("email"))));
        m_companiesTable->setItem(i, 3, new QTableWidgetItem(valueToText(o.value("active_jobs"))));
    }
    setStatus(tr("Empresas actualizadas desde la API."));
}

void AdminWindow::loadAccepted()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/accepted", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Aceptaciones: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_acceptedTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_acceptedTable->setItem(i, 0, new QTableWidgetItem(valueToText(o.value("id"))));
        m_acceptedTable->setItem(i, 1, new QTableWidgetItem(valueToText(o.value("company_name"))));
        m_acceptedTable->setItem(i, 2, new QTableWidgetItem(valueToText(o.value("job_title"))));
        m_acceptedTable->setItem(i, 3, new QTableWidgetItem(valueToText(o.value("engineer_name"))));
        m_acceptedTable->setItem(i, 4, new QTableWidgetItem(valueToText(o.value("selected_at"))));
    }
    setStatus(tr("Aceptaciones actualizadas desde la API."));
}

void AdminWindow::loadAiSettings()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/ai/settings", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("IA settings: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_aiTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_aiTable->setItem(i, 0, new QTableWidgetItem(valueToText(o.value("setting_key"))));
        m_aiTable->setItem(i, 1, new QTableWidgetItem(valueToText(o.value("setting_value"))));
        m_aiTable->setItem(i, 2, new QTableWidgetItem(valueToText(o.value("setting_group"))));
        m_aiTable->setItem(i, 3, new QTableWidgetItem(valueToText(o.value("description"))));
        m_aiTable->setItem(i, 4, new QTableWidgetItem(valueToText(o.value("updated_at"))));
    }
    setStatus(tr("Configuración IA actualizada desde la API."));
}

void AdminWindow::saveAiSetting()
{
    const QString key = m_aiKeyEdit->text().trimmed();
    const QString value = m_aiValueEdit->toPlainText();
    if (key.isEmpty()) {
        setStatus(tr("Ingresá una setting_key."), false);
        return;
    }

    QJsonObject payload;
    payload.insert("setting_key", key);
    payload.insert("setting_value", value);

    bool ok = false;
    QString error;
    ApiClient::instance().put("/admin/ai/settings", payload, &ok, &error);
    if (!ok) {
        setStatus(tr("No se pudo guardar: %1").arg(error), false);
        return;
    }
    setStatus(tr("Configuración guardada."));
    loadAiSettings();
}
