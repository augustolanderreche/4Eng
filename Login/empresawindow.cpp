#include "empresawindow.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QComboBox>
#include <QVBoxLayout>

EmpresaWindow::EmpresaWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
}

void EmpresaWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    m_welcomeLabel = new QLabel(tr("Panel Empresa - %1").arg(displayName), central);
    m_welcomeLabel->setObjectName("ewTitle");
    m_welcomeLabel->setAlignment(Qt::AlignCenter);

    auto *contentRow = new QHBoxLayout;
    m_menuList = new QListWidget(central);
    m_menuList->addItems({tr("Perfil"), tr("Publicaciones"), tr("Postulaciones"), tr("CVs"), tr("Solicitudes"), tr("Chat IA")});
    m_menuList->setCurrentRow(0);
    m_menuList->setObjectName("empresaMenuList");
    m_menuList->setFixedWidth(230);
    m_newPostMenuItem = new QListWidgetItem(tr("Nueva publicación"), m_menuList);
    m_newPostMenuItem->setHidden(true);

    m_contentStack = new QStackedWidget(central);
    m_contentStack->addWidget(createPerfilPage());
    m_contentStack->addWidget(createPublicacionesPage());
    m_contentStack->addWidget(createPostulacionesPage());
    m_contentStack->addWidget(createCvsPage());
    m_contentStack->addWidget(createSolicitudesPage());
    m_contentStack->addWidget(createChatPage());
    m_newPostPageIndex = m_contentStack->addWidget(createNuevaPublicacionPage());

    connect(m_menuList, &QListWidget::currentRowChanged, m_contentStack, &QStackedWidget::setCurrentIndex);

    contentRow->addWidget(m_menuList);
    contentRow->addWidget(m_contentStack, 1);

    layout->addWidget(m_welcomeLabel);
    layout->addLayout(contentRow);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    setStyleSheet(
        "#ewTitle { font-size: 18pt; font-weight: 700; color: #ecf7ff; }"
        "#empresaMenuList {"
        "  background: #0f1f33;"
        "  border: 1px solid #2b6cb0;"
        "  border-radius: 16px;"
        "  padding: 10px;"
        "}"
        "#empresaMenuList::item {"
        "  background: transparent;"
        "  border-radius: 12px;"
        "  color: #c8e7ff;"
        "  margin: 4px 0;"
        "  padding: 10px 12px;"
        "}"
        "#empresaMenuList::item:selected {"
        "  background: #1f6fb8;"
        "  color: #ffffff;"
        "  border: 1px solid #78b9ef;"
        "}"
        "QFrame#empresaCard {"
        "  background: #14263d;"
        "  border: 1px solid #2f5d8f;"
        "  border-radius: 16px;"
        "}"
        "QPushButton#blueButton {"
        "  background-color: #1f6fb8;"
        "  border: 1px solid #6eb1ea;"
        "  color: white;"
        "  border-radius: 12px;"
        "  padding: 10px 14px;"
        "}"
        "QPushButton#blueButton:hover { background-color: #2781d5; }"
        "QPushButton#outlineBlue {"
        "  background-color: #18314d;"
        "  border: 1px solid #4278b0;"
        "  color: #d8eeff;"
        "  border-radius: 12px;"
        "  padding: 10px 14px;"
        "}"
        "QPushButton#outlineBlue:hover { background-color: #214264; }"
    );

    setWindowTitle(tr("Panel empresa"));
    resize(1120, 740);
}

QWidget *EmpresaWindow::createPerfilPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    auto *summaryCard = new QFrame(page);
    summaryCard->setObjectName("empresaCard");
    auto *summaryLayout = new QVBoxLayout(summaryCard);
    auto *title = new QLabel(tr("Perfil de empresa"), summaryCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");

    auto *summary = new QLabel(
        tr("Empresa: Empresa Demo\n"
           "Industria: Software / Tecnología\n"
           "Tamaño: 11-50\n"
           "Contacto principal: Responsable RRHH\n"
           "Teléfono: +54 351 000000\n"
           "Website: empresademo.com\n"
           "Descripción: Empresa demo para pruebas del panel empresa."),
        summaryCard);
    summary->setWordWrap(true);

    auto *actions = new QHBoxLayout;
    auto *edit = new QPushButton(tr("Editar perfil"), summaryCard);
    auto *branding = new QPushButton(tr("Actualizar datos públicos"), summaryCard);
    edit->setObjectName("blueButton");
    branding->setObjectName("outlineBlue");
    connect(edit, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Perfil empresa"), tr("El perfil quedó actualizado a nivel visual para la demo."));
    });
    connect(branding, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Perfil empresa"), tr("Sitio web, industria y contacto quedaron listos para la demo."));
    });
    actions->addWidget(edit);
    actions->addWidget(branding);

    summaryLayout->addWidget(title);
    summaryLayout->addWidget(summary);
    summaryLayout->addStretch();
    summaryLayout->addLayout(actions);

    auto *metricsCard = new QFrame(page);
    metricsCard->setObjectName("empresaCard");
    auto *metricsLayout = new QVBoxLayout(metricsCard);
    auto *metricsTitle = new QLabel(tr("Actividad reciente"), metricsCard);
    metricsTitle->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");
    auto *metricsList = new QListWidget(metricsCard);
    metricsList->addItem("Vacantes activas: 2");
    metricsList->addItem("Vacantes pausadas: 1");
    metricsList->addItem("Postulaciones en revisión: 1");
    metricsList->addItem("Candidatos seleccionados: 1");
    metricsList->addItem("Solicitudes pendientes: 2");
    metricsList->setStyleSheet(
        "QListWidget { background:#1a3350; border:1px solid #3f77ae; border-radius:16px; color:#e3f4ff; }"
        "QListWidget::item { padding:10px; }"
    );

    metricsLayout->addWidget(metricsTitle);
    metricsLayout->addWidget(metricsList);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(summaryCard, 0, 1);
    layout->addWidget(metricsCard, 0, 2);
    return page;
}

QWidget *EmpresaWindow::createPublicacionesPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    m_publicacionesList = new QListWidget(page);
    m_publicacionesList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_publicacionesList->setStyleSheet(
        "QListWidget { background:#14263d; border:1px solid #2f5d8f; border-radius:16px; color:#e3f4ff; }"
        "QListWidget::item { border:1px solid #3f77ae; border-radius:12px; margin:6px; padding:10px; background:#1a3350; }"
        "QListWidget::item:selected { background:#1f6fb8; border-color:#78b9ef; }"
    );
    m_publicacionesList->addItem("Backend Python\nSkills: Python, FastAPI, SQL, Docker\nEstado: Abierta | Postulantes: 2");
    m_publicacionesList->addItem("C++ Qt Developer\nSkills: C++, Qt\nEstado: Abierta | Postulantes: 1");
    m_publicacionesList->addItem("Data Analyst\nSkills: SQL, Python\nEstado: Pausada | Postulantes: 0");

    auto *detailCard = new QFrame(page);
    detailCard->setObjectName("empresaCard");
    auto *detailLayout = new QVBoxLayout(detailCard);
    auto *title = new QLabel(tr("Detalle de publicación"), detailCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");
    m_publicacionDetailLabel = new QLabel(tr("Selecciona una publicación para editar requisitos, salario, ubicación y estado."), detailCard);
    m_publicacionDetailLabel->setWordWrap(true);
    auto *buttonRow = new QHBoxLayout;
    auto *newPost = new QPushButton(tr("Nueva publicación"), detailCard);
    auto *edit = new QPushButton(tr("Editar"), detailCard);
    auto *pause = new QPushButton(tr("Pausar/Cerrar"), detailCard);
    newPost->setObjectName("blueButton");
    edit->setObjectName("outlineBlue");
    pause->setObjectName("outlineBlue");
    connect(m_publicacionesList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            m_publicacionDetailLabel->setText(text + "\nSeniority: Junior/SemiSenior según puesto\nUbicación: Argentina\nUsa los botones para editar o pausar la publicación.");
        }
    });

    connect(newPost, &QPushButton::clicked, this, [this]() {
        m_newPostMenuItem->setHidden(false);
        m_menuList->setCurrentRow(m_menuList->count() - 1);
        m_contentStack->setCurrentIndex(m_newPostPageIndex);
    });

    connect(edit, &QPushButton::clicked, this, [this]() {
        auto *current = m_publicacionesList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("Publicaciones"), tr("Selecciona una publicación para editar."));
            return;
        }
        bool ok = false;
        const QString nuevoTitulo = QInputDialog::getText(this, tr("Editar publicación"), tr("Nuevo título del puesto:"), QLineEdit::Normal, "", &ok).trimmed();
        if (ok && !nuevoTitulo.isEmpty()) {
            QStringList lines = current->text().split('\n');
            if (!lines.isEmpty()) {
                lines[0] = nuevoTitulo;
                current->setText(lines.join("\n"));
            }
        }
    });

    connect(pause, &QPushButton::clicked, this, [this]() {
        auto *current = m_publicacionesList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("Publicaciones"), tr("Selecciona una publicación para cambiar estado."));
            return;
        }
        QString text = current->text();
        if (text.contains("Estado: Abierta")) {
            text.replace("Estado: Abierta", "Estado: Pausada");
        } else if (text.contains("Estado: Pausada")) {
            text.replace("Estado: Pausada", "Estado: Cerrada");
        } else {
            text.replace("Estado: Cerrada", "Estado: Abierta");
        }
        current->setText(text);
    });

    buttonRow->addWidget(newPost);
    buttonRow->addWidget(edit);
    buttonRow->addWidget(pause);

    detailLayout->addWidget(title);
    detailLayout->addWidget(m_publicacionDetailLabel);
    detailLayout->addStretch();
    detailLayout->addLayout(buttonRow);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(m_publicacionesList, 0, 1);
    layout->addWidget(detailCard, 0, 2);
    return page;
}

QWidget *EmpresaWindow::createPostulacionesPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    m_postulacionesList = new QListWidget(page);
    m_postulacionesList->setStyleSheet(
        "QListWidget { background:#14263d; border:1px solid #2f5d8f; border-radius:16px; color:#e3f4ff; }"
        "QListWidget::item { border:1px solid #3f77ae; border-radius:12px; margin:6px; padding:10px; background:#1a3350; }"
        "QListWidget::item:selected { background:#1f6fb8; border-color:#78b9ef; }"
    );
    m_postulacionesList->addItem("Backend Python\nSofia Diaz\nSkills: Python, FastAPI, Docker\nEstado: Seleccionada | Match: 96%");
    m_postulacionesList->addItem("Backend Python\nUsuario Demo\nSkills: Python, SQL\nEstado: EnRevision | Match: 82%");
    m_postulacionesList->addItem("C++ Qt Developer\nMarcos Gomez\nSkills: C++, Qt\nEstado: Postulada | Match: 91%");

    auto *detailCard = new QFrame(page);
    detailCard->setObjectName("empresaCard");
    auto *detailLayout = new QVBoxLayout(detailCard);
    auto *title = new QLabel(tr("Acciones sobre candidato"), detailCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");
    m_postulacionDetailLabel = new QLabel(tr("Revisá el candidato, su match y decidí el avance del proceso de selección."), detailCard);
    m_postulacionDetailLabel->setWordWrap(true);

    auto *row = new QHBoxLayout;
    auto *accept = new QPushButton(tr("Aceptar"), detailCard);
    auto *reject = new QPushButton(tr("Rechazar"), detailCard);
    auto *request = new QPushButton(tr("Solicitar más datos"), detailCard);
    auto *evaluation = new QPushButton(tr("Ver match IA"), detailCard);
    accept->setObjectName("blueButton");
    reject->setObjectName("outlineBlue");
    request->setObjectName("outlineBlue");
    evaluation->setObjectName("outlineBlue");
    row->addWidget(accept);
    row->addWidget(reject);
    row->addWidget(request);
    row->addWidget(evaluation);

    connect(m_postulacionesList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            m_postulacionDetailLabel->setText(text + "\nAcción sugerida: actualizar estado y registrar observaciones.");
        }
    });

    connect(accept, &QPushButton::clicked, this, [this]() {
        auto *current = m_postulacionesList->currentItem();
        if (!current) {
            return;
        }
        QString txt = current->text();
        txt.replace(QRegularExpression("Estado: [^|\\n]+"), "Estado: Seleccionada");
        current->setText(txt);
    });

    connect(reject, &QPushButton::clicked, this, [this]() {
        auto *current = m_postulacionesList->currentItem();
        if (!current) {
            return;
        }
        QString txt = current->text();
        txt.replace(QRegularExpression("Estado: [^|\\n]+"), "Estado: Rechazada");
        current->setText(txt);
    });

    connect(request, &QPushButton::clicked, this, [this]() {
        auto *current = m_postulacionesList->currentItem();
        if (!current) {
            return;
        }
        QMessageBox::information(this, tr("Solicitud enviada"), tr("Se solicitó información adicional al candidato."));
    });

    connect(evaluation, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 tr("Evaluación IA del candidato"),
                                 tr("Recomendación: RecomendadaConCapacitacion\n"
                                    "Fortalezas: Python, SQL\n"
                                    "Faltantes: FastAPI, Docker, Testing\n"
                                    "Resumen empresa: candidato con buena base, requiere capacitación en herramientas del stack."));
    });

    detailLayout->addWidget(title);
    detailLayout->addWidget(m_postulacionDetailLabel);
    detailLayout->addStretch();
    detailLayout->addLayout(row);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(m_postulacionesList, 0, 1);
    layout->addWidget(detailCard, 0, 2);
    return page;
}

QWidget *EmpresaWindow::createCvsPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    m_cvsList = new QListWidget(page);
    m_cvsList->addItem("Sofia Diaz - cv_backend_python.pdf - ANALYZED");
    m_cvsList->addItem("Marcos Gomez - cv_cpp_qt.pdf - ANALYZED");
    m_cvsList->addItem("Usuario Demo - cv_fullstack_2026.pdf - ANALYZED");
    m_cvsList->setStyleSheet(
        "QListWidget { background:#14263d; border:1px solid #2f5d8f; border-radius:16px; color:#e3f4ff; }"
        "QListWidget::item { border:1px solid #3f77ae; border-radius:12px; margin:6px; padding:10px; background:#1a3350; }"
        "QListWidget::item:selected { background:#1f6fb8; border-color:#78b9ef; }"
    );

    auto *detailCard = new QFrame(page);
    detailCard->setObjectName("empresaCard");
    auto *detailLayout = new QVBoxLayout(detailCard);
    auto *title = new QLabel(tr("Lectura y descarga de CV"), detailCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");
    m_cvDetailLabel = new QLabel(tr("Selecciona un CV para ver contenido, descargar o abrir el resumen generado por IA."), detailCard);
    m_cvDetailLabel->setWordWrap(true);

    auto *buttonRow = new QHBoxLayout;
    auto *read = new QPushButton(tr("Leer CV"), detailCard);
    auto *download = new QPushButton(tr("Descargar CV"), detailCard);
    auto *summary = new QPushButton(tr("Ver resumen IA"), detailCard);
    read->setObjectName("blueButton");
    download->setObjectName("outlineBlue");
    summary->setObjectName("outlineBlue");
    buttonRow->addWidget(read);
    buttonRow->addWidget(download);
    buttonRow->addWidget(summary);

    connect(m_cvsList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            m_cvDetailLabel->setText(tr("CV seleccionado: %1\nOpciones disponibles: leer, descargar y resumen IA.").arg(text));
        }
    });

    connect(read, &QPushButton::clicked, this, [this]() {
        auto *current = m_cvsList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("CVs"), tr("Selecciona un CV para leer."));
            return;
        }
        QMessageBox::information(this, tr("Lectura CV"), tr("Vista previa de %1").arg(current->text()));
    });

    connect(download, &QPushButton::clicked, this, [this]() {
        auto *current = m_cvsList->currentItem();
        if (!current) {
            return;
        }
        QMessageBox::information(this, tr("Descarga"), tr("Se descargó %1").arg(current->text()));
    });

    connect(summary, &QPushButton::clicked, this, [this]() {
        auto *current = m_cvsList->currentItem();
        if (!current) {
            return;
        }
        QMessageBox::information(this,
                                 tr("Resumen IA"),
                                 tr("Resumen IA para %1:\n"
                                    "Fortalezas: Python, FastAPI, SQL\n"
                                    "Gaps: Docker avanzado, testing\n"
                                    "Recomendación: perfil apto para APIs REST y backend Python.")
                                     .arg(current->text()));
    });

    detailLayout->addWidget(title);
    detailLayout->addWidget(m_cvDetailLabel);
    detailLayout->addStretch();
    detailLayout->addLayout(buttonRow);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(m_cvsList, 0, 1);
    layout->addWidget(detailCard, 0, 2);
    return page;
}

QWidget *EmpresaWindow::createSolicitudesPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    m_solicitudesList = new QListWidget(page);
    m_solicitudesList->setStyleSheet(
        "QListWidget { background:#14263d; border:1px solid #2f5d8f; border-radius:16px; color:#e3f4ff; }"
        "QListWidget::item { border:1px solid #3f77ae; border-radius:12px; margin:6px; padding:10px; background:#1a3350; }"
        "QListWidget::item:selected { background:#1f6fb8; border-color:#78b9ef; }"
    );
    m_solicitudesList->addItem("Sofia Diaz\nUPLOAD_CV\nSubir CV firmado y portfolio\nEstado: Pending");
    m_solicitudesList->addItem("Usuario Demo\nREQUEST_DOCUMENTS\nEnviar documentación adicional\nEstado: Pending");
    m_solicitudesList->addItem("Marcos Gomez\nCUSTOM_MESSAGE\nConsulta sobre experiencia Qt\nEstado: Pending");

    auto *detailCard = new QFrame(page);
    detailCard->setObjectName("empresaCard");
    auto *detailLayout = new QVBoxLayout(detailCard);
    auto *title = new QLabel(tr("Enviar solicitud a candidato"), detailCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");
    m_solicitudDetailLabel = new QLabel(tr("Gestiona solicitudes de CV, reuniones y documentos para avanzar contrataciones."), detailCard);
    m_solicitudDetailLabel->setWordWrap(true);

    auto *row = new QHBoxLayout;
    auto *cvReq = new QPushButton(tr("Pedir carga de CV"), detailCard);
    auto *meeting = new QPushButton(tr("Agendar reunión"), detailCard);
    auto *docs = new QPushButton(tr("Pedir documentos"), detailCard);
    auto *custom = new QPushButton(tr("Mensaje libre"), detailCard);
    cvReq->setObjectName("blueButton");
    meeting->setObjectName("outlineBlue");
    docs->setObjectName("outlineBlue");
    custom->setObjectName("outlineBlue");
    row->addWidget(cvReq);
    row->addWidget(meeting);
    row->addWidget(docs);
    row->addWidget(custom);

    connect(m_solicitudesList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            m_solicitudDetailLabel->setText(text + "\nPuedes enviar nuevas acciones al candidato.");
        }
    });

    connect(cvReq, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Solicitar CV"), tr("Nombre del candidato:"), QLineEdit::Normal, "", &ok).trimmed();
        if (ok && !name.isEmpty()) {
            m_solicitudesList->insertItem(0, name + "\nUPLOAD_CV\nSe solicita carga de CV actualizado\nEstado: Pending");
        }
    });

    connect(meeting, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Agendar reunión"), tr("Nombre del candidato:"), QLineEdit::Normal, "", &ok).trimmed();
        if (ok && !name.isEmpty()) {
            m_solicitudesList->insertItem(0, name + "\nSCHEDULE_MEETING\nReunión técnica pendiente de confirmación\nEstado: Pending");
        }
    });

    connect(docs, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Pedir documentos"), tr("Nombre del candidato:"), QLineEdit::Normal, "", &ok).trimmed();
        if (ok && !name.isEmpty()) {
            m_solicitudesList->insertItem(0, name + "\nREQUEST_DOCUMENTS\nEnviar documentación complementaria\nEstado: Pending");
        }
    });

    connect(custom, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Mensaje libre"), tr("Nombre del candidato:"), QLineEdit::Normal, "", &ok).trimmed();
        if (ok && !name.isEmpty()) {
            m_solicitudesList->insertItem(0, name + "\nCUSTOM_MESSAGE\nEnviar comentario o consulta personalizada\nEstado: Pending");
        }
    });

    detailLayout->addWidget(title);
    detailLayout->addWidget(m_solicitudDetailLabel);
    detailLayout->addStretch();
    detailLayout->addLayout(row);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(m_solicitudesList, 0, 1);
    layout->addWidget(detailCard, 0, 2);
    return page;
}

QWidget *EmpresaWindow::createChatPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    auto *chatCard = new QFrame(page);
    chatCard->setObjectName("empresaCard");
    auto *chatLayout = new QVBoxLayout(chatCard);

    auto *title = new QLabel(tr("Chat IA para Empresa"), chatCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");

    m_chatHistory = new QTextEdit(chatCard);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setStyleSheet("background:#102033; border:1px solid #34618f; border-radius:12px; color:#eaf6ff;");
    m_chatHistory->setHtml(
        "<div style='margin:8px;'>"
        "<div style='background:#1f6fb8; color:white; padding:10px 12px; border-radius:14px; width:58%; margin-left:auto; margin-bottom:8px;'>"
        "¿Qué skills priorizo para Backend Python?"
        "</div>"
        "<div style='background:#1a3350; color:#eaf6ff; padding:10px 12px; border-radius:14px; width:68%; margin-bottom:8px;'>"
        "Priorizá FastAPI, SQL y diseño de APIs. También revisá experiencia en testing y Docker."
        "</div>"
        "<div style='background:#1f6fb8; color:white; padding:10px 12px; border-radius:14px; width:54%; margin-left:auto; margin-bottom:8px;'>"
        "¿Y para C++ Qt Junior?"
        "</div>"
        "<div style='background:#1a3350; color:#eaf6ff; padding:10px 12px; border-radius:14px; width:70%;'>"
        "Enfocá en OOP, señales/slots y manejo de UI con QWidgets."
        "</div>"
        "</div>"
    );

    auto *row = new QHBoxLayout;
    m_chatInput = new QLineEdit(chatCard);
    m_chatInput->setPlaceholderText(tr("Escribe una duda para la IA..."));
    m_chatInput->setStyleSheet("background:#17314d; border:1px solid #4278b0; border-radius:12px; color:#eaf6ff; padding:8px;");
    auto *send = new QPushButton(tr("Enviar"), chatCard);
    send->setObjectName("blueButton");
    connect(send, &QPushButton::clicked, this, [this]() {
        const QString msg = m_chatInput->text().trimmed();
        if (msg.isEmpty()) {
            return;
        }
        m_chatHistory->append("<div style='background:#1f6fb8; color:white; padding:8px 10px; border-radius:12px; margin:6px 0; text-align:right;'><b>Empresa:</b> " + msg.toHtmlEscaped() + "</div>");
        m_chatHistory->append("<div style='background:#1a3350; color:#eaf6ff; padding:8px 10px; border-radius:12px; margin:6px 0;'><b>IA:</b> Tomado. Te recomiendo priorizar candidatos con experiencia comprobable en el stack solicitado.</div>");
        m_chatInput->clear();
    });
    row->addWidget(m_chatInput);
    row->addWidget(send);

    chatLayout->addWidget(title);
    chatLayout->addWidget(m_chatHistory);
    chatLayout->addLayout(row);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(chatCard, 0, 1, 1, 2);
    return page;
}

QWidget *EmpresaWindow::createNuevaPublicacionPage()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    auto *formCard = new QFrame(page);
    formCard->setObjectName("empresaCard");
    auto *formLayout = new QVBoxLayout(formCard);

    auto *title = new QLabel(tr("Nueva publicación de trabajo"), formCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #ecf7ff;");

    m_newPostTitleEdit = new QLineEdit(formCard);
    m_newPostTitleEdit->setPlaceholderText(tr("Título del puesto"));
    m_newPostSkillsEdit = new QLineEdit(formCard);
    m_newPostSkillsEdit->setPlaceholderText(tr("Skills requeridas (ej: Python, SQL, FastAPI)"));
    m_newPostLocationEdit = new QLineEdit(formCard);
    m_newPostLocationEdit->setPlaceholderText(tr("Ubicación / ciudad"));
    m_newPostModeEdit = new QLineEdit(formCard);
    m_newPostModeEdit->setPlaceholderText(tr("Modalidad (Remoto/Híbrido/Presencial)"));
    auto *experienceEdit = new QLineEdit(formCard);
    experienceEdit->setPlaceholderText(tr("Experiencia mínima (ej: 1.0)"));
    auto *seniorityCombo = new QComboBox(formCard);
    seniorityCombo->addItems({tr("Trainee"), tr("Junior"), tr("SemiSenior"), tr("Senior"), tr("Lead")});
    auto *countryEdit = new QLineEdit(formCard);
    countryEdit->setPlaceholderText(tr("País"));
    auto *salaryMinEdit = new QLineEdit(formCard);
    salaryMinEdit->setPlaceholderText(tr("Salario mínimo"));
    auto *salaryMaxEdit = new QLineEdit(formCard);
    salaryMaxEdit->setPlaceholderText(tr("Salario máximo"));
    auto *currencyCombo = new QComboBox(formCard);
    currencyCombo->addItems({tr("ARS"), tr("USD"), tr("EUR")});
    auto *optionalSkillsEdit = new QLineEdit(formCard);
    optionalSkillsEdit->setPlaceholderText(tr("Skills deseables (ej: Docker, Testing)"));
    m_newPostDescriptionEdit = new QTextEdit(formCard);
    m_newPostDescriptionEdit->setPlaceholderText(tr("Descripción completa del puesto y responsabilidades"));
    m_newPostDescriptionEdit->setMinimumHeight(180);

    auto *row = new QHBoxLayout;
    auto *publishButton = new QPushButton(tr("Publicar"), formCard);
    auto *cancelButton = new QPushButton(tr("Cancelar"), formCard);
    publishButton->setObjectName("blueButton");
    cancelButton->setObjectName("outlineBlue");

    connect(publishButton, &QPushButton::clicked, this, [this, experienceEdit, seniorityCombo, countryEdit, salaryMinEdit, salaryMaxEdit, currencyCombo, optionalSkillsEdit]() {
        const QString title = m_newPostTitleEdit->text().trimmed();
        const QString skills = m_newPostSkillsEdit->text().trimmed();
        const QString mode = m_newPostModeEdit->text().trimmed();
        if (title.isEmpty() || skills.isEmpty() || mode.isEmpty()) {
            QMessageBox::warning(this, tr("Nueva publicación"), tr("Completa título, skills y modalidad para publicar."));
            return;
        }

        const QString card = title + "\nSkills: " + skills + "\nEstado: Abierta | Postulantes: 0";
        m_publicacionesList->insertItem(0, card);
        m_publicacionesList->setCurrentRow(0);

        m_newPostTitleEdit->clear();
        m_newPostSkillsEdit->clear();
        m_newPostLocationEdit->clear();
        m_newPostModeEdit->clear();
        experienceEdit->clear();
        seniorityCombo->setCurrentIndex(0);
        countryEdit->clear();
        salaryMinEdit->clear();
        salaryMaxEdit->clear();
        currencyCombo->setCurrentIndex(0);
        optionalSkillsEdit->clear();
        m_newPostDescriptionEdit->clear();

        hideNewPublicationMenu();
        m_menuList->setCurrentRow(1);
        m_contentStack->setCurrentIndex(1);
        QMessageBox::information(this,
                                 tr("Nueva publicación"),
                                 tr("Publicación creada correctamente.\n"
                                    "Se cargó título, skills requeridas, seniority, modalidad y rango salarial en modo mock."));
    });

    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        hideNewPublicationMenu();
        m_menuList->setCurrentRow(1);
        m_contentStack->setCurrentIndex(1);
    });

    row->addWidget(publishButton);
    row->addWidget(cancelButton);

    formLayout->addWidget(title);
    formLayout->addWidget(m_newPostTitleEdit);
    formLayout->addWidget(m_newPostSkillsEdit);
    formLayout->addWidget(m_newPostLocationEdit);
    formLayout->addWidget(m_newPostModeEdit);
    formLayout->addWidget(experienceEdit);
    formLayout->addWidget(seniorityCombo);
    formLayout->addWidget(countryEdit);
    formLayout->addWidget(salaryMinEdit);
    formLayout->addWidget(salaryMaxEdit);
    formLayout->addWidget(currencyCombo);
    formLayout->addWidget(optionalSkillsEdit);
    formLayout->addWidget(m_newPostDescriptionEdit);
    formLayout->addLayout(row);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setColumnStretch(2, 2);
    layout->addWidget(formCard, 0, 1, 1, 2);
    return page;
}

void EmpresaWindow::hideNewPublicationMenu()
{
    if (m_newPostMenuItem) {
        m_newPostMenuItem->setHidden(true);
    }
}
