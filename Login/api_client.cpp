#include "api_client.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QHttpMultiPart>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QTimer>
#include <QVariant>
#include <QUrl>
#include <QUrlQuery>

ApiClient &ApiClient::instance()
{
    static ApiClient client;
    return client;
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
{
    m_apiBaseUrl = qEnvironmentVariable("VPS_POO_API_BASE", "https://4eng.com.ar/api");
    m_basicUser = qEnvironmentVariable("VPS_POO_BASIC_USER", "4eng");
    m_basicPassword = qEnvironmentVariable("VPS_POO_BASIC_PASSWORD", "pelondrakochunchu");

    while (m_apiBaseUrl.endsWith('/')) {
        m_apiBaseUrl.chop(1);
    }
}

QString ApiClient::apiBaseUrl() const
{
    return m_apiBaseUrl;
}

QString ApiClient::token() const
{
    return m_token;
}

QString ApiClient::role() const
{
    return m_currentUser.value("role").toString();
}

QString ApiClient::displayName() const
{
    const QString displayName = m_currentUser.value("display_name").toString();
    if (!displayName.isEmpty()) {
        return displayName;
    }

    const QString usernameValue = username();
    return usernameValue.isEmpty() ? QStringLiteral("Usuario") : usernameValue;
}

QString ApiClient::username() const
{
    const QString usernameValue = m_currentUser.value("username").toString();
    if (!usernameValue.isEmpty()) {
        return usernameValue;
    }
    return m_currentUser.value("sub").toString();
}

QJsonObject ApiClient::currentUser() const
{
    return m_currentUser;
}

bool ApiClient::isLoggedIn() const
{
    return !m_token.isEmpty();
}

void ApiClient::logout()
{
    m_token.clear();
    m_currentUser = QJsonObject();
}

void ApiClient::restoreSession(const QString &accessToken, const QJsonObject &user)
{
    m_token = accessToken;
    m_currentUser = user;
}

QByteArray ApiClient::basicAuthValue() const
{
    const QByteArray plain = QString("%1:%2").arg(m_basicUser, m_basicPassword).toUtf8();
    return "Basic " + plain.toBase64();
}

bool ApiClient::health(QString *errorMessage)
{
    bool ok = false;
    get("/health", &ok, errorMessage);
    return ok;
}

bool ApiClient::login(const QString &username,
                      const QString &password,
                      QString *errorMessage)
{
    QJsonObject payload;
    payload.insert("username", username);
    payload.insert("password", password);

    bool ok = false;
    const QJsonDocument doc = post("/auth/login", payload, &ok, errorMessage);
    if (!ok || !doc.isObject()) {
        return false;
    }

    const QJsonObject root = doc.object();
    m_token = root.value("access_token").toString();
    m_currentUser = root.value("user").toObject();

    if (m_token.isEmpty() || m_currentUser.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("La API respondió, pero no devolvió token o usuario.");
        }
        logout();
        return false;
    }

    return true;
}

bool ApiClient::registerUser(const QString &role,
                             const QString &username,
                             const QString &password,
                             const QString &email,
                             const QString &displayName,
                             QString *errorMessage)
{
    QJsonObject payload;
    payload.insert("role", role);
    payload.insert("username", username);
    payload.insert("password", password);
    payload.insert("email", email);
    payload.insert("display_name", displayName);

    bool ok = false;
    post("/auth/register", payload, &ok, errorMessage);
    return ok;
}

bool ApiClient::createEngineerProfile(const QString &firstName,
                                      const QString &lastName,
                                      int age,
                                      const QString &phone,
                                      const QString &country,
                                      const QString &mainProgrammingLanguage,
                                      QString *errorMessage)
{
    QJsonObject payload;
    payload.insert("first_name", firstName);
    payload.insert("last_name", lastName);
    payload.insert("age", age);
    payload.insert("phone", phone);
    payload.insert("country", country);
    payload.insert("main_programming_language", mainProgrammingLanguage);

    bool ok = false;
    post("/profile/engineer", payload, &ok, errorMessage);
    return ok;
}

bool ApiClient::createCompanyProfile(const QString &companyName,
                                     const QString &contactPhone,
                                     QString *errorMessage)
{
    QJsonObject payload;
    payload.insert("company_name", companyName);
    payload.insert("contact_phone", contactPhone);

    bool ok = false;
    post("/profile/company", payload, &ok, errorMessage);
    return ok;
}

QJsonDocument ApiClient::get(const QString &path,
                             bool *ok,
                             QString *errorMessage)
{
    return sendRequest("GET", path, QByteArray(), QByteArray(), ok, errorMessage);
}

QJsonDocument ApiClient::post(const QString &path,
                              const QJsonObject &payload,
                              bool *ok,
                              QString *errorMessage)
{
    return sendJson("POST", path, payload, ok, errorMessage);
}

QJsonDocument ApiClient::put(const QString &path,
                             const QJsonObject &payload,
                             bool *ok,
                             QString *errorMessage)
{
    return sendJson("PUT", path, payload, ok, errorMessage);
}

QJsonDocument ApiClient::sendJson(const QString &method,
                                  const QString &path,
                                  const QJsonObject &payload,
                                  bool *ok,
                                  QString *errorMessage)
{
    const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    return sendRequest(method, path, body, "application/json", ok, errorMessage);
}

QJsonDocument ApiClient::sendRequest(const QString &method,
                                     const QString &path,
                                     const QByteArray &body,
                                     const QByteArray &contentType,
                                     bool *ok,
                                     QString *errorMessage)
{
    if (ok) {
        *ok = false;
    }
    if (errorMessage) {
        errorMessage->clear();
    }

    QUrl url(m_apiBaseUrl + path);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", basicAuthValue());
    request.setRawHeader("Accept", "application/json");

    if (!contentType.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromUtf8(contentType));
    }

    if (!m_token.isEmpty()) {
        request.setRawHeader("X-Access-Token", QString("Bearer %1").arg(m_token).toUtf8());
    }

    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;

    if (method == "GET") {
        reply = manager.get(request);
    } else if (method == "POST") {
        reply = manager.post(request, body);
    } else if (method == "PUT") {
        reply = manager.put(request, body);
    } else {
        if (errorMessage) {
            *errorMessage = tr("Método HTTP no soportado: %1").arg(method);
        }
        return QJsonDocument();
    }

    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start(20000);
    loop.exec();

    if (!timeout.isActive()) {
        reply->abort();
        reply->deleteLater();
        if (errorMessage) {
            *errorMessage = tr("Tiempo de espera agotado al conectar con %1").arg(url.toString());
        }
        return QJsonDocument();
    }

    timeout.stop();

    const QByteArray responseBody = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300) {
        if (errorMessage) {
            *errorMessage = parseError(reply, responseBody);
        }
        reply->deleteLater();
        return QJsonDocument();
    }

    QJsonParseError parseErrorValue;
    QJsonDocument doc = QJsonDocument::fromJson(responseBody, &parseErrorValue);

    if (parseErrorValue.error != QJsonParseError::NoError && !responseBody.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("La API respondió, pero el JSON no es válido: %1").arg(parseErrorValue.errorString());
        }
        reply->deleteLater();
        return QJsonDocument();
    }

    if (ok) {
        *ok = true;
    }

    reply->deleteLater();
    return doc;
}

QString ApiClient::parseError(QNetworkReply *reply, const QByteArray &body) const
{
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &jsonError);

    if (jsonError.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject obj = doc.object();
        const QJsonValue detail = obj.value("detail");
        if (detail.isString()) {
            return detail.toString();
        }
        if (detail.isArray()) {
            return QString::fromUtf8(QJsonDocument(detail.toArray()).toJson(QJsonDocument::Compact));
        }
    }

    if (!body.trimmed().isEmpty()) {
        return QString::fromUtf8(body.left(600));
    }

    if (httpStatus > 0) {
        return tr("HTTP %1 - %2").arg(httpStatus).arg(reply->errorString());
    }

    return reply->errorString();
}

bool ApiClient::uploadCv(const QString &filePath,
                         QString *errorMessage,
                         QJsonObject *responseObject)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (responseObject) {
        *responseObject = QJsonObject();
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = tr("El archivo no existe.");
        }
        return false;
    }

    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = tr("No se pudo abrir el archivo: %1").arg(file->errorString());
        }
        delete file;
        return false;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileInfo.fileName())));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    QUrl url(m_apiBaseUrl + "/cv/upload");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", basicAuthValue());
    request.setRawHeader("Accept", "application/json");

    if (!m_token.isEmpty()) {
        request.setRawHeader("X-Access-Token", QString("Bearer %1").arg(m_token).toUtf8());
    }

    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start(30000);
    loop.exec();

    if (!timeout.isActive()) {
        reply->abort();
        reply->deleteLater();
        if (errorMessage) {
            *errorMessage = tr("Tiempo de espera agotado subiendo el CV.");
        }
        return false;
    }
    timeout.stop();

    const QByteArray responseBody = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300) {
        if (errorMessage) {
            *errorMessage = parseError(reply, responseBody);
        }
        reply->deleteLater();
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseBody);
    if (responseObject && doc.isObject()) {
        *responseObject = doc.object();
    }

    reply->deleteLater();
    return true;
}


bool ApiClient::analyzeCv(const QString &filePath,
                          const QString &puesto,
                          QString *errorMessage,
                          QJsonObject *responseObject)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (responseObject) {
        *responseObject = QJsonObject();
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = tr("El archivo no existe.");
        }
        return false;
    }

    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = tr("No se pudo abrir el archivo: %1").arg(file->errorString());
        }
        delete file;
        return false;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"archivo\"; filename=\"%1\"").arg(fileInfo.fileName())));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    QHttpPart puestoPart;
    puestoPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant(QStringLiteral("form-data; name=\"puesto\"")));
    puestoPart.setBody(puesto.toUtf8());
    multiPart->append(puestoPart);

    QUrl url(m_apiBaseUrl + "/cv/analyze");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", basicAuthValue());
    request.setRawHeader("Accept", "application/json");

    if (!m_token.isEmpty()) {
        request.setRawHeader("X-Access-Token", QString("Bearer %1").arg(m_token).toUtf8());
    }

    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start(90000);
    loop.exec();

    if (!timeout.isActive()) {
        reply->abort();
        reply->deleteLater();
        if (errorMessage) {
            *errorMessage = tr("Tiempo de espera agotado analizando el CV.");
        }
        return false;
    }
    timeout.stop();

    const QByteArray responseBody = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300) {
        if (errorMessage) {
            *errorMessage = parseError(reply, responseBody);
        }
        reply->deleteLater();
        return false;
    }

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(responseBody, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = tr("La API respondió, pero el análisis no vino como JSON válido.");
        }
        reply->deleteLater();
        return false;
    }

    if (responseObject) {
        *responseObject = doc.object();
    }

    reply->deleteLater();
    return true;
}
