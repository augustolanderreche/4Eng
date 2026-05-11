#ifndef DATABASECONFIG_H
#define DATABASECONFIG_H

#include <QSqlDatabase>
#include <QtGlobal>

namespace DatabaseConfig {

inline QSqlDatabase getLoginConnection(const QString &connectionName = QStringLiteral("login_connection"))
{
    if (QSqlDatabase::contains(connectionName)) {
        return QSqlDatabase::database(connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName);

    const QString host = qEnvironmentVariable("LOGIN_DB_HOST", "127.0.0.1");
    const QString dbName = qEnvironmentVariable("LOGIN_DB_NAME", "login_db");
    const QString user = qEnvironmentVariable("LOGIN_DB_USER", "qtuser");
    const QString password = qEnvironmentVariable("LOGIN_DB_PASSWORD", "qtpass");

    bool ok = false;
    const int port = qEnvironmentVariable("LOGIN_DB_PORT", "3306").toInt(&ok);

    db.setHostName(host);
    db.setPort(ok ? port : 3306);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);

    return db;
}

} // namespace DatabaseConfig

#endif // DATABASECONFIG_H