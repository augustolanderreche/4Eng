#include "adminwindow.h"

#include "admindb.h"
#include "api_client.h"
#include "empresawindow.h"
#include "userwindow.h"

#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPair>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QCloseEvent>

static QString valueToText(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool() ? "Sí" : "No";
    }

    if (value.isDouble()) {
        const double number = value.toDouble();
        if (qFuzzyCompare(number, static_cast<qint64>(number))) {
            return QString::number(static_cast<qint64>(number));
        }
        return QString::number(number, 'f', 2);
    }

    if (value.isString()) {
        return value.toString();
    }

    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }

    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }

    return "-";
}

static QTableWidgetItem *itemFor(const QJsonValue &value)
{
    return new QTableWidgetItem(valueToText(value));
}

AdminWindow::AdminWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
    loadDashboard();
    loadSystemSummary();
    loadUsers();
    loadCompanies();
    loadAccepted();
    loadApplications();
    loadCvs();
    loadAiSettings();
    loadActivity();
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
    m_tabs->addTab(createApplicationsTab(), tr("Postulaciones"));
    m_tabs->addTab(createCvsTab(), tr("CVs + IA"));
    m_tabs->addTab(createAcceptedTab(), tr("Aceptaciones"));
    m_tabs->addTab(createIaTab(), tr("Gestión IA"));
    m_tabs->addTab(createActivityTab(), tr("Actividad"));
    m_tabs->addTab(createChatTab(), tr("Chat IA"));
    m_tabs->addTab(createSystemTab(), tr("Sistema"));

    layout->addWidget(m_welcomeLabel);
    layout->addWidget(m_tabs, 1);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Panel admin"));
    resize(1280, 820);
}

void AdminWindow::setStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
}

void AdminWindow::closeEvent(QCloseEvent *event)
{
    if (ApiClient::instance().isLoggedIn()) {
        AdminDB::instance().updateLastActivity(nullptr);
    }

    QMainWindow::closeEvent(event);
}

int AdminWindow::selectedId(QTableWidget *table) const
{
    if (!table || table->currentRow() < 0 || !table->item(table->currentRow(), 0)) {
        return 0;
    }

    return table->item(table->currentRow(), 0)->text().toInt();
}

void AdminWindow::showJsonDialog(const QString &title, const QJsonDocument &document)
{
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    dialog->resize(900, 650);

    auto *layout = new QVBoxLayout(dialog);
    auto *text = new QTextEdit(dialog);
    text->setReadOnly(true);
    text->setPlainText(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    layout->addWidget(text, 1);
    layout->addWidget(buttons);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void AdminWindow::fillSimpleKeyValueTable(QTableWidget *table, const QJsonObject &object)
{
    if (!table) {
        return;
    }

    table->setRowCount(object.size());
    int row = 0;

    for (auto it = object.begin(); it != object.end(); ++it) {
        table->setItem(row, 0, new QTableWidgetItem(it.key()));
        table->setItem(row, 1, itemFor(it.value()));
        ++row;
    }
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

    connect(userModeButton, &QPushButton::clicked, this, []() {
        auto *userWindow = new UserWindow("Vista desde Admin");
        userWindow->setAttribute(Qt::WA_DeleteOnClose);
        userWindow->show();
    });

    connect(empresaModeButton, &QPushButton::clicked, this, []() {
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

    auto *row = new QHBoxLayout;
    auto *reload = new QPushButton(tr("Actualizar usuarios"), page);
    auto *detail = new QPushButton(tr("Ver detalle seleccionado"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadUsers);
    connect(detail, &QPushButton::clicked, this, &AdminWindow::showSelectedUserDetail);
    row->addWidget(reload);
    row->addWidget(detail);
    row->addStretch();

    m_usersTable = new QTableWidget(0, 11, page);
    m_usersTable->setHorizontalHeaderLabels({
        tr("ID"), tr("Usuario"), tr("Rol"), tr("Email"), tr("Nombre"),
        tr("Activo"), tr("Online"), tr("Último acceso"), tr("CVs"),
        tr("Postulaciones"), tr("Puestos")
    });
    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->verticalHeader()->setVisible(false);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addLayout(row);
    layout->addWidget(m_usersTable, 1);
    return page;
}

QWidget *AdminWindow::createCompaniesTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *row = new QHBoxLayout;
    auto *reload = new QPushButton(tr("Actualizar empresas"), page);
    auto *detail = new QPushButton(tr("Ver detalle seleccionado"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadCompanies);
    connect(detail, &QPushButton::clicked, this, &AdminWindow::showSelectedCompanyDetail);
    row->addWidget(reload);
    row->addWidget(detail);
    row->addStretch();

    m_companiesTable = new QTableWidget(0, 4, page);
    m_companiesTable->setHorizontalHeaderLabels({tr("ID"), tr("Empresa"), tr("Email"), tr("Puestos activos")});
    m_companiesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_companiesTable->verticalHeader()->setVisible(false);
    m_companiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_companiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addLayout(row);
    layout->addWidget(m_companiesTable, 1);
    return page;
}

QWidget *AdminWindow::createApplicationsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *reload = new QPushButton(tr("Actualizar postulaciones"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadApplications);

    m_applicationsTable = new QTableWidget(0, 11, page);
    m_applicationsTable->setHorizontalHeaderLabels({
        tr("ID"), tr("Estado"), tr("Puesto"), tr("Empresa"), tr("Candidato"),
        tr("Email"), tr("CV"), tr("CV estado"), tr("Score"), tr("Recomendación"), tr("Fecha")
    });
    m_applicationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_applicationsTable->verticalHeader()->setVisible(false);
    m_applicationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_applicationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(reload);
    layout->addWidget(m_applicationsTable, 1);
    return page;
}

QWidget *AdminWindow::createCvsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *reload = new QPushButton(tr("Actualizar CVs y análisis IA"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadCvs);

    m_cvsTable = new QTableWidget(0, 11, page);
    m_cvsTable->setHorizontalHeaderLabels({
        tr("ID"), tr("Candidato"), tr("Email"), tr("Archivo"), tr("Original"),
        tr("Estado"), tr("Activo"), tr("Score"), tr("Modelo"), tr("Subido"), tr("Analizado")
    });
    m_cvsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_cvsTable->verticalHeader()->setVisible(false);
    m_cvsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cvsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(reload);
    layout->addWidget(m_cvsTable, 1);
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

QWidget *AdminWindow::createActivityTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *reload = new QPushButton(tr("Actualizar actividad"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadActivity);

    auto *notifLabel = new QLabel(tr("Últimas notificaciones del sistema"), page);
    m_notificationsTable = new QTableWidget(0, 8, page);
    m_notificationsTable->setHorizontalHeaderLabels({
        tr("ID"), tr("Usuario"), tr("Nombre"), tr("Tipo"), tr("Título"), tr("Prioridad"), tr("Leída"), tr("Fecha")
    });
    m_notificationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_notificationsTable->verticalHeader()->setVisible(false);
    m_notificationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *chatLabel = new QLabel(tr("Conversaciones IA"), page);
    m_chatTable = new QTableWidget(0, 8, page);
    m_chatTable->setHorizontalHeaderLabels({
        tr("ID"), tr("Usuario"), tr("Nombre"), tr("Rol"), tr("Scope"), tr("Título"), tr("Mensajes"), tr("Último mensaje")
    });
    m_chatTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_chatTable->verticalHeader()->setVisible(false);
    m_chatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(reload);
    layout->addWidget(notifLabel);
    layout->addWidget(m_notificationsTable, 1);
    layout->addWidget(chatLabel);
    layout->addWidget(m_chatTable, 1);
    return page;
}

QWidget *AdminWindow::createChatTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *info = new QLabel(tr("Chat IA administrativo. Podés consultar sobre usuarios, empresas, postulaciones, configuración de IA o estado general del sistema."), page);
    info->setWordWrap(true);

    m_chatHistory = new QTextEdit(page);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setPlainText(tr("Chat IA conectado para administración del sistema."));

    m_chatInput = new QLineEdit(page);
    m_chatInput->setPlaceholderText(tr("Escribí una consulta administrativa..."));

    m_chatPdfLabel = new QLabel(tr("PDF adjunto: ninguno"), page);
    m_chatPdfLabel->setWordWrap(true);
    m_chatPdfLabel->setStyleSheet("color:#A7B3C6;");

    auto *attach = new QPushButton(tr("Adjuntar PDF"), page);
    auto *send = new QPushButton(tr("Enviar"), page);

    connect(attach, &QPushButton::clicked, this, &AdminWindow::chooseChatPdf);
    connect(send, &QPushButton::clicked, this, &AdminWindow::sendChatMessage);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &AdminWindow::sendChatMessage);

    auto *row = new QHBoxLayout;
    row->addWidget(m_chatInput, 1);
    row->addWidget(attach);
    row->addWidget(send);

    layout->addWidget(info);
    layout->addWidget(m_chatHistory, 1);
    layout->addWidget(m_chatPdfLabel);
    layout->addLayout(row);
    return page;
}

QWidget *AdminWindow::createSystemTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *reload = new QPushButton(tr("Actualizar resumen del sistema"), page);
    connect(reload, &QPushButton::clicked, this, &AdminWindow::loadSystemSummary);

    auto *info = new QLabel(page);
    info->setWordWrap(true);
    info->setText(tr("Sistema conectado a API: %1\nUsuario: %2\nRol: %3\nToken JWT: activo\n\nEsta vista resume el estado general del sistema, incluyendo usuarios, empresas, puestos, CVs, postulaciones, notificaciones y uso del chat IA.")
                      .arg(ApiClient::instance().apiBaseUrl(),
                           ApiClient::instance().username(),
                           ApiClient::instance().role()));

    m_systemTable = new QTableWidget(0, 2, page);
    m_systemTable->setHorizontalHeaderLabels({tr("Métrica"), tr("Valor")});
    m_systemTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_systemTable->verticalHeader()->setVisible(false);
    m_systemTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(info);
    layout->addWidget(reload);
    layout->addWidget(m_systemTable, 1);
    return page;
}

void AdminWindow::chooseChatPdf()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Seleccionar PDF para el chat"),
        QString(),
        tr("Archivos PDF (*.pdf)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    m_chatPdfPath = filePath;

    if (m_chatPdfLabel) {
        m_chatPdfLabel->setText(tr("PDF adjunto: %1").arg(QFileInfo(filePath).fileName()));
    }

    setStatus(tr("PDF adjuntado al chat: %1").arg(QFileInfo(filePath).fileName()));
}

void AdminWindow::sendChatMessage()
{
    const QString text = m_chatInput->text().trimmed();

    if (text.isEmpty()) {
        return;
    }

    m_chatHistory->append(tr("Admin: %1").arg(text));

    if (!m_chatPdfPath.isEmpty()) {
        m_chatHistory->append(tr("PDF adjunto: %1").arg(QFileInfo(m_chatPdfPath).fileName()));
    }

    m_chatInput->clear();

    QJsonObject context;
    context.insert("role", ApiClient::instance().role());
    context.insert("username", ApiClient::instance().username());
    context.insert("display_name", ApiClient::instance().displayName());

    QJsonObject response;
    QString error;

    const bool ok = ApiClient::instance().sendChatMessage(
        text,
        QStringLiteral("ADMIN_HELP"),
        m_chatConversationId,
        context,
        m_chatPdfPath,
        &error,
        &response
    );

    if (!ok) {
        m_chatHistory->append(tr("IA: No pude responder en este momento. %1").arg(error));
        setStatus(tr("Chat IA: %1").arg(error), false);
        return;
    }

    m_chatConversationId = response.value("conversation_id").toVariant().toLongLong();

    const QString reply = response.value("reply").toString();
    const bool fallback = response.value("fallback").toBool(false);
    const QString modelName = response.value("model_name").toString();

    m_chatHistory->append(tr("IA%1: %2").arg(fallback ? QStringLiteral(" (fallback)") : QString(), reply));

    if (!modelName.isEmpty()) {
        setStatus(tr("Chat IA respondió usando %1.").arg(modelName));
    } else {
        setStatus(tr("Chat IA respondió correctamente."));
    }

    m_chatPdfPath.clear();
    if (m_chatPdfLabel) {
        m_chatPdfLabel->setText(tr("PDF adjunto: ninguno"));
    }
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

void AdminWindow::loadSystemSummary()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/system/summary", &ok, &error);
    if (!ok || !doc.isObject()) {
        setStatus(tr("Resumen del sistema: %1").arg(error), false);
        return;
    }

    fillSimpleKeyValueTable(m_systemTable, doc.object().value("metrics").toObject());
    setStatus(tr("Resumen avanzado del sistema actualizado."));
}

void AdminWindow::loadUsers()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/users/full", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Usuarios: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_usersTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_usersTable->setItem(i, 0, itemFor(o.value("id")));
        m_usersTable->setItem(i, 1, itemFor(o.value("username")));
        m_usersTable->setItem(i, 2, itemFor(o.value("role")));
        m_usersTable->setItem(i, 3, itemFor(o.value("email")));
        m_usersTable->setItem(i, 4, itemFor(o.value("display_name")));
        m_usersTable->setItem(i, 5, itemFor(o.value("is_active")));
        m_usersTable->setItem(i, 6, itemFor(o.value("is_online")));
        m_usersTable->setItem(i, 7, itemFor(o.value("last_login")));
        m_usersTable->setItem(i, 8, itemFor(o.value("cv_count")));
        m_usersTable->setItem(i, 9, itemFor(o.value("applications_count")));
        m_usersTable->setItem(i, 10, itemFor(o.value("jobs_count")));
    }
    setStatus(tr("Usuarios completos actualizados desde la API."));
}

void AdminWindow::showSelectedUserDetail()
{
    const int id = selectedId(m_usersTable);
    if (id <= 0) {
        setStatus(tr("Seleccioná un usuario."), false);
        return;
    }

    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get(QString("/admin/users/%1/detail").arg(id), &ok, &error);
    if (!ok || !doc.isObject()) {
        setStatus(tr("Detalle usuario: %1").arg(error), false);
        return;
    }

    showJsonDialog(tr("Detalle del usuario %1").arg(id), doc);
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
        m_companiesTable->setItem(i, 0, itemFor(o.value("id")));
        m_companiesTable->setItem(i, 1, itemFor(o.value("company_name")));
        m_companiesTable->setItem(i, 2, itemFor(o.value("email")));
        m_companiesTable->setItem(i, 3, itemFor(o.value("active_jobs")));
    }
    setStatus(tr("Empresas actualizadas desde la API."));
}

void AdminWindow::showSelectedCompanyDetail()
{
    const int id = selectedId(m_companiesTable);
    if (id <= 0) {
        setStatus(tr("Seleccioná una empresa."), false);
        return;
    }

    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get(QString("/admin/companies/%1/detail").arg(id), &ok, &error);
    if (!ok || !doc.isObject()) {
        setStatus(tr("Detalle empresa: %1").arg(error), false);
        return;
    }

    showJsonDialog(tr("Detalle de empresa %1").arg(id), doc);
}

void AdminWindow::loadApplications()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/applications/all", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Postulaciones: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_applicationsTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_applicationsTable->setItem(i, 0, itemFor(o.value("id")));
        m_applicationsTable->setItem(i, 1, itemFor(o.value("status")));
        m_applicationsTable->setItem(i, 2, itemFor(o.value("job_title")));
        m_applicationsTable->setItem(i, 3, itemFor(o.value("company_name")));
        m_applicationsTable->setItem(i, 4, itemFor(o.value("engineer_name")));
        m_applicationsTable->setItem(i, 5, itemFor(o.value("engineer_email")));
        m_applicationsTable->setItem(i, 6, itemFor(o.value("cv_file_name")));
        m_applicationsTable->setItem(i, 7, itemFor(o.value("cv_status")));
        m_applicationsTable->setItem(i, 8, itemFor(o.value("latest_score")));
        m_applicationsTable->setItem(i, 9, itemFor(o.value("latest_recommendation")));
        m_applicationsTable->setItem(i, 10, itemFor(o.value("applied_at")));
    }
    setStatus(tr("Postulaciones actualizadas desde la API."));
}

void AdminWindow::loadCvs()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/admin/cvs", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("CVs: %1").arg(error), false);
        return;
    }

    const QJsonArray arr = doc.array();
    m_cvsTable->setRowCount(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject o = arr.at(i).toObject();
        m_cvsTable->setItem(i, 0, itemFor(o.value("id")));
        m_cvsTable->setItem(i, 1, itemFor(o.value("engineer_name")));
        m_cvsTable->setItem(i, 2, itemFor(o.value("engineer_email")));
        m_cvsTable->setItem(i, 3, itemFor(o.value("file_name")));
        m_cvsTable->setItem(i, 4, itemFor(o.value("original_file_name")));
        m_cvsTable->setItem(i, 5, itemFor(o.value("status")));
        m_cvsTable->setItem(i, 6, itemFor(o.value("is_active")));
        m_cvsTable->setItem(i, 7, itemFor(o.value("overall_score")));
        m_cvsTable->setItem(i, 8, itemFor(o.value("model_name")));
        m_cvsTable->setItem(i, 9, itemFor(o.value("uploaded_at")));
        m_cvsTable->setItem(i, 10, itemFor(o.value("analyzed_at")));
    }
    setStatus(tr("CVs y análisis actualizados desde la API."));
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
        m_acceptedTable->setItem(i, 0, itemFor(o.value("id")));
        m_acceptedTable->setItem(i, 1, itemFor(o.value("company_name")));
        m_acceptedTable->setItem(i, 2, itemFor(o.value("job_title")));
        m_acceptedTable->setItem(i, 3, itemFor(o.value("engineer_name")));
        m_acceptedTable->setItem(i, 4, itemFor(o.value("selected_at")));
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
        m_aiTable->setItem(i, 0, itemFor(o.value("setting_key")));
        m_aiTable->setItem(i, 1, itemFor(o.value("setting_value")));
        m_aiTable->setItem(i, 2, itemFor(o.value("setting_group")));
        m_aiTable->setItem(i, 3, itemFor(o.value("description")));
        m_aiTable->setItem(i, 4, itemFor(o.value("updated_at")));
    }
    setStatus(tr("Configuración IA actualizada desde la API."));
}

void AdminWindow::loadActivity()
{
    bool ok = false;
    QString error;

    QJsonDocument notificationsDoc = ApiClient::instance().get("/admin/notifications/all", &ok, &error);
    if (!ok || !notificationsDoc.isArray()) {
        setStatus(tr("Notificaciones admin: %1").arg(error), false);
        return;
    }

    const QJsonArray notifications = notificationsDoc.array();
    m_notificationsTable->setRowCount(notifications.size());
    for (int i = 0; i < notifications.size(); ++i) {
        const QJsonObject o = notifications.at(i).toObject();
        m_notificationsTable->setItem(i, 0, itemFor(o.value("id")));
        m_notificationsTable->setItem(i, 1, itemFor(o.value("username")));
        m_notificationsTable->setItem(i, 2, itemFor(o.value("display_name")));
        m_notificationsTable->setItem(i, 3, itemFor(o.value("type")));
        m_notificationsTable->setItem(i, 4, itemFor(o.value("title")));
        m_notificationsTable->setItem(i, 5, itemFor(o.value("priority")));
        m_notificationsTable->setItem(i, 6, itemFor(o.value("is_read")));
        m_notificationsTable->setItem(i, 7, itemFor(o.value("created_at")));
    }

    ok = false;
    error.clear();
    QJsonDocument chatDoc = ApiClient::instance().get("/admin/chat/conversations", &ok, &error);
    if (!ok || !chatDoc.isArray()) {
        setStatus(tr("Conversaciones IA admin: %1").arg(error), false);
        return;
    }

    const QJsonArray chats = chatDoc.array();
    m_chatTable->setRowCount(chats.size());
    for (int i = 0; i < chats.size(); ++i) {
        const QJsonObject o = chats.at(i).toObject();
        m_chatTable->setItem(i, 0, itemFor(o.value("id")));
        m_chatTable->setItem(i, 1, itemFor(o.value("username")));
        m_chatTable->setItem(i, 2, itemFor(o.value("display_name")));
        m_chatTable->setItem(i, 3, itemFor(o.value("owner_role")));
        m_chatTable->setItem(i, 4, itemFor(o.value("scope")));
        m_chatTable->setItem(i, 5, itemFor(o.value("title")));
        m_chatTable->setItem(i, 6, itemFor(o.value("messages_count")));
        m_chatTable->setItem(i, 7, itemFor(o.value("last_message_at")));
    }

    setStatus(tr("Actividad del sistema actualizada."));
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
