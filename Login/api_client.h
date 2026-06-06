#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <QByteArray>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

class QNetworkReply;

class ApiClient : public QObject
{
    Q_OBJECT

public:
    static ApiClient &instance();

    QString apiBaseUrl() const;
    QString token() const;
    QString role() const;
    QString displayName() const;
    QString username() const;
    QJsonObject currentUser() const;
    bool isLoggedIn() const;

    void logout();
    void restoreSession(const QString &accessToken, const QJsonObject &user);

    bool health(QString *errorMessage = nullptr);

    bool login(const QString &username,
               const QString &password,
               QString *errorMessage = nullptr);

    bool registerUser(const QString &role,
                      const QString &username,
                      const QString &password,
                      const QString &email,
                      const QString &displayName,
                      QString *errorMessage = nullptr);

    bool createEngineerProfile(const QString &firstName,
                               const QString &lastName,
                               int age,
                               const QString &phone,
                               const QString &country,
                               const QString &mainProgrammingLanguage,
                               QString *errorMessage = nullptr);

    bool createCompanyProfile(const QString &companyName,
                              const QString &contactPhone,
                              QString *errorMessage = nullptr);

    QJsonDocument get(const QString &path,
                      bool *ok = nullptr,
                      QString *errorMessage = nullptr);

    QJsonDocument post(const QString &path,
                       const QJsonObject &payload,
                       bool *ok = nullptr,
                       QString *errorMessage = nullptr);

    QJsonDocument put(const QString &path,
                      const QJsonObject &payload,
                      bool *ok = nullptr,
                      QString *errorMessage = nullptr);

    bool uploadCv(const QString &filePath,
                  QString *errorMessage = nullptr,
                  QJsonObject *responseObject = nullptr);

    bool analyzeCv(const QString &filePath,
                   const QString &puesto,
                   QString *errorMessage = nullptr,
                   QJsonObject *responseObject = nullptr);

    bool sendChatMessage(const QString &message,
                         const QString &scope,
                         qint64 conversationId,
                         const QJsonObject &context,
                         const QString &pdfFilePath,
                         QString *errorMessage = nullptr,
                         QJsonObject *responseObject = nullptr);

private:
    explicit ApiClient(QObject *parent = nullptr);

    QJsonDocument sendJson(const QString &method,
                           const QString &path,
                           const QJsonObject &payload,
                           bool *ok,
                           QString *errorMessage);

    QJsonDocument sendRequest(const QString &method,
                              const QString &path,
                              const QByteArray &body,
                              const QByteArray &contentType,
                              bool *ok,
                              QString *errorMessage);

    QString parseError(QNetworkReply *reply, const QByteArray &body) const;
    QByteArray basicAuthValue() const;

    QString m_apiBaseUrl;
    QString m_basicUser;
    QString m_basicPassword;
    QString m_token;
    QJsonObject m_currentUser;
};

#endif // API_CLIENT_H
