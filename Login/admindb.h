#ifndef ADMINDB_H
#define ADMINDB_H

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

class AdminDB : public QObject
{
    Q_OBJECT

public:
    static AdminDB &instance();

    QString databasePath() const;

    bool initialize(QString *errorMessage = nullptr);

    bool saveSession(const QJsonObject &user,
                     const QString &accessToken,
                     QString *errorMessage = nullptr);

    bool getActiveSession(QJsonObject *user,
                          QString *accessToken,
                          QString *errorMessage = nullptr);

    bool closeActiveSession(QString *errorMessage = nullptr);
    bool updateLastActivity(QString *errorMessage = nullptr);

    bool logLoginAttempt(const QString &username,
                         bool success,
                         QString *errorMessage = nullptr);

    bool isUserBlocked(const QString &username,
                       QDateTime *blockedUntil = nullptr,
                       QString *errorMessage = nullptr);

    bool incrementFailedAttempts(const QString &username,
                                 int *attemptCount = nullptr,
                                 QDateTime *blockedUntil = nullptr,
                                 QString *errorMessage = nullptr);

    bool resetFailedAttempts(const QString &username,
                             QString *errorMessage = nullptr);

    bool logAction(const QString &username,
                   const QString &action,
                   const QString &details = QString(),
                   QString *errorMessage = nullptr);

    bool cacheData(const QString &cacheType,
                   const QString &cacheKey,
                   const QJsonDocument &data,
                   int userId = 0,
                   int ttlHours = 24,
                   QString *errorMessage = nullptr);

    QJsonDocument getCachedData(const QString &cacheType,
                                const QString &cacheKey,
                                int userId,
                                bool *found = nullptr,
                                QString *errorMessage = nullptr);

    bool cleanExpiredCache(QString *errorMessage = nullptr);

    QStringList getLastSuccessfulLogins(const QString &username,
                                        int limit = 5,
                                        QString *errorMessage = nullptr);

private:
    explicit AdminDB(QObject *parent = nullptr);

    static constexpr int kSessionTimeoutMinutes = 5;

    QString nowIso() const;
    QString deviceInfo() const;
    bool ensureConnection(QString *errorMessage = nullptr);
    bool executeSql(const QString &sql, QString *errorMessage = nullptr);

    QString m_connectionName;
    QString m_databasePath;
};

#endif // ADMINDB_H
