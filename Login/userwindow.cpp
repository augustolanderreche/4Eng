#include "userwindow.h"

#include "api_client.h"
#include "admindb.h"
#include "mainwindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
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
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QCloseEvent>

static QString jsonValueToText(const QJsonValue &value, const QString &fallback = "-")
{
    if (value.isString()) {
        const QString text = value.toString();
        return text.isEmpty() ? fallback : text;
    }
    if (value.isBool()) {
        return value.toBool() ? "Sí" : "No";
    }
    if (value.isDouble()) {
        return QString::number(value.toVariant().toDouble());
    }
    if (value.isArray()) {
        QStringList values;
        for (const QJsonValue &item : value.toArray()) {
            values << jsonValueToText(item, QString());
        }
        return values.join(", ").isEmpty() ? fallback : values.join(", ");
    }
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    return fallback;
}

static QString jtext(const QJsonObject &obj, const QString &key, const QString &fallback = "-")
{
    return jsonValueToText(obj.value(key), fallback);
}

static QWidget *createCardWidget(const QString &title, const QString &subtitle, const QString &body)
{
    auto *card = new QWidget;
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("font-size:11pt; font-weight:700; color:#F2E9FF; border:none; background:transparent;");

    auto *subtitleLabel = new QLabel(subtitle, card);
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setStyleSheet("color:#C9B9FF; border:none; background:transparent;");

    auto *bodyLabel = new QLabel(body, card);
    bodyLabel->setWordWrap(true);
    bodyLabel->setStyleSheet("color:#EDE5FF; border:none; background:transparent;");

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(bodyLabel);

    card->setStyleSheet("background:#120a26; border:1px solid #5b49c9; border-radius:22px;");
    return card;
}

static QString formatChatHtml(const QString &text, bool fromUser)
{
    QString formatted = text.toHtmlEscaped();
    formatted.replace('\n', "<br>");
    const QString bg = fromUser ? "#5D3BF0" : "#261950";
    const QString border = fromUser ? "#8E75FF" : "#6246FF";
    const QString margin = fromUser ? "0 0 0 auto" : "0 auto 0 0";
    const QString label = fromUser ? "Tú" : "IA";

    return QStringLiteral(
        "<div style=\"max-width:78%%; margin:%1; padding:6px 0; text-align:%2;\">"
        "<div style=\"display:inline-block; background:%3; border:1px solid %4; border-radius:20px; padding:14px 18px; color:#F4EFFF; font-size:11pt; line-height:1.5; box-shadow:0 10px 24px rgba(0,0,0,0.18);\">"
        "<strong style=\"display:block; margin-bottom:8px; color:#E8DBFF;\">%5</strong>%6"
        "</div>"
        "</div>"
    ).arg(margin, fromUser ? "right" : "left", bg, border, label, formatted);
}

UserWindow::UserWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent),
      m_unreadNotifications(0)
{
    setupUi(displayName);
    loadProfile();
    loadJobs();
    loadApplications();
    refreshNotifications(true);

    if (m_notificationPollTimer) {
        m_notificationPollTimer->start(5000);
    }
}

void UserWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    central->setStyleSheet(
        "QWidget { background:#0b0719; color:#EDE6FF; font-family: 'Segoe UI', sans-serif; }"
        "QPushButton { background:#241b47; color:#F5EFFF; border:1px solid #5c47c4; border-radius:12px; padding:10px 14px; }"
        "QPushButton:hover { background:#3d2f80; }"
        "QListWidget { background:#120a22; color:#EDE6FF; border:1px solid #4e3fa2; border-radius:14px; }"
        "QListWidget::item { background:#120a26; border:1px solid #5b49c9; border-radius:18px; margin:6px; padding:12px; }"
        "QListWidget::item:selected { background:#5c47c4; color:#FFFFFF; }"
        "QLineEdit, QTextEdit { background:#11081c; color:#EEE5FF; border:1px solid #4f3fa7; border-radius:14px; padding:10px; }"
        "QLabel { color:#EDE6FF; }"
    );

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *headerRow = new QHBoxLayout;

    m_welcomeLabel = new QLabel(tr("Panel Usuario - %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 18pt; font-weight: 700; color: #f4eefe;");

    m_bellButton = new QPushButton(tr("🔔"), central);
    m_bellButton->setToolTip(tr("Ver notificaciones"));
    m_bellButton->setMinimumWidth(72);
    connect(m_bellButton, &QPushButton::clicked, this, &UserWindow::toggleNotificationPopup);

    m_logoutButton = new QPushButton(tr("Cerrar sesión"), central);
    connect(m_logoutButton, &QPushButton::clicked, this, &UserWindow::handleLogout);

    headerRow->addWidget(m_welcomeLabel, 1);
    headerRow->addWidget(m_bellButton);
    headerRow->addWidget(m_logoutButton);

    m_notificationPopup = new QFrame(central);
    m_notificationPopup->setFrameShape(QFrame::StyledPanel);
    m_notificationPopup->setObjectName("notificationPopup");
    m_notificationPopup->setStyleSheet(
        "QFrame#notificationPopup { background:#161028; border:1px solid #4f3da4; border-radius:18px; }"
    );
    m_notificationPopup->setVisible(false);

    auto *popupLayout = new QVBoxLayout(m_notificationPopup);
    popupLayout->setContentsMargins(12, 10, 12, 10);
    popupLayout->setSpacing(8);

    auto *popupTitle = new QLabel(tr("Notificaciones"), m_notificationPopup);
    popupTitle->setStyleSheet("font-weight:bold; color:#ffffff;");

    m_notificationPopupList = new QListWidget(m_notificationPopup);
    m_notificationPopupList->setMinimumHeight(120);
    m_notificationPopupList->setMaximumHeight(220);

    auto *popupButtons = new QHBoxLayout;
    auto *popupRefresh = new QPushButton(tr("Actualizar"), m_notificationPopup);
    auto *popupMarkRead = new QPushButton(tr("Marcar leída"), m_notificationPopup);
    auto *popupClose = new QPushButton(tr("Cerrar"), m_notificationPopup);

    connect(popupRefresh, &QPushButton::clicked, this, &UserWindow::loadNotifications);
    connect(popupMarkRead, &QPushButton::clicked, this, &UserWindow::markSelectedNotificationRead);
    connect(popupClose, &QPushButton::clicked, this, &UserWindow::hideNotificationPopup);

    popupButtons->addWidget(popupRefresh);
    popupButtons->addWidget(popupMarkRead);
    popupButtons->addStretch();
    popupButtons->addWidget(popupClose);

    popupLayout->addWidget(popupTitle);
    popupLayout->addWidget(m_notificationPopupList);
    popupLayout->addLayout(popupButtons);

    m_notificationPollTimer = new QTimer(this);
    connect(m_notificationPollTimer, &QTimer::timeout, this, &UserWindow::pollNotifications);

    m_notificationHideTimer = new QTimer(this);
    m_notificationHideTimer->setSingleShot(true);
    connect(m_notificationHideTimer, &QTimer::timeout, this, &UserWindow::hideNotificationPopup);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    auto *contentRow = new QHBoxLayout;
    m_menuList = new QListWidget(central);
    m_menuList->addItems({tr("Perfil"), tr("CV + IA"), tr("Puestos"), tr("Postulaciones"), tr("Notificaciones"), tr("Chat IA")});
    m_menuList->setCurrentRow(0);
    m_menuList->setFixedWidth(220);
    m_menuList->setSpacing(12);
    m_menuList->setStyleSheet(
        "QListWidget { background: transparent; border:none; }"
        "QListWidget::item { background:#120a26; border:1px solid #5f4cd1; border-radius:18px; padding:12px 10px; margin:4px; color:#EDE6FF; }"
        "QListWidget::item:selected { background:#5c47c4; color:#FFFFFF; }"
    );

    m_contentStack = new QStackedWidget(central);
    m_contentStack->addWidget(createProfileTab());
    m_contentStack->addWidget(createCvTab());
    m_contentStack->addWidget(createJobsTab());
    m_contentStack->addWidget(createApplicationsTab());
    m_contentStack->addWidget(createNotificationsTab());
    m_contentStack->addWidget(createChatTab());

    connect(m_menuList, &QListWidget::currentRowChanged, m_contentStack, &QStackedWidget::setCurrentIndex);

    contentRow->addWidget(m_menuList);
    contentRow->addWidget(m_contentStack, 1);

    layout->addLayout(headerRow);
    layout->addWidget(m_notificationPopup);
    layout->addLayout(contentRow, 1);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Panel usuario"));
    resize(1120, 760);
}

void UserWindow::setStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
    AdminDB::instance().updateLastActivity();
}

QWidget *UserWindow::createProfileTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar perfil"), page);
    connect(reload, &QPushButton::clicked, this, &UserWindow::loadProfile);

    m_profileLabel = new QLabel(page);
    m_profileLabel->setWordWrap(true);
    m_profileLabel->setStyleSheet("font-size: 11pt;");

    layout->addWidget(reload);
    layout->addWidget(m_profileLabel);
    layout->addStretch();
    return page;
}

QWidget *UserWindow::createCvTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *info = new QLabel(tr("Subí un CV y analizalo con IA. El análisis queda guardado en el VPS y una copia se cachea localmente en SQLite."), page);
    info->setWordWrap(true);

    m_cvTargetEdit = new QLineEdit(page);
    m_cvTargetEdit->setPlaceholderText(tr("Puesto objetivo para analizar compatibilidad. Ej: Desarrollador C++ Qt Junior"));

    auto *buttonRow = new QHBoxLayout;
    auto *upload = new QPushButton(tr("Subir CV sin analizar"), page);
    auto *analyze = new QPushButton(tr("Subir y analizar CV con IA"), page);
    connect(upload, &QPushButton::clicked, this, &UserWindow::uploadCv);
    connect(analyze, &QPushButton::clicked, this, &UserWindow::analyzeCv);
    buttonRow->addWidget(upload);
    buttonRow->addWidget(analyze);

    m_cvList = new QListWidget(page);
    m_cvList->addItem(tr("Los CV subidos aparecerán acá durante esta sesión."));

    m_cvAnalysisText = new QTextEdit(page);
    m_cvAnalysisText->setReadOnly(true);
    m_cvAnalysisText->setPlaceholderText(tr("Acá se mostrará el feedback de IA: fortalezas, debilidades, compatibilidad y recomendaciones."));

    layout->addWidget(info);
    layout->addWidget(m_cvTargetEdit);
    layout->addLayout(buttonRow);
    layout->addWidget(new QLabel(tr("CVs de esta sesión"), page));
    layout->addWidget(m_cvList, 1);
    layout->addWidget(new QLabel(tr("Resultado del análisis IA"), page));
    layout->addWidget(m_cvAnalysisText, 2);
    return page;
}

QWidget *UserWindow::createJobsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *buttonRow = new QHBoxLayout;
    auto *reload = new QPushButton(tr("Actualizar puestos"), page);
    auto *apply = new QPushButton(tr("Postularme al puesto seleccionado"), page);
    connect(reload, &QPushButton::clicked, this, &UserWindow::loadJobs);
    connect(apply, &QPushButton::clicked, this, &UserWindow::applyToSelectedJob);
    buttonRow->addWidget(reload);
    buttonRow->addWidget(apply);

    m_jobsList = new QListWidget(page);
    m_jobsList->setSpacing(12);
    m_jobsList->setStyleSheet(
        "QListWidget::item { background: transparent; border: none; margin:6px; padding:0; }"
        "QListWidget::item:selected { background: transparent; color:#FFFFFF; }"
    );

    m_jobDetailLabel = new QTextEdit(page);
    m_jobDetailLabel->setReadOnly(true);
    m_jobDetailLabel->setMinimumHeight(180);
    m_jobDetailLabel->setPlaceholderText(tr("Seleccioná un puesto para ver el detalle."));

    connect(m_jobsList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (!current) {
            m_jobDetailLabel->setPlainText(tr("Seleccioná un puesto para ver el detalle."));
            return;
        }
        m_jobDetailLabel->setPlainText(current->data(Qt::UserRole + 1).toString());
    });

    layout->addLayout(buttonRow);
    layout->addWidget(m_jobsList, 1);
    layout->addWidget(m_jobDetailLabel);
    return page;
}

QWidget *UserWindow::createApplicationsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *reload = new QPushButton(tr("Actualizar mis postulaciones"), page);
    connect(reload, &QPushButton::clicked, this, &UserWindow::loadApplications);

    m_applicationsList = new QListWidget(page);
    m_applicationsList->setSpacing(12);
    m_applicationsList->setStyleSheet(
        "QListWidget::item { background: transparent; border: none; margin:6px; padding:0; }"
        "QListWidget::item:selected { background: transparent; color:#FFFFFF; }"
    );

    m_applicationDetailLabel = new QTextEdit(page);
    m_applicationDetailLabel->setReadOnly(true);
    m_applicationDetailLabel->setMinimumHeight(180);
    m_applicationDetailLabel->setPlaceholderText(tr("Seleccioná una postulación."));

    connect(m_applicationsList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (!current) {
            m_applicationDetailLabel->setPlainText(tr("Seleccioná una postulación."));
            return;
        }
        m_applicationDetailLabel->setPlainText(current->data(Qt::UserRole + 1).toString());
    });

    layout->addWidget(reload);
    layout->addWidget(m_applicationsList, 1);
    layout->addWidget(m_applicationDetailLabel);
    return page;
}

QWidget *UserWindow::createNotificationsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *row = new QHBoxLayout;
    auto *reload = new QPushButton(tr("Actualizar notificaciones"), page);
    auto *markRead = new QPushButton(tr("Marcar seleccionada como leída"), page);
    connect(reload, &QPushButton::clicked, this, &UserWindow::loadNotifications);
    connect(markRead, &QPushButton::clicked, this, &UserWindow::markSelectedNotificationRead);
    row->addWidget(reload);
    row->addWidget(markRead);

    m_notificationsList = new QListWidget(page);
    layout->addLayout(row);
    layout->addWidget(m_notificationsList, 1);
    return page;
}

QWidget *UserWindow::createChatTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_chatHistory = new QTextEdit(page);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setHtml(
        "<div style='font-family:Segoe UI, sans-serif; color:#e8dcff; background:transparent; padding:12px;'>"
        "<div style='margin-bottom:16px; padding:18px 22px; background:#16102b; border:2px solid #5f4cd1; border-radius:28px; box-shadow:0 12px 32px rgba(80,71,232,0.18);'>"
        "<div style='font-size:10pt; font-weight:700; color:#DEC9FF; margin-bottom:10px;'>IA</div>"
        "Chat IA conectado. Podés pedir ayuda para mejorar tu CV, preparar entrevistas o entender puestos compatibles."
        "</div></div>"
    );
    m_chatInput = new QLineEdit(page);
    m_chatInput->setPlaceholderText(tr("Escribí un mensaje..."));

    m_chatPdfLabel = new QLabel(tr("PDF adjunto: ninguno"), page);
    m_chatPdfLabel->setWordWrap(true);
    m_chatPdfLabel->setStyleSheet("color:#B8B0F2;");

    auto *attach = new QPushButton(tr("Adjuntar PDF"), page);
    auto *send = new QPushButton(tr("Enviar"), page);

    connect(attach, &QPushButton::clicked, this, &UserWindow::chooseChatPdf);
    connect(send, &QPushButton::clicked, this, &UserWindow::sendChatMessage);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &UserWindow::sendChatMessage);

    auto *row = new QHBoxLayout;
    row->addWidget(m_chatInput, 1);
    row->addWidget(attach);
    row->addWidget(send);
    layout->addWidget(m_chatHistory, 1);
    layout->addWidget(m_chatPdfLabel);
    layout->addLayout(row);
    return page;
}

void UserWindow::loadProfile()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/users/profile", &ok, &error);
    if (!ok || !doc.isObject()) {
        setStatus(tr("Perfil: %1").arg(error), false);
        return;
    }

    const QJsonObject o = doc.object();
    m_profileLabel->setText(tr("ID: %1\nUsuario: %2\nNombre visible: %3\nRol: %4\nEmail: %5\nActivo: %6\nÚltimo login: %7")
                            .arg(jtext(o, "id"),
                                 jtext(o, "username"),
                                 jtext(o, "display_name"),
                                 jtext(o, "role"),
                                 jtext(o, "email"),
                                 jtext(o, "is_active"),
                                 jtext(o, "last_login")));
    setStatus(tr("Perfil actualizado desde la API."));
}

void UserWindow::loadJobs()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/jobs/list", &ok, &error);
    if (!ok || !doc.isArray()) {
        bool foundCache = false;
        const QJsonDocument cached = AdminDB::instance().getCachedData("job_posts", "all", ApiClient::instance().currentUser().value("id").toInt(), &foundCache);
        if (foundCache && cached.isArray()) {
            m_jobsList->clear();
            for (const QJsonValue &value : cached.array()) {
                const QJsonObject o = value.toObject();
                const QString title = jtext(o, "title");
                const QString subtitle = tr("%1 | %2 | %3").arg(jtext(o, "location_mode"), jtext(o, "country"), jtext(o, "status"));
                const QString body = tr("Descripción: %1\nSkills: %2\nExperiencia mínima: %3 años")
                    .arg(jtext(o, "description"), jtext(o, "required_skills"), jtext(o, "min_years_experience"));
                auto *item = new QListWidgetItem(m_jobsList);
                item->setSizeHint(QSize(0, 120));
                item->setData(Qt::UserRole, o.value("id").toInt());
                item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)));
                m_jobsList->addItem(item);
                m_jobsList->setItemWidget(item, createCardWidget(title, subtitle, body));
            }
            setStatus(tr("Sin conexión/API con error. Mostrando puestos desde SQLite local."));
            return;
        }

        setStatus(tr("Puestos: %1").arg(error), false);
        return;
    }

    AdminDB::instance().cacheData("job_posts", "all", doc, ApiClient::instance().currentUser().value("id").toInt());

    m_jobsList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        const QString title = jtext(o, "title");
        const QString subtitle = tr("%1 | %2 | %3").arg(jtext(o, "location_mode"), jtext(o, "country"), jtext(o, "status"));
        const QString body = tr("Descripción: %1\nSkills: %2\nExperiencia mínima: %3 años")
            .arg(jtext(o, "description"), jtext(o, "required_skills"), jtext(o, "min_years_experience"));
        auto *item = new QListWidgetItem(m_jobsList);
        item->setSizeHint(QSize(0, 120));
        item->setData(Qt::UserRole, o.value("id").toInt());
        item->setData(Qt::UserRole + 1,
                      tr("Puesto: %1\nDescripción: %2\nSkills: %3\nExperiencia mínima: %4\nModalidad: %5\nCiudad/País: %6 / %7\nEstado: %8")
                          .arg(title,
                               jtext(o, "description"),
                               jtext(o, "required_skills"),
                               jtext(o, "min_years_experience"),
                               jtext(o, "location_mode"),
                               jtext(o, "city"),
                               jtext(o, "country"),
                               jtext(o, "status")));
        m_jobsList->addItem(item);
        m_jobsList->setItemWidget(item, createCardWidget(title, subtitle, body));
    }
    setStatus(tr("Puestos actualizados desde la API y cacheados localmente."));
}

void UserWindow::applyToSelectedJob()
{
    auto *item = m_jobsList->currentItem();
    if (!item) {
        setStatus(tr("Seleccioná un puesto."), false);
        return;
    }

    QJsonObject payload;
    payload.insert("job_post_id", item->data(Qt::UserRole).toInt());

    bool ok = false;
    QString error;
    ApiClient::instance().post("/applications/apply", payload, &ok, &error);
    if (!ok) {
        setStatus(tr("No se pudo postular: %1").arg(error), false);
        return;
    }
    AdminDB::instance().logAction(ApiClient::instance().username(), "apply_job", tr("Postulación al job_post_id=%1").arg(item->data(Qt::UserRole).toInt()));
    setStatus(tr("Postulación enviada correctamente."));
    loadApplications();
}

void UserWindow::uploadCv()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Seleccionar CV"), QString(), tr("CV (*.pdf *.docx)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString error;
    QJsonObject response;
    if (!ApiClient::instance().uploadCv(filePath, &error, &response)) {
        setStatus(tr("No se pudo subir el CV: %1").arg(error), false);
        return;
    }

    const QString fileName = response.value("file_name").toString(QFileInfo(filePath).fileName());
    m_cvList->addItem(tr("%1 | SHA256: %2").arg(fileName, response.value("sha256").toString("-")));
    AdminDB::instance().logAction(ApiClient::instance().username(), "upload_cv", fileName);
    setStatus(tr("CV subido correctamente."));
}

QString UserWindow::formatCvAnalysis(const QJsonObject &response) const
{
    const QJsonObject analysis = response.value("analysis").toObject(response);

    QString text;
    text += tr("CV document ID: %1\n").arg(jtext(response, "cv_document_id"));
    text += tr("Archivo: %1\n").arg(jtext(response, "file_name"));
    text += tr("Puesto objetivo: %1\n\n").arg(jtext(response, "puesto"));

    text += tr("Resumen:\n%1\n\n").arg(jtext(analysis, "resumen", jtext(analysis, "summary")));
    text += tr("Nombre detectado: %1\n").arg(jtext(analysis, "nombre"));
    text += tr("Email detectado: %1\n").arg(jtext(analysis, "email"));
    text += tr("Teléfono detectado: %1\n").arg(jtext(analysis, "telefono"));
    text += tr("Seniority: %1\n").arg(jtext(analysis, "seniority"));
    text += tr("Años de experiencia: %1\n").arg(jtext(analysis, "años_experiencia"));
    text += tr("Compatibilidad: %1%%\n").arg(jtext(analysis, "compatibilidad", jtext(analysis, "overall_score")));
    text += tr("Explicación compatibilidad:\n%1\n\n").arg(jtext(analysis, "explicacion_compatibilidad"));

    text += tr("Tecnologías:\n%1\n\n").arg(jtext(analysis, "tecnologias", jtext(analysis, "detected_skills")));
    text += tr("Idiomas:\n%1\n\n").arg(jtext(analysis, "idiomas"));
    text += tr("Fortalezas:\n%1\n\n").arg(jtext(analysis, "fortalezas", jtext(analysis, "strengths")));
    text += tr("Debilidades:\n%1\n\n").arg(jtext(analysis, "debilidades", jtext(analysis, "weak_points")));

    const QString preview = response.value("preview_texto").toString();
    if (!preview.isEmpty()) {
        text += tr("Preview texto extraído:\n%1\n").arg(preview.left(1200));
    }

    return text;
}

void UserWindow::analyzeCv()
{
    const QString puesto = m_cvTargetEdit->text().trimmed();
    if (puesto.isEmpty()) {
        setStatus(tr("Indicá un puesto objetivo para que la IA calcule compatibilidad."), false);
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(this, tr("Seleccionar CV para analizar"), QString(), tr("CV/Imagen (*.pdf *.png *.jpg *.jpeg)"));
    if (filePath.isEmpty()) {
        return;
    }

    setStatus(tr("Subiendo y analizando CV con IA. Puede tardar unos segundos..."));

    QString error;
    QJsonObject response;
    if (!ApiClient::instance().analyzeCv(filePath, puesto, &error, &response)) {
        setStatus(tr("No se pudo analizar el CV: %1").arg(error), false);
        return;
    }

    const QString fileName = response.value("file_name").toString(QFileInfo(filePath).fileName());
    m_cvList->addItem(tr("%1 | Analizado con IA | Compatibilidad: %2%%")
                          .arg(fileName, response.value("analysis").toObject().value("compatibilidad").toVariant().toString()));

    m_cvAnalysisText->setPlainText(formatCvAnalysis(response));
    AdminDB::instance().cacheData("cv_analysis", QString::number(response.value("cv_document_id").toInt()), QJsonDocument(response), ApiClient::instance().currentUser().value("id").toInt(), 24);
    AdminDB::instance().logAction(ApiClient::instance().username(), "analyze_cv", tr("CV=%1 | Puesto=%2").arg(fileName, puesto));
    setStatus(tr("CV analizado con IA y guardado en VPS + caché local."));
}

void UserWindow::loadApplications()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/applications/my", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Postulaciones: %1").arg(error), false);
        return;
    }

    AdminDB::instance().cacheData("applications", "my", doc, ApiClient::instance().currentUser().value("id").toInt());

    m_applicationsList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        const QString title = tr("Postulación #%1").arg(jtext(o, "id"));
        const QString subtitle = tr("Puesto ID: %1 | Estado: %2").arg(jtext(o, "job_post_id"), jtext(o, "status"));
        const QString body = jtext(o, "message", tr("No hay descripción adicional."));
        auto *item = new QListWidgetItem(m_applicationsList);
        item->setSizeHint(QSize(0, 100));
        item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)));
        m_applicationsList->addItem(item);
        m_applicationsList->setItemWidget(item, createCardWidget(title, subtitle, body));
    }
    setStatus(tr("Postulaciones actualizadas desde la API y cacheadas localmente."));
}

void UserWindow::loadNotifications()
{
    refreshNotifications(false);
    setStatus(tr("Notificaciones actualizadas desde la API y cacheadas localmente."));
}

void UserWindow::pollNotifications()
{
    refreshNotifications(true);
}

QString UserWindow::notificationText(const QJsonObject &notification) const
{
    const QString prefix = notification.value("is_read").toBool() ? tr("[Leída]") : tr("[Nueva]");
    const QString title = jtext(notification, "title");
    const QString body = jtext(notification, "body", jtext(notification, "message"));
    const QString createdAt = jtext(notification, "created_at", QString());

    if (createdAt.isEmpty()) {
        return tr("%1 %2\n%3").arg(prefix, title, body);
    }

    return tr("%1 %2\n%3\n%4").arg(prefix, title, body, createdAt);
}

void UserWindow::refreshNotifications(bool showPopupForNew)
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/notifications/my", &ok, &error);

    if (!ok || !doc.isArray()) {
        if (!showPopupForNew) {
            setStatus(tr("Notificaciones: %1").arg(error), false);
        }
        return;
    }

    AdminDB::instance().cacheData(
        "notifications",
        "my",
        doc,
        ApiClient::instance().currentUser().value("id").toInt()
    );

    const QJsonArray arr = doc.array();
    QJsonArray newUnread;
    int unreadCount = 0;

    if (m_notificationsList) {
        m_notificationsList->clear();
    }
    if (m_notificationPopupList) {
        m_notificationPopupList->clear();
    }

    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        const qint64 id = o.value("id").toVariant().toLongLong();
        const bool isRead = o.value("is_read").toBool();
        const QString text = notificationText(o);

        if (m_notificationsList) {
            auto *item = new QListWidgetItem(text);
            item->setData(Qt::UserRole, id);
            m_notificationsList->addItem(item);
        }

        if (m_notificationPopupList) {
            auto *popupItem = new QListWidgetItem(text);
            popupItem->setData(Qt::UserRole, id);
            m_notificationPopupList->addItem(popupItem);
        }

        if (!isRead) {
            unreadCount++;
            if (!m_seenNotificationIds.contains(id)) {
                newUnread.append(o);
                AdminDB::instance().logAction(
                    ApiClient::instance().username(),
                    "notification_received",
                    QString("ID=%1 | %2").arg(id).arg(jtext(o, "title"))
                );
            }
        }

        if (id > 0) {
            m_seenNotificationIds.insert(id);
        }
    }

    updateNotificationBell(unreadCount);

    if (showPopupForNew && !newUnread.isEmpty()) {
        showNotificationPopup(true);
    }
}

void UserWindow::updateNotificationBell(int unreadCount)
{
    m_unreadNotifications = unreadCount;

    if (!m_bellButton) {
        return;
    }

    if (unreadCount > 0) {
        m_bellButton->setText(tr("🔔 (%1)").arg(unreadCount));
        m_bellButton->setStyleSheet("font-weight:bold; color:#FFD166;");
    } else {
        m_bellButton->setText(tr("🔔"));
        m_bellButton->setStyleSheet(QString());
    }
}

void UserWindow::showNotificationPopup(bool autoHide)
{
    if (!m_notificationPopup) {
        return;
    }

    m_notificationPopup->setVisible(true);

    if (autoHide && m_notificationHideTimer) {
        m_notificationHideTimer->start(6500);
    }
}

void UserWindow::toggleNotificationPopup()
{
    if (!m_notificationPopup) {
        return;
    }

    if (m_notificationPopup->isVisible()) {
        hideNotificationPopup();
        return;
    }

    refreshNotifications(false);
    showNotificationPopup(false);
}

void UserWindow::hideNotificationPopup()
{
    if (m_notificationPopup) {
        m_notificationPopup->setVisible(false);
    }
}

void UserWindow::markSelectedNotificationRead()
{
    QListWidgetItem *item = nullptr;

    if (m_notificationPopup && m_notificationPopup->isVisible() && m_notificationPopupList) {
        item = m_notificationPopupList->currentItem();
    }

    if (!item && m_notificationsList) {
        item = m_notificationsList->currentItem();
    }
    if (!item) {
        setStatus(tr("Seleccioná una notificación."), false);
        return;
    }

    const int id = item->data(Qt::UserRole).toInt();
    bool ok = false;
    QString error;
    ApiClient::instance().put(QString("/notifications/read/%1").arg(id), QJsonObject(), &ok, &error);
    if (!ok) {
        setStatus(tr("No se pudo marcar como leída: %1").arg(error), false);
        return;
    }
    AdminDB::instance().logAction(ApiClient::instance().username(), "notification_read", QString::number(id));
    refreshNotifications(false);
}

void UserWindow::handleLogout()
{
    m_explicitLogout = true;

    QString error;

    AdminDB::instance().logAction(
        ApiClient::instance().username(),
        QStringLiteral("logout"),
        QStringLiteral("Cierre de sesión desde panel usuario"),
        &error
    );

    AdminDB::instance().closeActiveSession(&error);
    ApiClient::instance().logout();

    auto *login = new MainWindow();
    login->show();

    close();
}

void UserWindow::closeEvent(QCloseEvent *event)
{
    if (!m_explicitLogout && ApiClient::instance().isLoggedIn()) {
        AdminDB::instance().updateLastActivity(nullptr);
    }

    QMainWindow::closeEvent(event);
}


void UserWindow::chooseChatPdf()
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

void UserWindow::sendChatMessage()
{
    const QString text = m_chatInput->text().trimmed();

    if (text.isEmpty()) {
        return;
    }

    m_chatHistory->append(formatChatHtml(text, true));

    if (!m_chatPdfPath.isEmpty()) {
        m_chatHistory->append(formatChatHtml(tr("PDF adjunto: %1").arg(QFileInfo(m_chatPdfPath).fileName()), true));
    }

    m_chatInput->clear();

    QJsonObject context;
    context.insert("role", ApiClient::instance().role());
    context.insert("username", ApiClient::instance().username());
    context.insert("display_name", ApiClient::instance().displayName());
    context.insert("use_uploaded_cvs", true);

    QJsonObject response;
    QString error;

    const bool ok = ApiClient::instance().sendChatMessage(
        text,
        QStringLiteral("CV_FEEDBACK"),
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

    const QString responseText = fallback ? tr("(fallback) %1").arg(reply) : reply;
    m_chatHistory->append(formatChatHtml(responseText, false));

    if (!modelName.isEmpty()) {
        setStatus(tr("Chat IA respondió usando %1.").arg(modelName));
    } else {
        setStatus(tr("Chat IA respondió correctamente."));
    }

    m_chatPdfPath.clear();
    if (m_chatPdfLabel) {
        m_chatPdfLabel->setText(tr("PDF adjunto: ninguno"));
    }

    AdminDB::instance().logAction(
        ApiClient::instance().username(),
        QStringLiteral("chat_message"),
        QStringLiteral("Mensaje enviado al chat IA desde panel Usuario")
    );
}
