#include "userwindow.h"

#include "api_client.h"
#include "localdbmanager.h"
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
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

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

UserWindow::UserWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(displayName);
    loadProfile();
    loadJobs();
    loadApplications();
    loadNotifications();
}

void UserWindow::setupUi(const QString &displayName)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    m_welcomeLabel = new QLabel(tr("Panel Usuario - %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 18pt; font-weight: 700; color: #f4eefe;");

    auto *logoutButton = new QPushButton(tr("Cerrar sesión"), central);
    logoutButton->setObjectName("logoutButton");
    connect(logoutButton, &QPushButton::clicked, this, &UserWindow::handleLogout);

    auto *headerRow = new QHBoxLayout;
    headerRow->addWidget(m_welcomeLabel, 1);
    headerRow->addWidget(logoutButton);

    m_statusLabel = new QLabel(central);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    auto *contentRow = new QHBoxLayout;
    m_menuList = new QListWidget(central);
    m_menuList->addItems({tr("Perfil"), tr("CV + IA"), tr("Puestos"), tr("Postulaciones"), tr("Notificaciones"), tr("Chat IA")});
    m_menuList->setCurrentRow(0);
    m_menuList->setFixedWidth(220);

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
    layout->addLayout(contentRow, 1);
    layout->addWidget(m_statusLabel);

    setWindowTitle(tr("Panel usuario"));
    resize(1120, 760);
}

void UserWindow::setStatus(const QString &message, bool ok)
{
    m_statusLabel->setStyleSheet(ok ? "color:#A8E6CF;" : "color:#F07178;");
    m_statusLabel->setText(message);
    LocalDbManager::instance().updateLastActivity();
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
    m_jobDetailLabel = new QLabel(tr("Seleccioná un puesto para ver el detalle."), page);
    m_jobDetailLabel->setWordWrap(true);

    connect(m_jobsList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (!current) {
            return;
        }
        m_jobDetailLabel->setText(current->data(Qt::UserRole + 1).toString());
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
    m_applicationDetailLabel = new QLabel(tr("Seleccioná una postulación."), page);
    m_applicationDetailLabel->setWordWrap(true);

    connect(m_applicationsList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (current) {
            m_applicationDetailLabel->setText(current->data(Qt::UserRole + 1).toString());
        }
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
    m_chatHistory->setPlainText(tr("Chat IA visual. Para hacerlo real falta exponer un endpoint de chat en FastAPI."));
    m_chatInput = new QLineEdit(page);
    m_chatInput->setPlaceholderText(tr("Escribí un mensaje..."));
    auto *send = new QPushButton(tr("Enviar"), page);
    connect(send, &QPushButton::clicked, this, &UserWindow::sendChatMessage);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &UserWindow::sendChatMessage);

    auto *row = new QHBoxLayout;
    row->addWidget(m_chatInput, 1);
    row->addWidget(send);
    layout->addWidget(m_chatHistory, 1);
    layout->addLayout(row);
    return page;
}

void UserWindow::handleLogout()
{
    QString error;
    const QString username = ApiClient::instance().username();

    if (!username.isEmpty()) {
        LocalDbManager::instance().logAction(
            username,
            QStringLiteral("logout"),
            QStringLiteral("Cierre de sesión manual desde panel Usuario"),
            &error
        );
    }

    LocalDbManager::instance().closeActiveSession(&error);
    ApiClient::instance().logout();

    auto *loginWindow = new MainWindow();
    loginWindow->show();
    close();
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
        const QJsonDocument cached = LocalDbManager::instance().getCachedData("job_posts", "all", ApiClient::instance().currentUser().value("id").toInt(), &foundCache);
        if (foundCache && cached.isArray()) {
            m_jobsList->clear();
            for (const QJsonValue &value : cached.array()) {
                const QJsonObject o = value.toObject();
                auto *item = new QListWidgetItem(QString("%1\n%2 | %3 | %4")
                                                     .arg(jtext(o, "title"), jtext(o, "location_mode"), jtext(o, "country"), jtext(o, "status")));
                item->setData(Qt::UserRole, o.value("id").toInt());
                item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)));
                m_jobsList->addItem(item);
            }
            setStatus(tr("Sin conexión/API con error. Mostrando puestos desde SQLite local."));
            return;
        }

        setStatus(tr("Puestos: %1").arg(error), false);
        return;
    }

    LocalDbManager::instance().cacheData("job_posts", "all", doc, ApiClient::instance().currentUser().value("id").toInt());

    m_jobsList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        const QString title = jtext(o, "title");
        const QString mode = jtext(o, "location_mode");
        const QString country = jtext(o, "country");
        const QString status = jtext(o, "status");
        auto *item = new QListWidgetItem(QString("%1\n%2 | %3 | %4").arg(title, mode, country, status));
        item->setData(Qt::UserRole, o.value("id").toInt());
        item->setData(Qt::UserRole + 1,
                      tr("Puesto: %1\nDescripción: %2\nSkills: %3\nExperiencia mínima: %4\nModalidad: %5\nCiudad/País: %6 / %7\nEstado: %8")
                          .arg(title,
                               jtext(o, "description"),
                               jtext(o, "required_skills"),
                               jtext(o, "min_years_experience"),
                               mode,
                               jtext(o, "city"),
                               country,
                               status));
        m_jobsList->addItem(item);
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
    LocalDbManager::instance().logAction(ApiClient::instance().username(), "apply_job", tr("Postulación al job_post_id=%1").arg(item->data(Qt::UserRole).toInt()));
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
    LocalDbManager::instance().logAction(ApiClient::instance().username(), "upload_cv", fileName);
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
    LocalDbManager::instance().cacheData("cv_analysis", QString::number(response.value("cv_document_id").toInt()), QJsonDocument(response), ApiClient::instance().currentUser().value("id").toInt(), 24);
    LocalDbManager::instance().logAction(ApiClient::instance().username(), "analyze_cv", tr("CV=%1 | Puesto=%2").arg(fileName, puesto));
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

    LocalDbManager::instance().cacheData("applications", "my", doc, ApiClient::instance().currentUser().value("id").toInt());

    m_applicationsList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        auto *item = new QListWidgetItem(tr("Postulación #%1\nPuesto ID: %2\nEstado: %3")
                                             .arg(jtext(o, "id"), jtext(o, "job_post_id"), jtext(o, "status")));
        item->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)));
        m_applicationsList->addItem(item);
    }
    setStatus(tr("Postulaciones actualizadas desde la API y cacheadas localmente."));
}

void UserWindow::loadNotifications()
{
    bool ok = false;
    QString error;
    const QJsonDocument doc = ApiClient::instance().get("/notifications/my", &ok, &error);
    if (!ok || !doc.isArray()) {
        setStatus(tr("Notificaciones: %1").arg(error), false);
        return;
    }

    LocalDbManager::instance().cacheData("notifications", "my", doc, ApiClient::instance().currentUser().value("id").toInt());

    m_notificationsList->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        auto *item = new QListWidgetItem(tr("%1%2\n%3")
                                             .arg(o.value("is_read").toBool() ? "[Leída] " : "[Nueva] ",
                                                  jtext(o, "title"),
                                                  jtext(o, "message")));
        item->setData(Qt::UserRole, o.value("id").toInt());
        m_notificationsList->addItem(item);
    }
    setStatus(tr("Notificaciones actualizadas desde la API y cacheadas localmente."));
}

void UserWindow::markSelectedNotificationRead()
{
    auto *item = m_notificationsList->currentItem();
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
    LocalDbManager::instance().logAction(ApiClient::instance().username(), "notification_read", QString::number(id));
    loadNotifications();
}

void UserWindow::sendChatMessage()
{
    const QString text = m_chatInput->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    m_chatHistory->append(tr("Vos: %1").arg(text));
    m_chatHistory->append(tr("Sistema: El chat todavía está en modo visual. Falta endpoint FastAPI para responder IA."));
    m_chatInput->clear();
}
