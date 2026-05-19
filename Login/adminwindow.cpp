#include "adminwindow.h"
#include "empresawindow.h"
#include "userwindow.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

AdminWindow::AdminWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
}

void AdminWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);

    m_welcomeLabel = new QLabel(tr("Panel Admin - %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #FFFFFF;");
    m_welcomeLabel->setAlignment(Qt::AlignCenter);

    m_tabs = new QTabWidget(central);
    m_tabs->addTab(createDashboardTab(), tr("Dashboard"));
    m_tabs->addTab(createUsersTab(), tr("Usuarios"));
    m_tabs->addTab(createCompaniesTab(), tr("Empresas"));
    m_tabs->addTab(createAcceptedTab(), tr("Aceptaciones"));
    m_tabs->addTab(createIaTab(), tr("Gestión IA"));

    layout->addWidget(m_welcomeLabel);
    layout->addWidget(m_tabs);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    setWindowTitle(tr("Panel admin"));
    resize(1100, 740);
}

QWidget *AdminWindow::createDashboardTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *stats = new QTableWidget(4, 2, page);
    stats->setHorizontalHeaderLabels({tr("Métrica"), tr("Valor")});
    stats->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    stats->verticalHeader()->setVisible(false);
    stats->setEditTriggers(QAbstractItemView::NoEditTriggers);

    stats->setItem(0, 0, new QTableWidgetItem("Usuarios registrados"));
    stats->setItem(0, 1, new QTableWidgetItem("128"));
    stats->setItem(1, 0, new QTableWidgetItem("Empresas registradas"));
    stats->setItem(1, 1, new QTableWidgetItem("23"));
    stats->setItem(2, 0, new QTableWidgetItem("Usuarios online"));
    stats->setItem(2, 1, new QTableWidgetItem("17"));
    stats->setItem(3, 0, new QTableWidgetItem("Solicitudes aceptadas (30 días)"));
    stats->setItem(3, 1, new QTableWidgetItem("42"));

    auto *switchRow = new QHBoxLayout;
    auto *userModeButton = new QPushButton(tr("Ver modo Usuario"), page);
    auto *empresaModeButton = new QPushButton(tr("Ver modo Empresa"), page);
    auto *adminModeButton = new QPushButton(tr("Ver modo Admin"), page);

    connect(userModeButton, &QPushButton::clicked, this, [this]() {
        auto *userWindow = new UserWindow("Vista desde Admin");
        userWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(userWindow, &QObject::destroyed, this, [this]() { this->show(); });
        userWindow->show();
        this->hide();
    });

    connect(empresaModeButton, &QPushButton::clicked, this, [this]() {
        auto *empresaWindow = new EmpresaWindow("Vista desde Admin");
        empresaWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(empresaWindow, &QObject::destroyed, this, [this]() { this->show(); });
        empresaWindow->show();
        this->hide();
    });

    connect(adminModeButton, &QPushButton::clicked, this, [this]() {
        m_tabs->setCurrentIndex(0);
    });

    switchRow->addWidget(userModeButton);
    switchRow->addWidget(empresaModeButton);
    switchRow->addWidget(adminModeButton);

    layout->addWidget(new QLabel(tr("Vista general del sistema"), page));
    layout->addWidget(stats);
    layout->addLayout(switchRow);
    return page;
}

QWidget *AdminWindow::createUsersTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *table = new QTableWidget(3, 6, page);
    table->setHorizontalHeaderLabels({tr("ID"), tr("Usuario"), tr("Rol"), tr("Online"), tr("Último acceso"), tr("CV")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("1"));
    table->setItem(0, 1, new QTableWidgetItem("user_demo"));
    table->setItem(0, 2, new QTableWidgetItem("Usuario"));
    table->setItem(0, 3, new QTableWidgetItem("Sí"));
    table->setItem(0, 4, new QTableWidgetItem("Hoy 11:05"));
    table->setItem(0, 5, new QTableWidgetItem("cv_backend_junior.pdf"));

    table->setItem(1, 0, new QTableWidgetItem("2"));
    table->setItem(1, 1, new QTableWidgetItem("sofia_dev"));
    table->setItem(1, 2, new QTableWidgetItem("Usuario"));
    table->setItem(1, 3, new QTableWidgetItem("No"));
    table->setItem(1, 4, new QTableWidgetItem("Hoy 09:40"));
    table->setItem(1, 5, new QTableWidgetItem("cv_sofia_2026.pdf"));

    table->setItem(2, 0, new QTableWidgetItem("3"));
    table->setItem(2, 1, new QTableWidgetItem("marcos_cpp"));
    table->setItem(2, 2, new QTableWidgetItem("Usuario"));
    table->setItem(2, 3, new QTableWidgetItem("Sí"));
    table->setItem(2, 4, new QTableWidgetItem("Hoy 11:10"));
    table->setItem(2, 5, new QTableWidgetItem("cv_marcos_qt.pdf"));

    auto *buttonRow = new QHBoxLayout;
    auto *viewInfo = new QPushButton(tr("Ver info"), page);
    auto *deleteUser = new QPushButton(tr("Eliminar usuario"), page);
    auto *toggleStatus = new QPushButton(tr("Actualizar estado"), page);

    connect(viewInfo, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("Usuarios"), tr("Selecciona un usuario ingeniero."));
            return;
        }
        const QString info = tr("Usuario: %1\nRol: %2\nOnline: %3\nÚltimo acceso: %4\nCV: %5")
                                 .arg(table->item(row, 1)->text(),
                                      table->item(row, 2)->text(),
                                      table->item(row, 3)->text(),
                                      table->item(row, 4)->text(),
                                      table->item(row, 5)->text());
        QMessageBox::information(this, tr("Info usuario"), info);
    });

    connect(deleteUser, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            return;
        }
        const QString username = table->item(row, 1)->text();
        table->removeRow(row);
        QMessageBox::information(this, tr("Usuarios"), tr("Usuario %1 eliminado.").arg(username));
    });

    connect(toggleStatus, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            return;
        }
        QTableWidgetItem *statusItem = table->item(row, 3);
        statusItem->setText(statusItem->text() == "Sí" ? "No" : "Sí");
    });

    buttonRow->addWidget(viewInfo);
    buttonRow->addWidget(deleteUser);
    buttonRow->addWidget(toggleStatus);

    layout->addWidget(new QLabel(tr("Gestión de usuarios ingenieros"), page));
    layout->addWidget(table);
    layout->addLayout(buttonRow);
    return page;
}

QWidget *AdminWindow::createCompaniesTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *table = new QTableWidget(3, 4, page);
    table->setHorizontalHeaderLabels({tr("ID"), tr("Empresa"), tr("Contacto"), tr("Publicaciones activas")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("12"));
    table->setItem(0, 1, new QTableWidgetItem("TechNova"));
    table->setItem(0, 2, new QTableWidgetItem("hr@technova.com"));
    table->setItem(0, 3, new QTableWidgetItem("3"));

    table->setItem(1, 0, new QTableWidgetItem("15"));
    table->setItem(1, 1, new QTableWidgetItem("Blue Systems"));
    table->setItem(1, 2, new QTableWidgetItem("jobs@bluesystems.com"));
    table->setItem(1, 3, new QTableWidgetItem("2"));

    table->setItem(2, 0, new QTableWidgetItem("18"));
    table->setItem(2, 1, new QTableWidgetItem("CloudWare"));
    table->setItem(2, 2, new QTableWidgetItem("talent@cloudware.io"));
    table->setItem(2, 3, new QTableWidgetItem("1"));

    auto *buttonRow = new QHBoxLayout;
    auto *viewDetail = new QPushButton(tr("Ver detalle"), page);
    auto *deleteCompany = new QPushButton(tr("Eliminar empresa"), page);

    connect(viewDetail, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("Empresas"), tr("Selecciona una empresa."));
            return;
        }
        QMessageBox::information(this,
                                 tr("Detalle empresa"),
                                 tr("Empresa: %1\nContacto: %2\nPublicaciones activas: %3")
                                     .arg(table->item(row, 1)->text(),
                                          table->item(row, 2)->text(),
                                          table->item(row, 3)->text()));
    });

    connect(deleteCompany, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            return;
        }
        const QString companyName = table->item(row, 1)->text();
        table->removeRow(row);
        QMessageBox::information(this, tr("Empresas"), tr("Empresa %1 eliminada.").arg(companyName));
    });

    buttonRow->addWidget(viewDetail);
    buttonRow->addWidget(deleteCompany);

    layout->addWidget(new QLabel(tr("Empresas registradas"), page));
    layout->addWidget(table);
    layout->addLayout(buttonRow);
    return page;
}

QWidget *AdminWindow::createAcceptedTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *table = new QTableWidget(4, 4, page);
    table->setHorizontalHeaderLabels({tr("Empresa"), tr("Puesto"), tr("Usuario aceptado"), tr("Fecha")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("Blue Systems"));
    table->setItem(0, 1, new QTableWidgetItem("C++ Qt Developer"));
    table->setItem(0, 2, new QTableWidgetItem("Marcos Gomez"));
    table->setItem(0, 3, new QTableWidgetItem("2026-05-17"));

    table->setItem(1, 0, new QTableWidgetItem("TechNova"));
    table->setItem(1, 1, new QTableWidgetItem("Backend Python"));
    table->setItem(1, 2, new QTableWidgetItem("Sofia Diaz"));
    table->setItem(1, 3, new QTableWidgetItem("2026-05-16"));

    table->setItem(2, 0, new QTableWidgetItem("DataLoop"));
    table->setItem(2, 1, new QTableWidgetItem("Data Analyst"));
    table->setItem(2, 2, new QTableWidgetItem("Lucia Ruiz"));
    table->setItem(2, 3, new QTableWidgetItem("2026-05-14"));

    table->setItem(3, 0, new QTableWidgetItem("CloudWare"));
    table->setItem(3, 1, new QTableWidgetItem("DevOps Jr"));
    table->setItem(3, 2, new QTableWidgetItem("Nicolas Vera"));
    table->setItem(3, 3, new QTableWidgetItem("2026-05-13"));

    layout->addWidget(new QLabel(tr("Solicitudes aceptadas por empresas"), page));
    layout->addWidget(table);
    return page;
}

QWidget *AdminWindow::createIaTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *settings = new QListWidget(page);
    settings->addItem("Modelo activo: gpt-4.1-mini");
    settings->addItem("Modo resumen CV: habilitado");
    settings->addItem("Modo chat soporte: habilitado");
    settings->addItem("Temperatura: 0.3");

    auto *prompt = new QTextEdit(page);
    prompt->setPlainText("Prompt de resumen CV por vacante:\nResume fortalezas, gaps y recomendacion final para la empresa.");

    auto *row = new QHBoxLayout;
    auto *saveConfig = new QPushButton(tr("Guardar configuración"), page);
    auto *testSummary = new QPushButton(tr("Probar resumen IA"), page);
    auto *viewLogs = new QPushButton(tr("Ver logs IA"), page);

    connect(saveConfig, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("IA"), tr("Configuración de IA guardada correctamente."));
    });

    connect(testSummary, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 tr("IA - Prueba resumen"),
                                 tr("Resumen de prueba:\nFortalezas: FastAPI, SQL\nGaps: Testing avanzado\nRecomendación: RecomendadaConCapacitacion"));
    });

    connect(viewLogs, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 tr("IA - Logs"),
                                 tr("2026-05-18 10:20: resumen_cv OK\n2026-05-18 10:25: chat_response OK\n2026-05-18 10:31: recommendation OK"));
    });

    row->addWidget(saveConfig);
    row->addWidget(testSummary);
    row->addWidget(viewLogs);

    layout->addWidget(new QLabel(tr("Gestión de IA"), page));
    layout->addWidget(settings);
    layout->addWidget(new QLabel(tr("Prompt activo"), page));
    layout->addWidget(prompt);
    layout->addLayout(row);
    return page;
}
