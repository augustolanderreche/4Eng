#include "empresawindow.h"

#include "api_client.h"
#include "localdbmanager.h"
#include "mainwindow.h"

#include <QComboBox>
#include <QTimer>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
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
#include <QFileDialog>
#include <QFileInfo>

static QString valueToText(const QJsonValue &value, const QString &fallback = "-")
{
    if (value.isString()) {
        const QString text = value.toString();
        return text.isEmpty() ? fallback : text;
    }

    if (value.isDouble()) {
        const double number = value.toDouble();
        const qint64 integer = static_cast<qint64>(number);
        if (qFuzzyCompare(number + 1.0, static_cast<double>(integer) + 1.0)) {
            return QString::number(integer);
        }
        return QString::number(number, 'f', 2);
    }

    if (value.isBool()) {
        return value.toBool() ? "Sí" : "No";
    }

    if (value.isArray()) {
        QStringList values;
        for (const QJsonValue &item : value.toArray()) {
            values << valueToText(item, QString());
        }
        const QString joined = values.join(", ");
        return joined.isEmpty() ? fallback : joined;
    }

    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }

    return fallback;
}

static QString objText(const QJsonObject &obj, const QString &key, const QString &fallback = "-")
{
    return valueToText(obj.value(key), fallback);
}

static qint64 objId(const QJsonObject &obj, const QString &key)
{
    return obj.value(key).toVariant().toLongLong();
}

static QJsonObject itemJson(QListWidgetItem *item)
{
    if (!item) {
        return {};
    }

    const QByteArray raw = item->data(Qt::UserRole + 1).toString().toUtf8();
    const QJsonDocument doc = QJsonDocument::fromJson(raw);

    if (!doc.isObject()) {
        return {};
    }

    return doc.object();
}

EmpresaWindow::EmpresaWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
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

void EmpresaWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *headerRow = new QHBoxLayout;

    m_welcomeLabel = new QLabel(tr("Panel Empresa - %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #FFFFFF;");

    m_bellButton = new QPushButton(tr("🔔"), central);
    m_bellButton->setToolTip(tr("Ver notificaciones"));
    m_bellButton->setMinimumWidth(72);
    connect(m_bellButton, &QPushButton::clicked, this, &EmpresaWindow::toggleNotificationPopup);

    auto *logoutButton = new QPushButton(tr("Cerrar sesión"), central);
    connect(logoutButton, &QPushButton::clicked, this, &EmpresaWindow::handleLogout);

    headerRow->addWidget(m_welcomeLabel, 1);
    headerRow->addWidget(m_bellButton);
    headerRow->addWidget(logoutButton);

    m_notificationPopup = new QFrame(central);
    m_notificationPopup->setFrameShape(QFrame::StyledPanel);
    m_notificationPopup->setObjectName("notificationPopup");
    m_notificationPopup->setStyleSheet(
        "QFrame#notificationPopup { background:#1f1f1f; border:1px solid #4a4a4a; border-radius:8px; }"
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

    connect(popupRefresh, &QPushButton::clicked, this, [this]() { refreshNotifications(false); });
    connect(popupMarkRead, &QPushButton::clicked, this, &EmpresaWindow::markSelectedNotificationRead);
    connect(popupClose, &QPushButton::clicked, this, &EmpresaWindow::hideNotificationPopup);

    popupButtons->addWidget(popupRefresh);
    popupButtons->addWidget(popupMarkRead);
    popupButtons->addStretch();
    popupButtons->addWidget(popupClose);

    popupLayout->addWidget(popupTitle);
    popupLayout->addWidget(m_notificationPopupList);
    popupLayout->addLayout(popupButtons);

    m_notificationPollTimer = new QTimer(this);
    connect(m_notificationPollTimer, &QTimer::timeout, this, &EmpresaWindow::pollNotifications);

    m_notificationHideTimer = new QTimer(this);
    m_notificationHideTimer->setSingleShot(true);
    connect(m_notificationHideTimer, &QTimer::timeout, this, &EmpresaWindow::hideNotificationPopup);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    auto *contentRow = new QHBoxLayout;

    m_menuList = new QListWidget(central);
    m_menuList->addItems({
        tr("Perfil"),
        tr("Mis publicaciones"),
        tr("Postulantes"),
        tr("Nueva publicación"),
        tr("Chat IA")
    });
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
    layout->addWidget(m_notificationPopup);
    layout->addLayout(contentRow, 1);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Panel empresa"));
    resize(1220, 780);
}

void EmpresaWindow::setStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
    LocalDbManager::instance().updateLastActivity();
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

    auto *buttonRow = new QHBoxLayout;

    auto *reload = new QPushButton(tr("Actualizar mis publicaciones"), page);
    connect(reload, &QPushButton::clicked, this, &EmpresaWindow::loadJobs);

    auto *viewApplicants = new QPushButton(tr("Ver postulantes de esta publicación"), page);
    connect(viewApplicants, &QPushButton::clicked, this, &EmpresaWindow::loadApplicationsForSelectedJob);

    auto *ranking = new QPushButton(tr("Ranking IA de esta publicación"), page);
    connect(ranking, &QPushButton::clicked, this, &EmpresaWindow::rankCandidatesForSelectedJob);

    buttonRow->addWidget(reload);
    buttonRow->addWidget(viewApplicants);
    buttonRow->addWidget(ranking);

    m_publicacionesList = new QListWidget(page);
    m_publicacionDetailLabel = new QLabel(tr("Seleccioná una publicación para ver detalle."), page);
    m_publicacionDetailLabel->setWordWrap(true);

    connect(m_publicacionesList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (!current) {
            m_publicacionDetailLabel->setText(tr("Seleccioná una publicación para ver detalle."));
            return;
        }

        const QJsonObject job = itemJson(current);
        m_publicacionDetailLabel->setText(formatJobDetail(job));
    });

    layout->addLayout(buttonRow);
    layout->addWidget(m_publicacionesList, 1);
    layout->addWidget(m_publicacionDetailLabel);

    return page;
}

QWidget *EmpresaWindow::createPostulacionesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *buttonRow = new QHBoxLayout;

    auto *reload = new QPushButton(tr("Actualizar postulantes"), page);
    connect(reload, &QPushButton::clicked, this, &EmpresaWindow::loadApplications);

    auto *ranking = new QPushButton(tr("Ranking IA"), page);
    connect(ranking, &QPushButton::clicked, this, &EmpresaWindow::rankCandidatesForSelectedJob);

    auto *accept = new QPushButton(tr("Aceptar"), page);
    connect(accept, &QPushButton::clicked, this, &EmpresaWindow::acceptSelectedApplication);

    auto *reject = new QPushButton(tr("Rechazar"), page);
    connect(reject, &QPushButton::clicked, this, &EmpresaWindow::rejectSelectedApplication);

    auto *requestCv = new QPushButton(tr("Pedir CV actualizado"), page);
    connect(requestCv, &QPushButton::clicked, this, &EmpresaWindow::requestCvUpdate);

    auto *meeting = new QPushButton(tr("Pedir reunión"), page);
    connect(meeting, &QPushButton::clicked, this, &EmpresaWindow::requestMeeting);

    auto *message = new QPushButton(tr("Enviar mensaje"), page);
    connect(message, &QPushButton::clicked, this, &EmpresaWindow::sendCustomMessage);

    buttonRow->addWidget(reload);
    buttonRow->addWidget(ranking);
    buttonRow->addWidget(accept);
    buttonRow->addWidget(reject);
    buttonRow->addWidget(requestCv);
    buttonRow->addWidget(meeting);
    buttonRow->addWidget(message);

    m_postulacionesList = new QListWidget(page);
    m_postulacionDetailLabel = new QLabel(tr("Seleccioná una postulación."), page);
    m_postulacionDetailLabel->setWordWrap(true);

    m_rankingResultText = new QTextEdit(page);
    m_rankingResultText->setReadOnly(true);
    m_rankingResultText->setPlaceholderText(tr("Acá se mostrará el ranking IA de candidatos por publicación."));

    connect(m_postulacionesList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (!current) {
            m_postulacionDetailLabel->setText(tr("Seleccioná una postulación."));
            return;
        }

        const QJsonObject application = itemJson(current);
        m_postulacionDetailLabel->setText(formatApplicationDetail(application));
    });

    layout->addLayout(buttonRow);
    layout->addWidget(m_postulacionesList, 2);
    layout->addWidget(m_postulacionDetailLabel);
    layout->addWidget(new QLabel(tr("Resultado Ranking IA"), page));
    layout->addWidget(m_rankingResultText, 1);

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
    m_chatHistory->setPlainText(tr("Chat IA conectado. Podés pedir ayuda para evaluar candidatos, preparar entrevistas o redactar mensajes."));

    m_chatInput = new QLineEdit(page);
    m_chatInput->setPlaceholderText(tr("Mensaje..."));

    m_chatPdfLabel = new QLabel(tr("PDF adjunto: ninguno"), page);
    m_chatPdfLabel->setWordWrap(true);
    m_chatPdfLabel->setStyleSheet("color:#A7B3C6;");

    auto *attach = new QPushButton(tr("Adjuntar PDF"), page);
    auto *send = new QPushButton(tr("Enviar"), page);

    connect(attach, &QPushButton::clicked, this, &EmpresaWindow::chooseChatPdf);
    connect(send, &QPushButton::clicked, this, &EmpresaWindow::sendChatMessage);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &EmpresaWindow::sendChatMessage);

    auto *row = new QHBoxLayout;
    row->addWidget(m_chatInput, 1);
    row->addWidget(attach);
    row->addWidget(send);

    layout->addWidget(m_chatHistory, 1);
    layout->addWidget(m_chatPdfLabel);
    layout->addLayout(row);

    return page;
}

qint64 EmpresaWindow::selectedJobId() const
{
    return m_publicacionesList && m_publicacionesList->currentItem()
               ? m_publicacionesList->currentItem()->data(Qt::UserRole).toLongLong()
               : 0;
}

qint64 EmpresaWindow::selectedApplicationId() const
{
    return m_postulacionesList && m_postulacionesList->currentItem()
               ? m_postulacionesList->currentItem()->data(Qt::UserRole).toLongLong()
               : 0;
}

QString EmpresaWindow::formatJobDetail(const QJsonObject &job) const
{
    return tr(
               "ID: %1\n"
               "Título: %2\n"
               "Estado: %3\n"
               "Modalidad: %4\n"
               "Ciudad/País: %5 / %6\n"
               "Senioridad: %7\n"
               "Experiencia mínima: %8 años\n"
               "Postulaciones: %9\n"
               "Skills: %10\n\n"
               "Descripción:\n%11"
               )
        .arg(objText(job, "id"),
             objText(job, "title"),
             objText(job, "status"),
             objText(job, "location_mode"),
             objText(job, "city"),
             objText(job, "country"),
             objText(job, "seniority"),
             objText(job, "min_years_experience"),
             objText(job, "applications_count"),
             objText(job, "required_skills"),
             objText(job, "description"));
}

QString EmpresaWindow::formatApplicationDetail(const QJsonObject &application) const
{
    return tr(
               "Postulación ID: %1\n"
               "Puesto: %2\n"
               "Estado: %3\n"
               "Fecha postulación: %4\n\n"
               "Candidato:\n"
               "Nombre: %5\n"
               "Usuario ID: %6\n"
               "Email: %7\n"
               "Teléfono: %8\n"
               "País: %9\n"
               "Lenguaje principal: %10\n\n"
               "CV:\n"
               "CV ID: %11\n"
               "Archivo: %12\n"
               "Estado CV: %13\n"
               "Analizado: %14\n\n"
               "Análisis IA del CV:\n"
               "Score CV: %15\n"
               "Resumen: %16\n"
               "Fortalezas: %17\n"
               "Debilidades/Gaps: %18\n"
               "Skills detectadas: %19\n\n"
               "Último ranking empresa:\n"
               "Score ranking: %20\n"
               "Recomendación: %21\n"
               "Resumen empresa: %22"
               )
        .arg(objText(application, "application_id"),
             objText(application, "job_title"),
             objText(application, "status"),
             objText(application, "applied_at"),
             objText(application, "candidate_name"),
             objText(application, "engineer_user_id"),
             objText(application, "candidate_email"),
             objText(application, "candidate_phone"),
             objText(application, "candidate_country"),
             objText(application, "main_programming_language"),
             objText(application, "cv_document_id"),
             objText(application, "cv_file_name"),
             objText(application, "cv_status"),
             objText(application, "cv_analyzed_at"),
             objText(application, "cv_overall_score"),
             objText(application, "cv_summary"),
             objText(application, "cv_strengths"),
             objText(application, "cv_weak_points"),
             objText(application, "cv_detected_skills"),
             objText(application, "latest_rank_score"),
             objText(application, "latest_recommendation"),
             objText(application, "latest_company_summary"));
}

QString EmpresaWindow::formatRankingResult(const QJsonObject &response) const
{
    QString text;
    text += tr("Mensaje: %1\n").arg(objText(response, "message"));
    text += tr("Modelo: %1\n").arg(objText(response, "model_name"));
    text += tr("Fallback local: %1\n\n").arg(objText(response, "fallback"));

    const QJsonArray ranking = response.value("ranking").toArray();

    if (ranking.isEmpty()) {
        text += tr("No hay candidatos para rankear.");
        return text;
    }

    for (const QJsonValue &value : ranking) {
        const QJsonObject item = value.toObject();

        text += tr(
                    "#%1 - %2\n"
                    "Postulación ID: %3 | Score: %4 | Recomendación: %5\n"
                    "Resumen: %6\n"
                    "Fortalezas: %7\n"
                    "Gaps / faltantes: %8\n"
                    "Capacitación sugerida: %9\n\n"
                    )
                    .arg(objText(item, "position"),
                         objText(item, "candidate_name"),
                         objText(item, "application_id"),
                         objText(item, "score"),
                         objText(item, "recommendation"),
                         objText(item, "summary_for_company"),
                         objText(item, "strengths"),
                         objText(item, "missing_skills"),
                         objText(item, "training_suggestions"));
    }

    return text;
}

void EmpresaWindow::populateApplications(const QJsonArray &applications)
{
    m_postulacionesList->clear();

    for (const QJsonValue &value : applications) {
        const QJsonObject application = value.toObject();

        auto *item = new QListWidgetItem(
            tr("#%1 | %2\n%3 | Estado: %4")
                .arg(objText(application, "application_id"),
                     objText(application, "candidate_name"),
                     objText(application, "job_title"),
                     objText(application, "status"))
        );

        item->setData(Qt::UserRole, objId(application, "application_id"));
        item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(application).toJson(QJsonDocument::Compact)));

        m_postulacionesList->addItem(item);
    }

    if (applications.isEmpty()) {
        m_postulacionDetailLabel->setText(tr("No hay postulantes para mostrar."));
    } else {
        m_postulacionesList->setCurrentRow(0);
    }
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

    m_profileLabel->setText(
        tr("ID: %1\nUsuario: %2\nNombre visible: %3\nRol: %4\nEmail: %5\nÚltimo login: %6")
            .arg(objText(o, "id"),
                 objText(o, "username"),
                 objText(o, "display_name"),
                 objText(o, "role"),
                 objText(o, "email"),
                 objText(o, "last_login"))
    );

    setStatus(tr("Perfil de empresa actualizado desde la API."));
}

void EmpresaWindow::loadJobs()
{
    bool ok = false;
    QString error;

    const QJsonDocument doc = ApiClient::instance().get("/company/jobs/my", &ok, &error);

    if (!ok || !doc.isArray()) {
        setStatus(tr("Mis publicaciones: %1").arg(error), false);
        return;
    }

    m_publicacionesList->clear();

    const QJsonArray arr = doc.array();

    for (const QJsonValue &value : arr) {
        const QJsonObject job = value.toObject();

        auto *item = new QListWidgetItem(
            tr("#%1 | %2\n%3 | %4 | Postulantes: %5")
                .arg(objText(job, "id"),
                     objText(job, "title"),
                     objText(job, "location_mode"),
                     objText(job, "status"),
                     objText(job, "applications_count"))
        );

        item->setData(Qt::UserRole, objId(job, "id"));
        item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(job).toJson(QJsonDocument::Compact)));

        m_publicacionesList->addItem(item);
    }

    if (!arr.isEmpty()) {
        m_publicacionesList->setCurrentRow(0);
    }

    setStatus(tr("Mis publicaciones actualizadas desde la API."));
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

    populateApplications(doc.array());
    setStatus(tr("Postulaciones de la empresa actualizadas desde la API."));
}

void EmpresaWindow::loadApplicationsForSelectedJob()
{
    const qint64 jobId = selectedJobId();

    if (jobId <= 0) {
        setStatus(tr("Seleccioná primero una publicación."), false);
        return;
    }

    bool ok = false;
    QString error;

    const QJsonDocument doc = ApiClient::instance().get(
        QString("/company/jobs/%1/applications").arg(jobId),
        &ok,
        &error
    );

    if (!ok || !doc.isObject()) {
        setStatus(tr("No se pudieron cargar postulantes del puesto: %1").arg(error), false);
        return;
    }

    const QJsonObject response = doc.object();
    populateApplications(response.value("applications").toArray());

    m_menuList->setCurrentRow(2);
    setStatus(tr("Postulantes cargados para la publicación #%1.").arg(jobId));
}

void EmpresaWindow::rankCandidatesForSelectedJob()
{
    qint64 jobId = selectedJobId();

    if (jobId <= 0) {
        const QJsonObject application = itemJson(m_postulacionesList ? m_postulacionesList->currentItem() : nullptr);
        jobId = objId(application, "job_post_id");
    }

    if (jobId <= 0) {
        setStatus(tr("Seleccioná una publicación o una postulación asociada."), false);
        return;
    }

    bool ok = false;
    QString error;

    const QJsonDocument doc = ApiClient::instance().post(
        QString("/company/jobs/%1/rank-candidates").arg(jobId),
        QJsonObject(),
        &ok,
        &error
    );

    if (!ok || !doc.isObject()) {
        setStatus(tr("No se pudo generar ranking IA: %1").arg(error), false);
        return;
    }

    const QJsonObject response = doc.object();

    if (m_rankingResultText) {
        m_rankingResultText->setPlainText(formatRankingResult(response));
    }

    m_menuList->setCurrentRow(2);
    loadApplicationsForSelectedJob();
    setStatus(tr("Ranking IA generado para publicación #%1.").arg(jobId));
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

void EmpresaWindow::updateSelectedApplicationStatus(const QString &status, const QString &defaultMessage)
{
    const qint64 applicationId = selectedApplicationId();

    if (applicationId <= 0) {
        setStatus(tr("Seleccioná una postulación."), false);
        return;
    }

    bool accepted = false;
    const QString message = QInputDialog::getMultiLineText(
        this,
        tr("Mensaje para el candidato"),
        tr("Mensaje / nota:"),
        defaultMessage,
        &accepted
    );

    if (!accepted) {
        return;
    }

    QJsonObject payload;
    payload.insert("status", status);
    payload.insert("message", message.trimmed());

    bool ok = false;
    QString error;

    const QJsonDocument doc = ApiClient::instance().put(
        QString("/company/applications/%1/status").arg(applicationId),
        payload,
        &ok,
        &error
    );

    Q_UNUSED(doc);

    if (!ok) {
        setStatus(tr("No se pudo actualizar la postulación: %1").arg(error), false);
        return;
    }

    setStatus(tr("Postulación actualizada y notificación enviada al usuario."));
    loadApplications();
}

void EmpresaWindow::acceptSelectedApplication()
{
    updateSelectedApplicationStatus(
        "Seleccionada",
        tr("Felicitaciones, tu postulación fue aceptada. La empresa se pondrá en contacto para continuar el proceso.")
    );
}

void EmpresaWindow::rejectSelectedApplication()
{
    updateSelectedApplicationStatus(
        "Rechazada",
        tr("Gracias por postularte. En esta oportunidad avanzaremos con otro perfil.")
    );
}

void EmpresaWindow::sendCandidateRequest(const QString &requestType,
                                         const QString &defaultTitle,
                                         const QString &defaultDetails,
                                         const QString &meetingAt)
{
    const qint64 applicationId = selectedApplicationId();

    if (applicationId <= 0) {
        setStatus(tr("Seleccioná una postulación."), false);
        return;
    }

    bool accepted = false;

    const QString details = QInputDialog::getMultiLineText(
        this,
        tr("Solicitud para el candidato"),
        tr("Detalle de la solicitud:"),
        defaultDetails,
        &accepted
    );

    if (!accepted) {
        return;
    }

    QJsonObject payload;
    payload.insert("request_type", requestType);
    payload.insert("title", defaultTitle);
    payload.insert("details", details.trimmed());

    if (!meetingAt.isEmpty()) {
        payload.insert("meeting_at", meetingAt);
    }

    bool ok = false;
    QString error;

    const QJsonDocument doc = ApiClient::instance().post(
        QString("/company/applications/%1/request").arg(applicationId),
        payload,
        &ok,
        &error
    );

    Q_UNUSED(doc);

    if (!ok) {
        setStatus(tr("No se pudo enviar la solicitud: %1").arg(error), false);
        return;
    }

    setStatus(tr("Solicitud enviada y notificación creada para el usuario."));
    loadApplications();
}

void EmpresaWindow::requestCvUpdate()
{
    sendCandidateRequest(
        "UPLOAD_CV",
        tr("Actualización de CV solicitada"),
        tr("Por favor, subí una versión actualizada de tu CV para continuar con el proceso.")
    );
}

void EmpresaWindow::requestMeeting()
{
    bool accepted = false;

    const QString meetingAt = QInputDialog::getText(
        this,
        tr("Fecha tentativa de reunión"),
        tr("Fecha/hora sugerida. Formato sugerido: 2026-06-05 15:30:00"),
        QLineEdit::Normal,
        QString(),
        &accepted
    );

    if (!accepted) {
        return;
    }

    sendCandidateRequest(
        "SCHEDULE_MEETING",
        tr("Solicitud de reunión"),
        tr("Queremos coordinar una reunión para avanzar con tu postulación."),
        meetingAt.trimmed()
    );
}

void EmpresaWindow::sendCustomMessage()
{
    sendCandidateRequest(
        "CUSTOM_MESSAGE",
        tr("Mensaje de la empresa"),
        tr("La empresa tiene una actualización sobre tu postulación.")
    );
}


void EmpresaWindow::chooseChatPdf()
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

void EmpresaWindow::sendChatMessage()
{
    const QString text = m_chatInput->text().trimmed();

    if (text.isEmpty()) {
        return;
    }

    m_chatHistory->append(tr("Empresa: %1").arg(text));

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
        QStringLiteral("HIRING_HELP"),
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

    LocalDbManager::instance().logAction(
        ApiClient::instance().username(),
        QStringLiteral("chat_message"),
        QStringLiteral("Mensaje enviado al chat IA desde panel Empresa")
    );
}

void EmpresaWindow::pollNotifications()
{
    refreshNotifications(true);
}

QString EmpresaWindow::notificationText(const QJsonObject &notification) const
{
    const QString prefix = notification.value("is_read").toBool() ? tr("[Leída]") : tr("[Nueva]");
    const QString title = objText(notification, "title");
    const QString body = objText(notification, "body", objText(notification, "message"));
    const QString createdAt = objText(notification, "created_at", QString());

    if (createdAt.isEmpty()) {
        return tr("%1 %2\n%3").arg(prefix, title, body);
    }

    return tr("%1 %2\n%3\n%4").arg(prefix, title, body, createdAt);
}

void EmpresaWindow::refreshNotifications(bool showPopupForNew)
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

    LocalDbManager::instance().cacheData(
        "notifications",
        "my",
        doc,
        ApiClient::instance().currentUser().value("id").toInt()
    );

    const QJsonArray arr = doc.array();
    QJsonArray newUnread;
    int unreadCount = 0;

    if (m_notificationPopupList) {
        m_notificationPopupList->clear();
    }

    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        const qint64 id = o.value("id").toVariant().toLongLong();
        const bool isRead = o.value("is_read").toBool();
        const QString text = notificationText(o);

        if (m_notificationPopupList) {
            auto *popupItem = new QListWidgetItem(text);
            popupItem->setData(Qt::UserRole, id);
            m_notificationPopupList->addItem(popupItem);
        }

        if (!isRead) {
            unreadCount++;
            if (!m_seenNotificationIds.contains(id)) {
                newUnread.append(o);
                LocalDbManager::instance().logAction(
                    ApiClient::instance().username(),
                    "notification_received",
                    QString("ID=%1 | %2").arg(id).arg(objText(o, "title"))
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

void EmpresaWindow::updateNotificationBell(int unreadCount)
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

void EmpresaWindow::showNotificationPopup(bool autoHide)
{
    if (!m_notificationPopup) {
        return;
    }

    m_notificationPopup->setVisible(true);

    if (autoHide && m_notificationHideTimer) {
        m_notificationHideTimer->start(6500);
    }
}

void EmpresaWindow::toggleNotificationPopup()
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

void EmpresaWindow::hideNotificationPopup()
{
    if (m_notificationPopup) {
        m_notificationPopup->setVisible(false);
    }
}

void EmpresaWindow::markSelectedNotificationRead()
{
    if (!m_notificationPopupList || !m_notificationPopupList->currentItem()) {
        setStatus(tr("Seleccioná una notificación."), false);
        return;
    }

    const int id = m_notificationPopupList->currentItem()->data(Qt::UserRole).toInt();

    bool ok = false;
    QString error;

    ApiClient::instance().put(QString("/notifications/read/%1").arg(id), QJsonObject(), &ok, &error);

    if (!ok) {
        setStatus(tr("No se pudo marcar como leída: %1").arg(error), false);
        return;
    }

    LocalDbManager::instance().logAction(
        ApiClient::instance().username(),
        "notification_read",
        QString::number(id)
    );

    refreshNotifications(false);
    setStatus(tr("Notificación marcada como leída."));
}

void EmpresaWindow::handleLogout()
{
    QString error;

    LocalDbManager::instance().logAction(
        ApiClient::instance().username(),
        QStringLiteral("logout"),
        QStringLiteral("Cierre de sesión desde panel empresa"),
        &error
    );

    LocalDbManager::instance().closeActiveSession(&error);
    ApiClient::instance().logout();

    auto *login = new MainWindow();
    login->show();

    close();
}
