#ifndef LOCALDBCONFIG_H
#define LOCALDBCONFIG_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>

class LocalDBConfig
{
public:
    /**
     * @brief Obtiene o crea la conexión SQLite local
     * @param connectionName Nombre de la conexión
     * @return QSqlDatabase configurada para SQLite
     */
    static QSqlDatabase getLocalConnection(const QString &connectionName = "local_connection")
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        
        if (db.isValid()) {
            return db;
        }

        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);

        // Ruta: %APPDATA%/4eng-Login/local.db (Windows)
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QString dbPath = appDataPath + "/local.db";

        db.setDatabaseName(dbPath);
        
        if (!db.open()) {
            qWarning() << "Error opening local DB:" << db.lastError().text();
            return db;
        }

        initializeTables(db);
        return db;
    }

    /**
     * @brief Inicializa todas las tablas si no existen
     */
    static bool initializeTables(QSqlDatabase &db)
    {
        QSqlQuery query(db);

        // Tabla: sessions
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS sessions ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  user_id INTEGER NOT NULL,"
            "  username TEXT UNIQUE NOT NULL,"
            "  role TEXT NOT NULL,"
            "  display_name TEXT,"
            "  login_timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  last_activity DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  device_fingerprint TEXT,"
            "  is_active BOOLEAN DEFAULT 1"
            ")"))
        {
            qWarning() << "Error creating sessions table:" << query.lastError().text();
            return false;
        }

        // Tabla: login_attempts
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS login_attempts ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  username TEXT NOT NULL,"
            "  login_timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  success BOOLEAN NOT NULL,"
            "  ip_vps TEXT,"
            "  device_info TEXT"
            ")"))
        {
            qWarning() << "Error creating login_attempts table:" << query.lastError().text();
            return false;
        }

        // Tabla: failed_attempts
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS failed_attempts ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  username TEXT UNIQUE NOT NULL,"
            "  attempt_count INTEGER DEFAULT 0,"
            "  last_failed_attempt DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  blocked_until DATETIME,"
            "  is_blocked BOOLEAN DEFAULT 0"
            ")"))
        {
            qWarning() << "Error creating failed_attempts table:" << query.lastError().text();
            return false;
        }

        // Tabla: offline_cache
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS offline_cache ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  cache_type TEXT NOT NULL,"
            "  cache_key TEXT NOT NULL,"
            "  cache_data TEXT,"
            "  cached_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  expires_at DATETIME,"
            "  user_id INTEGER NOT NULL,"
            "  UNIQUE(cache_type, cache_key, user_id)"
            ")"))
        {
            qWarning() << "Error creating offline_cache table:" << query.lastError().text();
            return false;
        }

        // Tabla: user_preferences
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS user_preferences ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  username TEXT UNIQUE NOT NULL,"
            "  theme TEXT DEFAULT 'dark',"
            "  language TEXT DEFAULT 'es',"
            "  auto_login BOOLEAN DEFAULT 1,"
            "  last_viewed_section TEXT,"
            "  font_size TEXT DEFAULT 'medium'"
            ")"))
        {
            qWarning() << "Error creating user_preferences table:" << query.lastError().text();
            return false;
        }

        // Tabla: audit_log
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS audit_log ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  username TEXT NOT NULL,"
            "  action TEXT NOT NULL,"
            "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  details TEXT"
            ")"))
        {
            qWarning() << "Error creating audit_log table:" << query.lastError().text();
            return false;
        }

        // Tabla: sync_metadata (para rastrear sincronizaciones)
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS sync_metadata ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  sync_type TEXT UNIQUE NOT NULL,"
            "  last_sync DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  status TEXT DEFAULT 'pending'"
            ")"))
        {
            qWarning() << "Error creating sync_metadata table:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Guarda una nueva sesión de login
     */
    static bool saveSession(QSqlDatabase &db, int userId, const QString &username, 
                           const QString &role, const QString &displayName)
    {
        QSqlQuery query(db);
        query.prepare(
            "INSERT OR REPLACE INTO sessions (user_id, username, role, display_name, login_timestamp, last_activity, is_active) "
            "VALUES (:user_id, :username, :role, :display_name, :login_timestamp, :last_activity, 1)");
        
        query.addBindValue(userId);
        query.addBindValue(username);
        query.addBindValue(role);
        query.addBindValue(displayName);
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(QDateTime::currentDateTime());

        if (!query.exec()) {
            qWarning() << "Error saving session:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Registra un intento de login (exitoso o fallido)
     */
    static bool logLoginAttempt(QSqlDatabase &db, const QString &username, bool success)
    {
        QSqlQuery query(db);
        query.prepare(
            "INSERT INTO login_attempts (username, login_timestamp, success) "
            "VALUES (:username, :timestamp, :success)");
        
        query.addBindValue(username);
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(success ? 1 : 0);

        if (!query.exec()) {
            qWarning() << "Error logging login attempt:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Incrementa el contador de intentos fallidos
     */
    static bool incrementFailedAttempts(QSqlDatabase &db, const QString &username)
    {
        QSqlQuery query(db);
        
        // Verificar si el registro existe
        query.prepare("SELECT attempt_count FROM failed_attempts WHERE username = :username");
        query.addBindValue(username);
        
        if (!query.exec()) {
            qWarning() << "Error querying failed_attempts:" << query.lastError().text();
            return false;
        }

        int attemptCount = 0;
        if (query.next()) {
            attemptCount = query.value(0).toInt();
        }

        attemptCount++;
        bool isBlocked = attemptCount >= 5;
        QDateTime blockedUntil = isBlocked ? QDateTime::currentDateTime().addSecs(15 * 60) : QDateTime();

        // Insertar o actualizar
        query.prepare(
            "INSERT OR REPLACE INTO failed_attempts (username, attempt_count, last_failed_attempt, blocked_until, is_blocked) "
            "VALUES (:username, :attempt_count, :timestamp, :blocked_until, :is_blocked)");
        
        query.addBindValue(username);
        query.addBindValue(attemptCount);
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(blockedUntil);
        query.addBindValue(isBlocked ? 1 : 0);

        if (!query.exec()) {
            qWarning() << "Error updating failed_attempts:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Resetea el contador de intentos fallidos
     */
    static bool resetFailedAttempts(QSqlDatabase &db, const QString &username)
    {
        QSqlQuery query(db);
        query.prepare(
            "INSERT OR REPLACE INTO failed_attempts (username, attempt_count, last_failed_attempt, blocked_until, is_blocked) "
            "VALUES (:username, 0, datetime('now'), NULL, 0)");
        
        query.addBindValue(username);

        if (!query.exec()) {
            qWarning() << "Error resetting failed_attempts:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Verifica si un usuario está bloqueado
     */
    static bool isUserBlocked(QSqlDatabase &db, const QString &username, QString &blockReason)
    {
        QSqlQuery query(db);
        query.prepare(
            "SELECT is_blocked, blocked_until FROM failed_attempts WHERE username = :username");
        
        query.addBindValue(username);

        if (!query.exec()) {
            qWarning() << "Error checking if user is blocked:" << query.lastError().text();
            return false;
        }

        if (query.next()) {
            bool isBlocked = query.value(0).toBool();
            QDateTime blockedUntil = query.value(1).toDateTime();

            if (isBlocked) {
                // Verificar si el bloqueo sigue vigente
                if (QDateTime::currentDateTime() < blockedUntil) {
                    qint64 secondsRemaining = QDateTime::currentDateTime().secsTo(blockedUntil);
                    blockReason = QString("Usuario bloqueado. Reintenta en %1 segundos.").arg(secondsRemaining);
                    return true;
                } else {
                    // El bloqueo expiró, resetear
                    resetFailedAttempts(db, username);
                    return false;
                }
            }
        }

        return false;
    }

    /**
     * @brief Obtiene la sesión activa actual
     */
    static bool getActiveSession(QSqlDatabase &db, QString &username, QString &role, QString &displayName)
    {
        QSqlQuery query(db);
        query.prepare(
            "SELECT username, role, display_name FROM sessions WHERE is_active = 1 LIMIT 1");

        if (!query.exec()) {
            qWarning() << "Error getting active session:" << query.lastError().text();
            return false;
        }

        if (query.next()) {
            username = query.value(0).toString();
            role = query.value(1).toString();
            displayName = query.value(2).toString();
            return true;
        }

        return false;
    }

    /**
     * @brief Cierra la sesión actual
     */
    static bool closeSession(QSqlDatabase &db)
    {
        QSqlQuery query(db);
        query.prepare("UPDATE sessions SET is_active = 0 WHERE is_active = 1");

        if (!query.exec()) {
            qWarning() << "Error closing session:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Registra una acción en el audit log
     */
    static bool logAction(QSqlDatabase &db, const QString &username, const QString &action, const QString &details = "")
    {
        QSqlQuery query(db);
        query.prepare(
            "INSERT INTO audit_log (username, action, timestamp, details) "
            "VALUES (:username, :action, :timestamp, :details)");
        
        query.addBindValue(username);
        query.addBindValue(action);
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(details);

        if (!query.exec()) {
            qWarning() << "Error logging action:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Obtiene los últimos N intentos de login exitosos
     */
    static QList<QJsonObject> getLastLoginAttempts(QSqlDatabase &db, const QString &username, int limit = 5)
    {
        QList<QJsonObject> attempts;
        QSqlQuery query(db);
        query.prepare(
            "SELECT login_timestamp, success FROM login_attempts WHERE username = :username AND success = 1 "
            "ORDER BY login_timestamp DESC LIMIT :limit");
        
        query.addBindValue(username);
        query.addBindValue(limit);

        if (!query.exec()) {
            qWarning() << "Error getting last login attempts:" << query.lastError().text();
            return attempts;
        }

        while (query.next()) {
            QJsonObject obj;
            obj["timestamp"] = query.value(0).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
            obj["success"] = query.value(1).toBool();
            attempts.append(obj);
        }

        return attempts;
    }

    /**
     * @brief Guarda datos en caché offline
     */
    static bool cacheData(QSqlDatabase &db, const QString &cacheType, const QString &cacheKey, 
                         const QString &cacheData, int userId, int expirationMinutes = 1440) // 24h por defecto
    {
        QSqlQuery query(db);
        query.prepare(
            "INSERT OR REPLACE INTO offline_cache (cache_type, cache_key, cache_data, cached_at, expires_at, user_id) "
            "VALUES (:cache_type, :cache_key, :cache_data, :cached_at, :expires_at, :user_id)");
        
        query.addBindValue(cacheType);
        query.addBindValue(cacheKey);
        query.addBindValue(cacheData);
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(QDateTime::currentDateTime().addSecs(expirationMinutes * 60));
        query.addBindValue(userId);

        if (!query.exec()) {
            qWarning() << "Error caching data:" << query.lastError().text();
            return false;
        }

        return true;
    }

    /**
     * @brief Obtiene datos del caché
     */
    static QString getCachedData(QSqlDatabase &db, const QString &cacheType, const QString &cacheKey, int userId)
    {
        QSqlQuery query(db);
        query.prepare(
            "SELECT cache_data FROM offline_cache "
            "WHERE cache_type = :cache_type AND cache_key = :cache_key AND user_id = :user_id "
            "AND expires_at > datetime('now')");
        
        query.addBindValue(cacheType);
        query.addBindValue(cacheKey);
        query.addBindValue(userId);

        if (!query.exec()) {
            qWarning() << "Error getting cached data:" << query.lastError().text();
            return "";
        }

        if (query.next()) {
            return query.value(0).toString();
        }

        return "";
    }

    /**
     * @brief Limpia caché expirado
     */
    static bool cleanExpiredCache(QSqlDatabase &db)
    {
        QSqlQuery query(db);
        query.prepare("DELETE FROM offline_cache WHERE expires_at <= datetime('now')");

        if (!query.exec()) {
            qWarning() << "Error cleaning expired cache:" << query.lastError().text();
            return false;
        }

        return true;
    }
};

#endif // LOCALDBCONFIG_H
