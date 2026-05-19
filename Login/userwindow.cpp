#include "userwindow.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

UserWindow::UserWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
}

void UserWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    m_welcomeLabel = new QLabel(tr("Panel Usuario - %1").arg(displayName), central);
    m_welcomeLabel->setObjectName("uwTitle");

    m_bellButton = new QToolButton(central);
    m_bellButton->setText("🔔");
    m_bellButton->setObjectName("bellButton");
    m_bellButton->setToolTip(tr("Notificaciones"));
    connect(m_bellButton, &QToolButton::clicked, this, &UserWindow::showNotificationsPopup);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_welcomeLabel);
    topRow->addStretch();
    topRow->addWidget(m_bellButton);

    auto *contentRow = new QHBoxLayout;
    m_menuList = new QListWidget(central);
    m_menuList->addItems({tr("CV"), tr("Puestos"), tr("Solicitudes"), tr("Chat IA")});
    m_menuList->setCurrentRow(0);
    m_menuList->setObjectName("menuList");
    m_menuList->setFixedWidth(220);

    m_contentStack = new QStackedWidget(central);
    m_contentStack->addWidget(createCvTab());
    m_contentStack->addWidget(createJobsTab());
    m_contentStack->addWidget(createApplicationsTab());
    m_contentStack->addWidget(createChatTab());

    connect(m_menuList, &QListWidget::currentRowChanged, m_contentStack, &QStackedWidget::setCurrentIndex);

    m_notificationsPopup = createNotificationPopup();

    contentRow->addWidget(m_menuList);
    contentRow->addWidget(m_contentStack, 1);

    layout->addLayout(topRow);
    layout->addLayout(contentRow);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    setStyleSheet(
        "#uwTitle { font-size: 18pt; font-weight: 700; color: #f4eefe; }"
        "#menuList {"
        "  background: #1a1028;"
        "  border: 1px solid #4d2d78;"
        "  border-radius: 16px;"
        "  padding: 10px;"
        "}"
        "#menuList::item {"
        "  background: transparent;"
        "  border-radius: 12px;"
        "  color: #d9c8f5;"
        "  margin: 4px 0;"
        "  padding: 10px 12px;"
        "}"
        "#menuList::item:selected {"
        "  background: #6b3fa6;"
        "  color: #ffffff;"
        "  border: 1px solid #9f79d9;"
        "}"
        "QFrame#card {"
        "  background: #1e1230;"
        "  border: 1px solid #5c3a8f;"
        "  border-radius: 16px;"
        "}"
        "QPushButton#violetButton {"
        "  background-color: #6a3ca8;"
        "  border: 1px solid #9f79d9;"
        "  color: white;"
        "  border-radius: 12px;"
        "  padding: 10px 14px;"
        "}"
        "QPushButton#violetButton:hover { background-color: #7a49ba; }"
        "QPushButton#outlineViolet {"
        "  background-color: #23153a;"
        "  border: 1px solid #7b57b6;"
        "  color: #eadbff;"
        "  border-radius: 12px;"
        "  padding: 10px 14px;"
        "}"
        "QPushButton#outlineViolet:hover { background-color: #2d1b47; }"
        "QToolButton#bellButton {"
        "  background-color: #2a1842;"
        "  border: 1px solid #7b57b6;"
        "  color: #f2e7ff;"
        "  border-radius: 14px;"
        "  padding: 8px 12px;"
        "  font-size: 16pt;"
        "}"
        "QToolButton#bellButton:hover { background-color: #3a245a; }"
    );

    setWindowTitle(tr("Panel usuario"));
    resize(1080, 720);
}

QWidget *UserWindow::createCvTab()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    auto *hint = new QLabel(tr("Subi tu CV para postularte. Los archivos quedan listos para revisión de empresas."), page);
    hint->setWordWrap(true);
    hint->setAlignment(Qt::AlignCenter);

    auto *centerCard = new QFrame(page);
    centerCard->setObjectName("card");
    auto *cardLayout = new QVBoxLayout(centerCard);

    auto *title = new QLabel(tr("Tus CVs"), centerCard);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #f4eefe;");

    m_cvList = new QListWidget(centerCard);
    m_cvList->addItem("cv_backend_junior.pdf");
    m_cvList->addItem("cv_fullstack_2026.pdf");
    m_cvList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cvList->setStyleSheet("background:#2a1842; border:1px solid #6a46a3; border-radius: 12px; color:#f2e7ff;");

    auto *buttonRow = new QHBoxLayout;
    auto *upload = new QPushButton(tr("Subir CV"), centerCard);
    auto *view = new QPushButton(tr("Ver CV seleccionado"), centerCard);
    auto *remove = new QPushButton(tr("Eliminar CV"), centerCard);
    upload->setObjectName("violetButton");
    view->setObjectName("outlineViolet");
    remove->setObjectName("outlineViolet");
    connect(upload, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString fileName = QInputDialog::getText(this, tr("Subir CV"), tr("Nombre del archivo CV:"), QLineEdit::Normal, "", &ok).trimmed();
        if (ok && !fileName.isEmpty()) {
            m_cvList->addItem(fileName);
            m_cvList->setCurrentRow(m_cvList->count() - 1);
            pushNotification(tr("[Sistema] CV subido: %1").arg(fileName));
        }
    });

    connect(view, &QPushButton::clicked, this, [this]() {
        auto *current = m_cvList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("CV"), tr("Selecciona un CV para visualizar."));
            return;
        }
        QMessageBox::information(this, tr("CV seleccionado"), tr("Visualizando: %1").arg(current->text()));
    });

    connect(remove, &QPushButton::clicked, this, [this]() {
        auto *current = m_cvList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("CV"), tr("Selecciona un CV para eliminar."));
            return;
        }
        const QString removed = current->text();
        delete m_cvList->takeItem(m_cvList->row(current));
        pushNotification(tr("[Sistema] CV eliminado: %1").arg(removed));
    });

    buttonRow->addWidget(upload);
    buttonRow->addWidget(view);
    buttonRow->addWidget(remove);

    cardLayout->addWidget(title);
    cardLayout->addWidget(hint);
    cardLayout->addWidget(m_cvList);
    cardLayout->addLayout(buttonRow);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setColumnStretch(2, 2);
    layout->addWidget(centerCard, 0, 1);
    return page;
}

QWidget *UserWindow::createJobsTab()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    m_jobsList = new QListWidget(page);
    m_jobsList->setObjectName("cardsList");
    m_jobsList->setStyleSheet(
        "QListWidget#cardsList { background:#1e1230; border:1px solid #5c3a8f; border-radius:16px; color:#f1e6ff; }"
        "QListWidget#cardsList::item { border:1px solid #6a46a3; border-radius:12px; margin:6px; padding:10px; background:#2a1842; }"
        "QListWidget#cardsList::item:selected { background:#6b3fa6; border-color:#a27bdd; }"
    );
    m_jobsList->addItem("TechNova\nPuesto: Backend Python\nSkills: Python, FastAPI, SQL\nMatch: 92%");
    m_jobsList->addItem("Blue Systems\nPuesto: C++ Qt Developer\nSkills: C++, Qt\nMatch: 88%");
    m_jobsList->addItem("CloudWare\nPuesto: DevOps Jr\nSkills: Linux, Docker\nMatch: 64%");
    m_jobsList->setCurrentRow(0);

    auto *detailCard = new QFrame(page);
    detailCard->setObjectName("card");
    auto *detailLayout = new QVBoxLayout(detailCard);
    auto *title = new QLabel(tr("Detalle de publicación seleccionada"), detailCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #f4eefe;");
    m_jobDetailLabel = new QLabel(tr("Empresa: TechNova\nPuesto: Backend Python\nModalidad: Remoto\nDescripción: APIs REST y mantenimiento de microservicios."), detailCard);
    m_jobDetailLabel->setWordWrap(true);
    auto *buttonRow = new QHBoxLayout;
    auto *viewInfo = new QPushButton(tr("Ver info"), detailCard);
    auto *apply = new QPushButton(tr("Postularme"), detailCard);
    viewInfo->setObjectName("outlineViolet");
    apply->setObjectName("violetButton");
    buttonRow->addWidget(viewInfo);
    buttonRow->addWidget(apply);
    detailLayout->addWidget(title);
    connect(m_jobsList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            m_jobDetailLabel->setText(text + "\nModalidad: Remoto\nDescripción: Publicación recomendada por tus skills.");
        }
    });

    connect(viewInfo, &QPushButton::clicked, this, [this]() {
        auto *current = m_jobsList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("Puestos"), tr("Selecciona un puesto para ver información."));
            return;
        }
        QMessageBox::information(this, tr("Detalle del puesto"), current->text());
    });

    connect(apply, &QPushButton::clicked, this, [this]() {
        auto *current = m_jobsList->currentItem();
        if (!current) {
            QMessageBox::information(this, tr("Puestos"), tr("Selecciona un puesto para postularte."));
            return;
        }
        const QString cardText = current->text();
        const QStringList lines = cardText.split('\n');
        if (lines.size() >= 2) {
            m_applicationsList->addItem(lines.at(0) + "\n" + lines.at(1) + "\nEstado: Postulada\nÚltima actualización: Ahora");
            pushNotification(tr("[Postulación] Enviada a %1").arg(lines.at(0)));
            QMessageBox::information(this, tr("Postulación"), tr("Tu solicitud fue enviada correctamente."));
        }
    });

    detailLayout->addWidget(m_jobDetailLabel);
    detailLayout->addStretch();
    detailLayout->addLayout(buttonRow);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(m_jobsList, 0, 1);
    layout->addWidget(detailCard, 0, 2);
    return page;
}

QWidget *UserWindow::createApplicationsTab()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    m_applicationsList = new QListWidget(page);
    m_applicationsList->setStyleSheet(
        "QListWidget { background:#1e1230; border:1px solid #5c3a8f; border-radius:16px; color:#f1e6ff; }"
        "QListWidget::item { border:1px solid #6a46a3; border-radius:12px; margin:6px; padding:10px; background:#2a1842; }"
        "QListWidget::item:selected { background:#6b3fa6; border-color:#a27bdd; }"
    );
    m_applicationsList->addItem("Blue Systems\nPuesto: C++ Qt Developer\nEstado: Seleccionada\nÚltima actualización: Ayer 18:20");
    m_applicationsList->addItem("TechNova\nPuesto: Backend Python\nEstado: EnRevision\nÚltima actualización: Hoy 10:35");
    m_applicationsList->addItem("DataLoop\nPuesto: Data Analyst\nEstado: Postulada\nÚltima actualización: Hace 2 días");

    auto *infoCard = new QFrame(page);
    infoCard->setObjectName("card");
    auto *infoLayout = new QVBoxLayout(infoCard);
    auto *title = new QLabel(tr("Información de solicitud"), infoCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #f4eefe;");
    m_applicationDetailLabel = new QLabel(tr("Selecciona una tarjeta para ver detalle de empresa, requisitos adicionales y próximos pasos."), infoCard);
    m_applicationDetailLabel->setWordWrap(true);

    connect(m_applicationsList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            m_applicationDetailLabel->setText(text + "\nPróximo paso: revisar notificaciones y responder solicitud de empresa.");
        }
    });
    infoLayout->addWidget(title);
    infoLayout->addWidget(m_applicationDetailLabel);
    infoLayout->addStretch();

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(m_applicationsList, 0, 1);
    layout->addWidget(infoCard, 0, 2);
    return page;
}

QWidget *UserWindow::createChatTab()
{
    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);

    auto *chatCard = new QFrame(page);
    chatCard->setObjectName("card");
    auto *chatLayout = new QVBoxLayout(chatCard);

    auto *title = new QLabel(tr("Chat IA"), chatCard);
    title->setStyleSheet("font-size: 14pt; font-weight: 700; color: #f4eefe;");

    m_chatHistory = new QTextEdit(chatCard);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setStyleSheet("background:#150e24; border:1px solid #563682; border-radius:12px; color:#f5edff;");
    m_chatHistory->setHtml(
        "<div style='margin:8px;'>"
        "<div style='background:#6b3fa6; color:white; padding:10px 12px; border-radius:14px; width:55%; margin-left:auto; margin-bottom:8px;'>"
        "Hola IA, ¿qué puesto me conviene con Python y SQL?"
        "</div>"
        "<div style='background:#2c1b45; color:#f5edff; padding:10px 12px; border-radius:14px; width:68%; margin-bottom:8px;'>"
        "Con tu perfil te conviene Backend Python en TechNova y Data Analyst en DataLoop."
        "</div>"
        "<div style='background:#6b3fa6; color:white; padding:10px 12px; border-radius:14px; width:50%; margin-left:auto; margin-bottom:8px;'>"
        "¿Qué me falta para mejorar el match?"
        "</div>"
        "<div style='background:#2c1b45; color:#f5edff; padding:10px 12px; border-radius:14px; width:70%;'>"
        "Sumá FastAPI, testing y dockerización de servicios para subir tu match arriba de 90%."
        "</div>"
        "</div>"
    );

    auto *inputRow = new QHBoxLayout;
    m_chatInput = new QLineEdit(chatCard);
    m_chatInput->setPlaceholderText(tr("Escribe tu consulta a la IA..."));
    m_chatInput->setStyleSheet("background:#24163a; border:1px solid #6a46a3; border-radius:12px; color:#f5edff; padding:8px;");
    auto *sendButton = new QPushButton(tr("Enviar"), chatCard);
    sendButton->setObjectName("violetButton");
    connect(sendButton, &QPushButton::clicked, this, [this]() {
        const QString msg = m_chatInput->text().trimmed();
        if (msg.isEmpty()) {
            return;
        }
        m_chatHistory->append("<div style='background:#6b3fa6; color:white; padding:8px 10px; border-radius:12px; margin:6px 0; text-align:right;'><b>Tú:</b> " + msg.toHtmlEscaped() + "</div>");
        m_chatHistory->append("<div style='background:#2c1b45; color:#f5edff; padding:8px 10px; border-radius:12px; margin:6px 0;'><b>IA:</b> Entendido. Te recomiendo revisar requisitos del puesto y actualizar tu CV con proyectos recientes.</div>");
        m_chatInput->clear();
    });
    inputRow->addWidget(m_chatInput);
    inputRow->addWidget(sendButton);

    chatLayout->addWidget(title);
    chatLayout->addWidget(m_chatHistory);
    chatLayout->addLayout(inputRow);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 2);
    layout->addWidget(chatCard, 0, 1, 1, 2);
    return page;
}

QFrame *UserWindow::createNotificationPopup()
{
    auto *popup = new QFrame(nullptr);
    popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    popup->setObjectName("card");
    popup->setMinimumWidth(420);

    auto *layout = new QVBoxLayout(popup);
    auto *title = new QLabel(tr("Notificaciones"), popup);
    title->setStyleSheet("font-size: 12pt; font-weight: 700; color: #f4eefe;");

    m_notificationsList = new QListWidget(popup);
    m_notificationsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_notificationsList->addItem("[Aceptada] Blue Systems: Fuiste seleccionado para C++ Qt Developer.");
    m_notificationsList->addItem("[Solicitud] TechNova pide CV firmado y disponibilidad horaria.");
    m_notificationsList->addItem("[Sistema] Perfil actualizado correctamente.");
    m_notificationsList->setStyleSheet("background:#24163a; border:1px solid #6a46a3; border-radius:12px; color:#f5edff;");

    auto *row = new QHBoxLayout;
    auto *markOne = new QPushButton(tr("Marcar seleccionada como leída"), popup);
    auto *markAll = new QPushButton(tr("Marcar todas como leídas"), popup);
    markOne->setObjectName("outlineViolet");
    markAll->setObjectName("violetButton");

    connect(markOne, &QPushButton::clicked, this, [this]() {
        auto *current = m_notificationsList->currentItem();
        if (current) {
            current->setText("[Leída] " + current->text());
        }
    });
    connect(markAll, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_notificationsList->count(); ++i) {
            QListWidgetItem *item = m_notificationsList->item(i);
            if (!item->text().startsWith("[Leída]")) {
                item->setText("[Leída] " + item->text());
            }
        }
    });

    row->addWidget(markOne);
    row->addWidget(markAll);

    layout->addWidget(title);
    layout->addWidget(m_notificationsList);
    layout->addLayout(row);
    layout->setContentsMargins(14, 14, 14, 14);
    return popup;
}

void UserWindow::showNotificationsPopup()
{
    if (!m_notificationsPopup) {
        return;
    }

    QPoint pos = m_bellButton->mapToGlobal(QPoint(0, m_bellButton->height() + 8));
    m_notificationsPopup->move(pos.x() - m_notificationsPopup->width() + m_bellButton->width(), pos.y());
    m_notificationsPopup->show();
}

void UserWindow::pushNotification(const QString &text)
{
    if (!m_notificationsList) {
        return;
    }
    m_notificationsList->insertItem(0, text);
}
