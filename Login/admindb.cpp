#include "admindb.h"

#include <QDir>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QSysInfo>
#include <QVariant>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

AdminDB &AdminDB::instance()
{
    static AdminDB manager;
    return manager;
}

AdminDB::AdminDB(QObject *parent)
    : QObject(parent),
      m_connectionName(QStringLiteral("local_4eng_sqlite"))
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::home().filePath(QStringLiteral(".4eng-login"));
    }

    QDir dir(baseDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        QDir fallback(QCoreApplication::applicationDirPath());
        fallback.mkpath(QStringLiteral("."));
        dir = fallback;
    }

    m_databasePath = dir.filePath(QStringLiteral("local.db"));

    // Migra automáticamente la DB vieja (junto al .exe) a una ruta estable.
    const QString legacyPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("local.db"));
    if (legacyPath != m_databasePath && QFile::exists(legacyPath) && !QFile::exists(m_databasePath)) {
        QFile::copy(legacyPath, m_databasePath);
    }

    qDebug() << "SQLite local DB:" << m_databasePath;
}

QString AdminDB::databasePath() const
{
    return m_databasePath;
}

QString AdminDB::nowIso() const
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString AdminDB::deviceInfo() const
{
    return QStringLiteral("%1 | %2 | %3")
        .arg(QSysInfo::prettyProductName(),
             QSysInfo::machineHostName(),
             QSysInfo::currentCpuArchitecture());
}

bool AdminDB::ensureConnection(QString *errorMessage)
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) {
            return true;
        }
        if (!db.open()) {
            if (errorMessage) {
                *errorMessage = db.lastError().text();
            }
            return false;
        }
        return true;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);

    if (!db.open()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::executeSql(const QString &sql, QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(sql)) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::initialize(QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    const QStringList scripts = {
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER,
                username TEXT UNIQUE,
                role TEXT,
                display_name TEXT,
                email TEXT,
                access_token TEXT,
                login_timestamp DATETIME,
                last_activity DATETIME,
                device_fingerprint TEXT,
                is_active BOOLEAN DEFAULT 1
            )
        )SQL"),
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS login_attempts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT,
                login_timestamp DATETIME,
                success BOOLEAN,
                ip_vps TEXT,
                device_info TEXT
            )
        )SQL"),
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS failed_attempts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE,
                attempt_count INTEGER DEFAULT 0,
                last_failed_attempt DATETIME,
                blocked_until DATETIME,
                is_blocked BOOLEAN DEFAULT 0
            )
        )SQL"),
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS offline_cache (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                cache_type TEXT,
                cache_key TEXT,
                cache_data TEXT,
                cached_at DATETIME,
                expires_at DATETIME,
                user_id INTEGER,
                UNIQUE(cache_type, cache_key, user_id)
            )
        )SQL"),
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS user_preferences (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE,
                theme TEXT DEFAULT 'dark',
                language TEXT DEFAULT 'es',
                auto_login BOOLEAN DEFAULT 1,
                last_viewed_section TEXT,
                font_size TEXT DEFAULT 'medium'
            )
        )SQL"),
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS audit_log (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT,
                action TEXT,
                timestamp DATETIME,
                details TEXT
            )
        )SQL"),
        QStringLiteral(R"SQL(
            CREATE TABLE IF NOT EXISTS sync_metadata (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sync_type TEXT UNIQUE,
                last_sync DATETIME,
                status TEXT
            )
        )SQL")
    };

    for (const QString &script : scripts) {
        if (!executeSql(script, errorMessage)) {
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery infoQuery(db);
    if (!infoQuery.exec(QStringLiteral("PRAGMA table_info(sessions)"))) {
        if (errorMessage) {
            *errorMessage = infoQuery.lastError().text();
        }
        return false;
    }

    QStringList sessionColumns;
    while (infoQuery.next()) {
        sessionColumns << infoQuery.value(QStringLiteral("name")).toString().toLower();
    }

    auto ensureSessionColumn = [&](const QString &columnName, const QString &columnType) {
        if (sessionColumns.contains(columnName.toLower())) {
            return true;
        }

        QSqlQuery alterQuery(db);
        const QString sql = QStringLiteral("ALTER TABLE sessions ADD COLUMN %1 %2")
                                .arg(columnName, columnType);
        if (!alterQuery.exec(sql)) {
            if (errorMessage) {
                *errorMessage = alterQuery.lastError().text();
            }
            return false;
        }

        sessionColumns << columnName.toLower();
        return true;
    };

    if (!ensureSessionColumn(QStringLiteral("email"), QStringLiteral("TEXT"))) {
        return false;
    }

    if (!ensureSessionColumn(QStringLiteral("access_token"), QStringLiteral("TEXT"))) {
        return false;
    }

    return cleanExpiredCache(errorMessage);
}

bool AdminDB::saveSession(const QJsonObject &user,
                                 const QString &accessToken,
                                 QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    db.transaction();

    QSqlQuery deactivate(db);
    if (!deactivate.exec(QStringLiteral("UPDATE sessions SET is_active = 0"))) {
        if (errorMessage) {
            *errorMessage = deactivate.lastError().text();
        }
        db.rollback();
        return false;
    }

    const int userId = user.value(QStringLiteral("id")).toInt(user.value(QStringLiteral("user_id")).toInt());
    const QString username = user.value(QStringLiteral("username")).toString();
    const QString role = user.value(QStringLiteral("role")).toString();
    const QString displayName = user.value(QStringLiteral("display_name")).toString();
    const QString email = user.value(QStringLiteral("email")).toString();
    const QString now = nowIso();
    const QString fingerprint = deviceInfo();
    const auto esc = [](const QString &value) {
        QString out = value;
        out.replace('"', "''");
        return out;
    };

    QSqlQuery updateQuery(db);
    const QString updateSql = QStringLiteral(
        "UPDATE sessions "
        "SET user_id = %1, "
        "role = '%2', "
        "display_name = '%3', "
        "email = '%4', "
        "access_token = '%5', "
        "login_timestamp = '%6', "
        "last_activity = '%7', "
        "device_fingerprint = '%8', "
        "is_active = 1 "
        "WHERE username = '%9'")
        .arg(userId)
        .arg(esc(role))
        .arg(esc(displayName))
        .arg(esc(email))
        .arg(esc(accessToken))
        .arg(esc(now))
        .arg(esc(now))
        .arg(esc(fingerprint))
        .arg(esc(username));

    if (!updateQuery.exec(updateSql)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("saveSession/update: %1").arg(updateQuery.lastError().text());
        }
        db.rollback();
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        QSqlQuery insertQuery(db);
        const QString insertSql = QStringLiteral(
            "INSERT INTO sessions "
            "(user_id, username, role, display_name, email, access_token, "
            "login_timestamp, last_activity, device_fingerprint, is_active) "
            "VALUES (%1, '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9', 1)")
            .arg(userId)
            .arg(esc(username))
            .arg(esc(role))
            .arg(esc(displayName))
            .arg(esc(email))
            .arg(esc(accessToken))
            .arg(esc(now))
            .arg(esc(now))
            .arg(esc(fingerprint));

        if (!insertQuery.exec(insertSql)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("saveSession/insert: %1").arg(insertQuery.lastError().text());
            }
            db.rollback();
            return false;
        }
    }

    return db.commit();
}

bool AdminDB::getActiveSession(QJsonObject *user,
                                      QString *accessToken,
                                      QString *errorMessage)
{
    if (user) {
        *user = QJsonObject();
    }
    if (accessToken) {
        accessToken->clear();
    }

    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        SELECT user_id, username, role, display_name, email, access_token, last_activity
        FROM sessions
        WHERE is_active = 1
        ORDER BY login_timestamp DESC
        LIMIT 1
    )SQL"));

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if (!query.next()) {
        return false;
    }

    const QDateTime lastActivity = QDateTime::fromString(query.value(QStringLiteral("last_activity")).toString(), Qt::ISODate);
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();

    if (!lastActivity.isValid() || lastActivity.secsTo(nowUtc) > (kSessionTimeoutMinutes * 60)) {
        closeActiveSession(nullptr);
        if (errorMessage && !lastActivity.isValid()) {
            *errorMessage = QStringLiteral("Sesión local inválida");
        }
        return false;
    }

    if (user) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), query.value(QStringLiteral("user_id")).toInt());
        obj.insert(QStringLiteral("user_id"), query.value(QStringLiteral("user_id")).toInt());
        obj.insert(QStringLiteral("username"), query.value(QStringLiteral("username")).toString());
        obj.insert(QStringLiteral("role"), query.value(QStringLiteral("role")).toString());
        obj.insert(QStringLiteral("display_name"), query.value(QStringLiteral("display_name")).toString());
        obj.insert(QStringLiteral("email"), query.value(QStringLiteral("email")).toString());
        *user = obj;
    }

    if (accessToken) {
        *accessToken = query.value(QStringLiteral("access_token")).toString();
    }

    return true;
}

bool AdminDB::closeActiveSession(QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral("UPDATE sessions SET is_active = 0, last_activity = datetime('now') WHERE is_active = 1"))) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::updateLastActivity(QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE sessions SET last_activity = :last_activity WHERE is_active = 1"));
    query.bindValue(QStringLiteral(":last_activity"), nowIso());

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::logLoginAttempt(const QString &username,
                                     bool success,
                                     QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        INSERT INTO login_attempts (username, login_timestamp, success, ip_vps, device_info)
        VALUES (:username, :login_timestamp, :success, :ip_vps, :device_info)
    )SQL"));
    query.bindValue(QStringLiteral(":username"), username);
    query.bindValue(QStringLiteral(":login_timestamp"), nowIso());
    query.bindValue(QStringLiteral(":success"), success ? 1 : 0);
    query.bindValue(QStringLiteral(":ip_vps"), QStringLiteral("4eng.com.ar"));
    query.bindValue(QStringLiteral(":device_info"), deviceInfo());

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::isUserBlocked(const QString &username,
                                   QDateTime *blockedUntil,
                                   QString *errorMessage)
{
    if (blockedUntil) {
        *blockedUntil = QDateTime();
    }

    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT blocked_until, is_blocked FROM failed_attempts WHERE username = :username LIMIT 1"));
    query.bindValue(QStringLiteral(":username"), username);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if (!query.next()) {
        return false;
    }

    const bool isBlocked = query.value(QStringLiteral("is_blocked")).toInt() == 1;
    const QString blockedUntilText = query.value(QStringLiteral("blocked_until")).toString();
    const QDateTime until = QDateTime::fromString(blockedUntilText, Qt::ISODate);

    if (!isBlocked || !until.isValid()) {
        return false;
    }

    if (until <= QDateTime::currentDateTimeUtc()) {
        resetFailedAttempts(username, nullptr);
        return false;
    }

    if (blockedUntil) {
        *blockedUntil = until;
    }

    return true;
}

bool AdminDB::incrementFailedAttempts(const QString &username,
                                             int *attemptCount,
                                             QDateTime *blockedUntil,
                                             QString *errorMessage)
{
    if (attemptCount) {
        *attemptCount = 0;
    }
    if (blockedUntil) {
        *blockedUntil = QDateTime();
    }

    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery select(db);
    select.prepare(QStringLiteral("SELECT attempt_count FROM failed_attempts WHERE username = :username LIMIT 1"));
    select.bindValue(QStringLiteral(":username"), username);

    if (!select.exec()) {
        if (errorMessage) {
            *errorMessage = select.lastError().text();
        }
        return false;
    }

    int count = 0;
    if (select.next()) {
        count = select.value(0).toInt();
    }

    count += 1;
    const bool shouldBlock = count >= 5;
    const QDateTime until = shouldBlock ? QDateTime::currentDateTimeUtc().addSecs(15 * 60) : QDateTime();

    QSqlQuery query(db);
    query.prepare(QStringLiteral(R"SQL(
        INSERT INTO failed_attempts
            (username, attempt_count, last_failed_attempt, blocked_until, is_blocked)
        VALUES
            (:username, :attempt_count, :last_failed_attempt, :blocked_until, :is_blocked)
        ON CONFLICT(username) DO UPDATE SET
            attempt_count = excluded.attempt_count,
            last_failed_attempt = excluded.last_failed_attempt,
            blocked_until = excluded.blocked_until,
            is_blocked = excluded.is_blocked
    )SQL"));
    query.bindValue(QStringLiteral(":username"), username);
    query.bindValue(QStringLiteral(":attempt_count"), count);
    query.bindValue(QStringLiteral(":last_failed_attempt"), nowIso());
    query.bindValue(QStringLiteral(":blocked_until"), shouldBlock ? until.toString(Qt::ISODate) : QVariant(QVariant::String));
    query.bindValue(QStringLiteral(":is_blocked"), shouldBlock ? 1 : 0);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if (attemptCount) {
        *attemptCount = count;
    }
    if (blockedUntil) {
        *blockedUntil = until;
    }

    return true;
}

bool AdminDB::resetFailedAttempts(const QString &username,
                                         QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        INSERT INTO failed_attempts
            (username, attempt_count, last_failed_attempt, blocked_until, is_blocked)
        VALUES
            (:username, 0, NULL, NULL, 0)
        ON CONFLICT(username) DO UPDATE SET
            attempt_count = 0,
            blocked_until = NULL,
            is_blocked = 0
    )SQL"));
    query.bindValue(QStringLiteral(":username"), username);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::logAction(const QString &username,
                               const QString &action,
                               const QString &details,
                               QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        INSERT INTO audit_log (username, action, timestamp, details)
        VALUES (:username, :action, :timestamp, :details)
    )SQL"));
    query.bindValue(QStringLiteral(":username"), username);
    query.bindValue(QStringLiteral(":action"), action);
    query.bindValue(QStringLiteral(":timestamp"), nowIso());
    query.bindValue(QStringLiteral(":details"), details);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool AdminDB::cacheData(const QString &cacheType,
                               const QString &cacheKey,
                               const QJsonDocument &data,
                               int userId,
                               int ttlHours,
                               QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        INSERT INTO offline_cache
            (cache_type, cache_key, cache_data, cached_at, expires_at, user_id)
        VALUES
            (:cache_type, :cache_key, :cache_data, :cached_at, :expires_at, :user_id)
        ON CONFLICT(cache_type, cache_key, user_id) DO UPDATE SET
            cache_data = excluded.cache_data,
            cached_at = excluded.cached_at,
            expires_at = excluded.expires_at
    )SQL"));
    query.bindValue(QStringLiteral(":cache_type"), cacheType);
    query.bindValue(QStringLiteral(":cache_key"), cacheKey);
    query.bindValue(QStringLiteral(":cache_data"), QString::fromUtf8(data.toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":cached_at"), nowIso());
    query.bindValue(QStringLiteral(":expires_at"), QDateTime::currentDateTimeUtc().addSecs(ttlHours * 3600).toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":user_id"), userId);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

QJsonDocument AdminDB::getCachedData(const QString &cacheType,
                                            const QString &cacheKey,
                                            int userId,
                                            bool *found,
                                            QString *errorMessage)
{
    if (found) {
        *found = false;
    }

    if (!ensureConnection(errorMessage)) {
        return QJsonDocument();
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        SELECT cache_data
        FROM offline_cache
        WHERE cache_type = :cache_type
          AND cache_key = :cache_key
          AND user_id = :user_id
          AND expires_at > :now
        LIMIT 1
    )SQL"));
    query.bindValue(QStringLiteral(":cache_type"), cacheType);
    query.bindValue(QStringLiteral(":cache_key"), cacheKey);
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":now"), nowIso());

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return QJsonDocument();
    }

    if (!query.next()) {
        return QJsonDocument();
    }

    if (found) {
        *found = true;
    }

    return QJsonDocument::fromJson(query.value(0).toString().toUtf8());
}

bool AdminDB::cleanExpiredCache(QString *errorMessage)
{
    if (!ensureConnection(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("DELETE FROM offline_cache WHERE expires_at <= :now"));
    query.bindValue(QStringLiteral(":now"), nowIso());

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

QStringList AdminDB::getLastSuccessfulLogins(const QString &username,
                                                    int limit,
                                                    QString *errorMessage)
{
    QStringList values;

    if (!ensureConnection(errorMessage)) {
        return values;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(R"SQL(
        SELECT login_timestamp
        FROM login_attempts
        WHERE username = :username AND success = 1
        ORDER BY login_timestamp DESC
        LIMIT :limit
    )SQL"));
    query.bindValue(QStringLiteral(":username"), username);
    query.bindValue(QStringLiteral(":limit"), limit);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return values;
    }

    while (query.next()) {
        values << query.value(0).toString();
    }

    return values;
}
