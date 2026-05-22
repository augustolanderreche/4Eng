#include "adminwindow.h"
#include "empresawindow.h"
#include "userwindow.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
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
    m_tabs->addTab(createAuditTab(), tr("Auditoría"));
    m_tabs->addTab(createSystemTab(), tr("Sistema"));

    layout->addWidget(m_welcomeLabel);
    layout->addWidget(m_tabs);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    setWindowTitle(tr("Panel admin"));
    resize(1180, 780);
}

QWidget *AdminWindow::createDashboardTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *stats = new QTableWidget(6, 2, page);
    stats->setHorizontalHeaderLabels({tr("Métrica"), tr("Valor")});
    stats->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    stats->verticalHeader()->setVisible(false);
    stats->setEditTriggers(QAbstractItemView::NoEditTriggers);

    stats->setItem(0, 0, new QTableWidgetItem("Usuarios registrados"));
    stats->setItem(0, 1, new QTableWidgetItem("5"));
    stats->setItem(1, 0, new QTableWidgetItem("Empresas registradas"));
    stats->setItem(1, 1, new QTableWidgetItem("1"));
    stats->setItem(2, 0, new QTableWidgetItem("Usuarios online"));
    stats->setItem(2, 1, new QTableWidgetItem("2"));
    stats->setItem(3, 0, new QTableWidgetItem("CVs cargados"));
    stats->setItem(3, 1, new QTableWidgetItem("3"));
    stats->setItem(4, 0, new QTableWidgetItem("Postulaciones activas"));
    stats->setItem(4, 1, new QTableWidgetItem("3"));
    stats->setItem(5, 0, new QTableWidgetItem("Candidatos seleccionados"));
    stats->setItem(5, 1, new QTableWidgetItem("1"));

    auto *switchRow = new QHBoxLayout;
    auto *userModeButton = new QPushButton(tr("Ver modo Usuario"), page);
    auto *empresaModeButton = new QPushButton(tr("Ver modo Empresa"), page);
    auto *adminModeButton = new QPushButton(tr("Ver modo Admin"), page);

    connect(userModeButton, &QPushButton::clicked, this, [this]() {
        auto *userWindow = new UserWindow("Vista desde Admin");
        userWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(userWindow, &QObject::destroyed, this, [this]() { show(); });
        userWindow->show();
        hide();
    });

    connect(empresaModeButton, &QPushButton::clicked, this, [this]() {
        auto *empresaWindow = new EmpresaWindow("Vista desde Admin");
        empresaWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(empresaWindow, &QObject::destroyed, this, [this]() { show(); });
        empresaWindow->show();
        hide();
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

    auto *table = new QTableWidget(3, 8, page);
    table->setHorizontalHeaderLabels({tr("ID"), tr("Usuario"), tr("Nombre"), tr("Rol"), tr("Online"), tr("Último acceso"), tr("CV"), tr("Skills")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("3"));
    table->setItem(0, 1, new QTableWidgetItem("user_demo"));
    table->setItem(0, 2, new QTableWidgetItem("Usuario Demo"));
    table->setItem(0, 3, new QTableWidgetItem("Usuario"));
    table->setItem(0, 4, new QTableWidgetItem("No"));
    table->setItem(0, 5, new QTableWidgetItem("Sin login reciente"));
    table->setItem(0, 6, new QTableWidgetItem("cv_fullstack_2026.pdf"));
    table->setItem(0, 7, new QTableWidgetItem("Python, SQL"));

    table->setItem(1, 0, new QTableWidgetItem("4"));
    table->setItem(1, 1, new QTableWidgetItem("sofia_dev"));
    table->setItem(1, 2, new QTableWidgetItem("Sofia Diaz"));
    table->setItem(1, 3, new QTableWidgetItem("Usuario"));
    table->setItem(1, 4, new QTableWidgetItem("Sí"));
    table->setItem(1, 5, new QTableWidgetItem("NOW()"));
    table->setItem(1, 6, new QTableWidgetItem("cv_backend_python.pdf"));
    table->setItem(1, 7, new QTableWidgetItem("Python, FastAPI, SQL"));

    table->setItem(2, 0, new QTableWidgetItem("5"));
    table->setItem(2, 1, new QTableWidgetItem("marcos_cpp"));
    table->setItem(2, 2, new QTableWidgetItem("Marcos Gomez"));
    table->setItem(2, 3, new QTableWidgetItem("Usuario"));
    table->setItem(2, 4, new QTableWidgetItem("Sí"));
    table->setItem(2, 5, new QTableWidgetItem("NOW()"));
    table->setItem(2, 6, new QTableWidgetItem("cv_cpp_qt.pdf"));
    table->setItem(2, 7, new QTableWidgetItem("C++, Qt"));

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
        const QString info = tr("Usuario: %1\nNombre: %2\nRol: %3\nOnline: %4\nÚltimo acceso: %5\nCV: %6\nSkills: %7")
                                 .arg(table->item(row, 1)->text(),
                                      table->item(row, 2)->text(),
                                      table->item(row, 3)->text(),
                                      table->item(row, 4)->text(),
                                      table->item(row, 5)->text(),
                                      table->item(row, 6)->text(),
                                      table->item(row, 7)->text());
        QMessageBox::information(this, tr("Info usuario"), info);
    });

    connect(deleteUser, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            return;
        }
        const QString username = table->item(row, 1)->text();
        table->removeRow(row);
        QMessageBox::information(this, tr("Usuarios"), tr("Usuario %1 eliminado en modo mock.").arg(username));
    });

    connect(toggleStatus, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            return;
        }
        QTableWidgetItem *statusItem = table->item(row, 4);
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

    auto *table = new QTableWidget(1, 6, page);
    table->setHorizontalHeaderLabels({tr("ID"), tr("Empresa"), tr("Industria"), tr("Contacto"), tr("Tamaño"), tr("Publicaciones")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("2"));
    table->setItem(0, 1, new QTableWidgetItem("Empresa Demo"));
    table->setItem(0, 2, new QTableWidgetItem("Software / Tecnología"));
    table->setItem(0, 3, new QTableWidgetItem("Responsable RRHH"));
    table->setItem(0, 4, new QTableWidgetItem("11-50"));
    table->setItem(0, 5, new QTableWidgetItem("2 activas, 1 pausada"));

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
                                 tr("Empresa: %1\nIndustria: %2\nContacto: %3\nTamaño: %4\nPublicaciones: %5")
                                     .arg(table->item(row, 1)->text(),
                                          table->item(row, 2)->text(),
                                          table->item(row, 3)->text(),
                                          table->item(row, 4)->text(),
                                          table->item(row, 5)->text()));
    });

    connect(deleteCompany, &QPushButton::clicked, this, [this, table]() {
        const int row = table->currentRow();
        if (row < 0) {
            return;
        }
        const QString companyName = table->item(row, 1)->text();
        table->removeRow(row);
        QMessageBox::information(this, tr("Empresas"), tr("Empresa %1 eliminada en modo mock.").arg(companyName));
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

    auto *table = new QTableWidget(1, 4, page);
    table->setHorizontalHeaderLabels({tr("Empresa"), tr("Puesto"), tr("Usuario aceptado"), tr("Fecha")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("Empresa Demo"));
    table->setItem(0, 1, new QTableWidgetItem("Backend Python"));
    table->setItem(0, 2, new QTableWidgetItem("Sofia Diaz"));
    table->setItem(0, 3, new QTableWidgetItem("2026-05-21"));

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
    settings->addItem("Prompt CV Summary: activo");
    settings->addItem("Prompt Job Match: activo");

    auto *prompt = new QTextEdit(page);
    prompt->setPlainText(
        "PROMPT_CV_SUMMARY:\nResume fortalezas, gaps y recomendacion final para el CV.\n\n"
        "PROMPT_JOB_MATCH:\nCompara el CV contra el puesto y devuelve score, fortalezas y skills faltantes.");

    auto *row = new QHBoxLayout;
    auto *saveConfig = new QPushButton(tr("Guardar configuración"), page);
    auto *testSummary = new QPushButton(tr("Probar resumen IA"), page);
    auto *viewLogs = new QPushButton(tr("Ver logs IA"), page);

    connect(saveConfig, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("IA"), tr("Configuración de IA guardada correctamente en modo demo."));
    });

    connect(testSummary, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 tr("IA - Prueba resumen"),
                                 tr("Resumen de prueba:\nFortalezas: FastAPI, SQL\nGaps: Testing avanzado\nRecomendación: RecomendadaConCapacitacion"));
    });

    connect(viewLogs, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 tr("IA - Logs"),
                                 tr("2026-05-21 10:20: cv_summary OK\n2026-05-21 10:25: chat_response OK\n2026-05-21 10:31: recommendation OK"));
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

QWidget *AdminWindow::createAuditTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *table = new QTableWidget(4, 5, page);
    table->setHorizontalHeaderLabels({tr("Fecha"), tr("Admin"), tr("Acción"), tr("Target"), tr("Detalle")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItem(0, 0, new QTableWidgetItem("2026-05-21 10:20"));
    table->setItem(0, 1, new QTableWidgetItem("admin_demo"));
    table->setItem(0, 2, new QTableWidgetItem("DATABASE_INITIALIZED"));
    table->setItem(0, 3, new QTableWidgetItem("DATABASE"));
    table->setItem(0, 4, new QTableWidgetItem("final-v1"));

    table->setItem(1, 0, new QTableWidgetItem("2026-05-21 10:35"));
    table->setItem(1, 1, new QTableWidgetItem("admin_demo"));
    table->setItem(1, 2, new QTableWidgetItem("VIEW_USER_PROFILE"));
    table->setItem(1, 3, new QTableWidgetItem("USER 4"));
    table->setItem(1, 4, new QTableWidgetItem("Sofia Diaz"));

    table->setItem(2, 0, new QTableWidgetItem("2026-05-21 10:42"));
    table->setItem(2, 1, new QTableWidgetItem("admin_demo"));
    table->setItem(2, 2, new QTableWidgetItem("UPDATE_AI_SETTING"));
    table->setItem(2, 3, new QTableWidgetItem("AI_MODEL_ACTIVE"));
    table->setItem(2, 4, new QTableWidgetItem("gpt-4.1-mini"));

    table->setItem(3, 0, new QTableWidgetItem("2026-05-21 10:55"));
    table->setItem(3, 1, new QTableWidgetItem("admin_demo"));
    table->setItem(3, 2, new QTableWidgetItem("REVIEW_APPLICATION"));
    table->setItem(3, 3, new QTableWidgetItem("APPLICATION 2"));
    table->setItem(3, 4, new QTableWidgetItem("En revisión"));

    auto *row = new QHBoxLayout;
    auto *refresh = new QPushButton(tr("Refrescar logs"), page);
    auto *exportLogs = new QPushButton(tr("Exportar auditoría"), page);
    connect(refresh, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Auditoría"), tr("Logs refrescados en modo mock."));
    });
    connect(exportLogs, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Auditoría"), tr("Exportación simulada de admin_audit_logs completada."));
    });
    row->addWidget(refresh);
    row->addWidget(exportLogs);

    layout->addWidget(new QLabel(tr("Auditoría administrativa"), page));
    layout->addWidget(table);
    layout->addLayout(row);
    return page;
}

QWidget *AdminWindow::createSystemTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *statusList = new QListWidget(page);
    statusList->addItem("Auth mock activo: Sí");
    statusList->addItem("DB nueva usada por UI: No");
    statusList->addItem("Chat IA UI: Completo en modo demo");
    statusList->addItem("Notificaciones UI: Completo en modo demo");
    statusList->addItem("Límite de admins esperado por backend: 3");
    statusList->addItem("Polling sugerido para notificaciones: 10/15 segundos");

    auto *notes = new QTextEdit(page);
    notes->setReadOnly(true);
    notes->setPlainText(
        "Contexto del front actual:\n"
        "- Login y registro siguen siendo mock/legacy.\n"
        "- Usuario, Empresa y Admin ya cubren el flujo visual completo que exige la base final.\n"
        "- Falta únicamente conectar los datos reales y reemplazar los hardcodeos cuando se migre a FastAPI.");

    auto *row = new QHBoxLayout;
    auto *toggleDemo = new QPushButton(tr("Ver estado demo"), page);
    auto *reviewScope = new QPushButton(tr("Revisar alcance"), page);
    connect(toggleDemo, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Sistema"), tr("El sistema sigue operando en modo front completo + datos demo hardcodeados."));
    });
    connect(reviewScope, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Sistema"), tr("El alcance actual cubre perfiles, CVs, vacantes, postulaciones, solicitudes, IA, notificaciones y auditoría a nivel visual."));
    });
    row->addWidget(toggleDemo);
    row->addWidget(reviewScope);

    layout->addWidget(new QLabel(tr("Estado del sistema"), page));
    layout->addWidget(statusList);
    layout->addWidget(new QLabel(tr("Notas de operación"), page));
    layout->addWidget(notes);
    layout->addLayout(row);
    return page;
}
