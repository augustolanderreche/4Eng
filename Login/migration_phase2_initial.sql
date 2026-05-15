-- ============================================================
-- MIGRATION SCRIPT - FASE 2 INITIAL PREPARATION
-- Proyecto: 4eng-Login
-- Fecha: 2026-05-15
-- Descripción: Agregar índices y columna last_login para Fase 2
-- ============================================================

USE login_db;

-- ============================================================
-- 1) AGREGAR COLUMNA last_login EN TABLE users
-- ============================================================
-- Propósito: Auditoria básica de último acceso
ALTER TABLE users 
ADD COLUMN last_login DATETIME NULL COMMENT 'Fecha y hora del último login exitoso' AFTER updated_at;

-- ============================================================
-- 2) CREAR ÍNDICES PARA PERFORMANCE EN FASE 2
-- ============================================================

-- Índices en tabla users (búsquedas frecuentes: login, búsqueda por email)
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_role ON users(role);

-- Índices en tabla applications (filtrado por ingeniero y vacante)
CREATE INDEX idx_applications_engineer ON applications(engineer_user_id);
CREATE INDEX idx_applications_job ON applications(job_post_id);
CREATE INDEX idx_applications_company ON applications(company_user_id);
CREATE INDEX idx_applications_status ON applications(status);

-- Índices en tabla job_posts (listado de vacantes por empresa)
CREATE INDEX idx_job_posts_company ON job_posts(company_user_id);
CREATE INDEX idx_job_posts_status ON job_posts(status);

-- Índices en tabla cv_documents (búsqueda de CVs por usuario)
CREATE INDEX idx_cv_documents_engineer ON cv_documents(engineer_user_id);

-- Índices en tabla ai_evaluations (búsqueda de evaluaciones por aplicación)
CREATE INDEX idx_ai_evaluations_application ON ai_evaluations(application_id);

-- Índices en tabla engineer_profiles y company_profiles (búsqueda por usuario)
CREATE INDEX idx_engineer_profiles_user ON engineer_profiles(user_id);
CREATE INDEX idx_company_profiles_user ON company_profiles(user_id);

-- ============================================================
-- 3) VERIFICACIÓN POST-MIGRACIÓN
-- ============================================================
-- Ejecuta esto DESPUÉS para verificar que todo se creó correctamente:

-- SELECT * FROM INFORMATION_SCHEMA.COLUMNS 
-- WHERE TABLE_NAME='users' AND COLUMN_NAME='last_login';

-- SHOW INDEX FROM users;
-- SHOW INDEX FROM applications;
-- SHOW INDEX FROM job_posts;
-- SHOW INDEX FROM cv_documents;
-- SHOW INDEX FROM ai_evaluations;
-- SHOW INDEX FROM engineer_profiles;
-- SHOW INDEX FROM company_profiles;

-- ============================================================
-- FIN DE MIGRACIÓN
-- ============================================================
