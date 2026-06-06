-- =========================================================
-- VPS-POO - BASE FINAL LIMPIA
-- Sistema de gestion de CVs, postulaciones e IA
-- MySQL 8.x / MariaDB compatible en su mayor parte
-- Pensado para Docker init.sql o phpMyAdmin
-- =========================================================

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";
SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- IMPORTANTE:
-- Este script reinicia la base completa. Usalo solo si todavia no tenes datos reales.
DROP DATABASE IF EXISTS `vps-poo`;
CREATE DATABASE `vps-poo`
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_0900_ai_ci;
USE `vps-poo`;

-- =========================================================
-- 1) USUARIOS / AUTENTICACION
-- =========================================================
CREATE TABLE `users` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `role` ENUM('Usuario','Empresa','Admin') NOT NULL,
  `username` VARCHAR(50) NOT NULL,
  `email` VARCHAR(120) NOT NULL,
  `password_hash` VARCHAR(255) NOT NULL,
  `display_name` VARCHAR(120) NOT NULL,
  `is_active` TINYINT(1) NOT NULL DEFAULT 1,
  `is_online` TINYINT(1) NOT NULL DEFAULT 0,
  `last_login` DATETIME DEFAULT NULL,
  `last_seen_at` DATETIME DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `deleted_at` DATETIME DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_users_username` (`username`),
  UNIQUE KEY `uq_users_email` (`email`),
  KEY `idx_users_role` (`role`),
  KEY `idx_users_active` (`is_active`, `deleted_at`),
  KEY `idx_users_online` (`is_online`, `last_seen_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 2) PERFILES
-- =========================================================
CREATE TABLE `engineer_profiles` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` BIGINT UNSIGNED NOT NULL,
  `first_name` VARCHAR(80) NOT NULL,
  `last_name` VARCHAR(80) NOT NULL,
  `age` INT DEFAULT NULL,
  `phone` VARCHAR(30) DEFAULT NULL,
  `country` VARCHAR(80) DEFAULT NULL,
  `main_programming_language` VARCHAR(80) DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_engineer_profiles_user` (`user_id`),
  CONSTRAINT `fk_engineer_profiles_user`
    FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `company_profiles` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` BIGINT UNSIGNED NOT NULL,
  `company_name` VARCHAR(140) NOT NULL,
  `contact_phone` VARCHAR(30) DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_company_profiles_user` (`user_id`),
  KEY `idx_company_profiles_name` (`company_name`),
  CONSTRAINT `fk_company_profiles_user`
    FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 3) SKILLS
-- =========================================================
CREATE TABLE `skills` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(80) NOT NULL,
  `category` VARCHAR(80) DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_skills_name` (`name`),
  KEY `idx_skills_category` (`category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `user_skills` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` BIGINT UNSIGNED NOT NULL,
  `skill_id` BIGINT UNSIGNED NOT NULL,
  `level` ENUM('Beginner','Intermediate','Advanced','Expert') NOT NULL DEFAULT 'Beginner',
  `years_experience` DECIMAL(4,1) DEFAULT NULL,
  `source` ENUM('MANUAL','CV_AI','ADMIN') NOT NULL DEFAULT 'MANUAL',
  `confidence` DECIMAL(5,2) DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_user_skill` (`user_id`, `skill_id`),
  KEY `idx_user_skills_user` (`user_id`),
  KEY `idx_user_skills_skill` (`skill_id`),
  CONSTRAINT `fk_user_skills_user`
    FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_user_skills_skill`
    FOREIGN KEY (`skill_id`) REFERENCES `skills` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 4) PUBLICACIONES LABORALES
-- =========================================================
CREATE TABLE `job_posts` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `company_user_id` BIGINT UNSIGNED NOT NULL,
  `title` VARCHAR(150) NOT NULL,
  `description` MEDIUMTEXT NOT NULL,
  `required_skills_snapshot` JSON DEFAULT NULL,
  `min_years_experience` DECIMAL(4,1) DEFAULT NULL,
  `seniority` ENUM('Trainee','Junior','SemiSenior','Senior','Lead') DEFAULT NULL,
  `location_mode` ENUM('Remoto','Hibrido','Presencial') NOT NULL DEFAULT 'Remoto',
  `city` VARCHAR(80) DEFAULT NULL,
  `country` VARCHAR(80) DEFAULT NULL,
  `salary_min` DECIMAL(12,2) DEFAULT NULL,
  `salary_max` DECIMAL(12,2) DEFAULT NULL,
  `currency` VARCHAR(10) DEFAULT 'ARS',
  `status` ENUM('Borrador','Abierta','Pausada','Cerrada') NOT NULL DEFAULT 'Abierta',
  `published_at` DATETIME DEFAULT NULL,
  `closed_at` DATETIME DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_job_posts_company` (`company_user_id`),
  KEY `idx_job_posts_status` (`status`),
  KEY `idx_job_posts_seniority` (`seniority`),
  CONSTRAINT `fk_job_posts_company`
    FOREIGN KEY (`company_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `job_post_skills` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `job_post_id` BIGINT UNSIGNED NOT NULL,
  `skill_id` BIGINT UNSIGNED NOT NULL,
  `min_level` ENUM('Beginner','Intermediate','Advanced','Expert') NOT NULL DEFAULT 'Beginner',
  `is_required` TINYINT(1) NOT NULL DEFAULT 1,
  `weight` DECIMAL(5,2) NOT NULL DEFAULT 1.00,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_job_post_skill` (`job_post_id`, `skill_id`),
  KEY `idx_job_post_skills_job` (`job_post_id`),
  KEY `idx_job_post_skills_skill` (`skill_id`),
  CONSTRAINT `fk_job_post_skills_job`
    FOREIGN KEY (`job_post_id`) REFERENCES `job_posts` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_job_post_skills_skill`
    FOREIGN KEY (`skill_id`) REFERENCES `skills` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 5) CVS Y ANALISIS GENERAL DEL CV
-- =========================================================
CREATE TABLE `cv_documents` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `engineer_user_id` BIGINT UNSIGNED NOT NULL,
  `file_name` VARCHAR(255) NOT NULL,
  `original_file_name` VARCHAR(255) DEFAULT NULL,
  `file_url` VARCHAR(500) NOT NULL,
  `mime_type` VARCHAR(120) DEFAULT NULL,
  `file_size_bytes` BIGINT UNSIGNED DEFAULT NULL,
  `file_sha256` CHAR(64) DEFAULT NULL,
  `status` ENUM('UPLOADED','PROCESSING','ANALYZED','FAILED') NOT NULL DEFAULT 'UPLOADED',
  `is_active` TINYINT(1) NOT NULL DEFAULT 1,
  `extracted_text` MEDIUMTEXT DEFAULT NULL,
  `parsed_skills` JSON DEFAULT NULL,
  `uploaded_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `analyzed_at` DATETIME DEFAULT NULL,
  `error_message` TEXT DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_cv_documents_engineer` (`engineer_user_id`),
  KEY `idx_cv_documents_status` (`status`),
  KEY `idx_cv_documents_active` (`engineer_user_id`, `is_active`),
  KEY `idx_cv_documents_sha256` (`file_sha256`),
  CONSTRAINT `fk_cv_documents_engineer`
    FOREIGN KEY (`engineer_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `cv_ai_summaries` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `cv_document_id` BIGINT UNSIGNED NOT NULL,
  `engineer_user_id` BIGINT UNSIGNED NOT NULL,
  `summary` TEXT NOT NULL,
  `profile_title` VARCHAR(180) DEFAULT NULL,
  `overall_score` DECIMAL(5,2) DEFAULT NULL,
  `strengths` JSON DEFAULT NULL,
  `weak_points` JSON DEFAULT NULL,
  `detected_skills` JSON DEFAULT NULL,
  `suggested_roles` JSON DEFAULT NULL,
  `training_suggestions` JSON DEFAULT NULL,
  `model_name` VARCHAR(120) DEFAULT NULL,
  `prompt_version` VARCHAR(50) DEFAULT NULL,
  `raw_response` LONGTEXT DEFAULT NULL,
  `is_latest` TINYINT(1) NOT NULL DEFAULT 1,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_cv_ai_cv` (`cv_document_id`),
  KEY `idx_cv_ai_engineer_latest` (`engineer_user_id`, `is_latest`),
  CONSTRAINT `fk_cv_ai_summaries_cv`
    FOREIGN KEY (`cv_document_id`) REFERENCES `cv_documents` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_cv_ai_summaries_engineer`
    FOREIGN KEY (`engineer_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 6) POSTULACIONES Y MATCH IA CV VS PUESTO
-- =========================================================
CREATE TABLE `applications` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `job_post_id` BIGINT UNSIGNED NOT NULL,
  `engineer_user_id` BIGINT UNSIGNED NOT NULL,
  `company_user_id` BIGINT UNSIGNED DEFAULT NULL,
  `cv_document_id` BIGINT UNSIGNED DEFAULT NULL,
  `status` ENUM('Postulada','EnRevision','DatosSolicitados','ReunionSolicitada','Seleccionada','Rechazada','Cancelada') NOT NULL DEFAULT 'Postulada',
  `candidate_message` TEXT DEFAULT NULL,
  `company_message` TEXT DEFAULT NULL,
  `rejection_reason` TEXT DEFAULT NULL,
  `applied_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `reviewed_at` DATETIME DEFAULT NULL,
  `selected_at` DATETIME DEFAULT NULL,
  `rejected_at` DATETIME DEFAULT NULL,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_application_job_engineer` (`job_post_id`, `engineer_user_id`),
  KEY `idx_applications_engineer` (`engineer_user_id`),
  KEY `idx_applications_company_status` (`company_user_id`, `status`),
  KEY `idx_applications_job` (`job_post_id`),
  KEY `idx_applications_status` (`status`),
  KEY `idx_applications_cv` (`cv_document_id`),
  CONSTRAINT `fk_applications_job_post`
    FOREIGN KEY (`job_post_id`) REFERENCES `job_posts` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_applications_engineer`
    FOREIGN KEY (`engineer_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_applications_company`
    FOREIGN KEY (`company_user_id`) REFERENCES `users` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_applications_cv_document`
    FOREIGN KEY (`cv_document_id`) REFERENCES `cv_documents` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `application_status_history` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `application_id` BIGINT UNSIGNED NOT NULL,
  `old_status` VARCHAR(40) DEFAULT NULL,
  `new_status` VARCHAR(40) NOT NULL,
  `changed_by_user_id` BIGINT UNSIGNED DEFAULT NULL,
  `note` TEXT DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_app_status_history_app` (`application_id`),
  KEY `idx_app_status_history_user` (`changed_by_user_id`),
  CONSTRAINT `fk_app_status_history_app`
    FOREIGN KEY (`application_id`) REFERENCES `applications` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_app_status_history_user`
    FOREIGN KEY (`changed_by_user_id`) REFERENCES `users` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `ai_evaluations` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `application_id` BIGINT UNSIGNED DEFAULT NULL,
  `engineer_user_id` BIGINT UNSIGNED NOT NULL,
  `company_user_id` BIGINT UNSIGNED DEFAULT NULL,
  `job_post_id` BIGINT UNSIGNED DEFAULT NULL,
  `cv_document_id` BIGINT UNSIGNED DEFAULT NULL,
  `model_name` VARCHAR(120) NOT NULL,
  `prompt_version` VARCHAR(50) DEFAULT NULL,
  `score` DECIMAL(5,2) DEFAULT NULL,
  `recommendation` ENUM('NoRecomendada','RecomendadaConCapacitacion','Recomendada') NOT NULL DEFAULT 'RecomendadaConCapacitacion',
  `strengths` JSON DEFAULT NULL,
  `missing_skills` JSON DEFAULT NULL,
  `training_suggestions` JSON DEFAULT NULL,
  `summary_for_user` TEXT DEFAULT NULL,
  `summary_for_company` TEXT DEFAULT NULL,
  `raw_response` LONGTEXT DEFAULT NULL,
  `is_latest` TINYINT(1) NOT NULL DEFAULT 1,
  `evaluated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_ai_eval_application` (`application_id`),
  KEY `idx_ai_eval_engineer` (`engineer_user_id`),
  KEY `idx_ai_eval_job` (`job_post_id`),
  KEY `idx_ai_eval_company_job` (`company_user_id`, `job_post_id`),
  KEY `idx_ai_eval_latest` (`engineer_user_id`, `job_post_id`, `is_latest`),
  CONSTRAINT `fk_ai_eval_application`
    FOREIGN KEY (`application_id`) REFERENCES `applications` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_ai_eval_engineer`
    FOREIGN KEY (`engineer_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_ai_eval_company`
    FOREIGN KEY (`company_user_id`) REFERENCES `users` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_ai_eval_job`
    FOREIGN KEY (`job_post_id`) REFERENCES `job_posts` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_ai_eval_cv`
    FOREIGN KEY (`cv_document_id`) REFERENCES `cv_documents` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `training_recommendations` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `engineer_user_id` BIGINT UNSIGNED NOT NULL,
  `ai_evaluation_id` BIGINT UNSIGNED DEFAULT NULL,
  `cv_summary_id` BIGINT UNSIGNED DEFAULT NULL,
  `skill_id` BIGINT UNSIGNED DEFAULT NULL,
  `skill_name` VARCHAR(120) NOT NULL,
  `recommendation_text` TEXT NOT NULL,
  `provider` VARCHAR(120) DEFAULT NULL,
  `resource_url` VARCHAR(500) DEFAULT NULL,
  `estimated_hours` INT DEFAULT NULL,
  `priority` ENUM('LOW','NORMAL','HIGH') NOT NULL DEFAULT 'NORMAL',
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_training_engineer` (`engineer_user_id`),
  KEY `idx_training_ai_eval` (`ai_evaluation_id`),
  KEY `idx_training_cv_summary` (`cv_summary_id`),
  KEY `idx_training_skill` (`skill_id`),
  CONSTRAINT `fk_training_engineer`
    FOREIGN KEY (`engineer_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_training_ai_eval`
    FOREIGN KEY (`ai_evaluation_id`) REFERENCES `ai_evaluations` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_training_cv_summary`
    FOREIGN KEY (`cv_summary_id`) REFERENCES `cv_ai_summaries` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_training_skill`
    FOREIGN KEY (`skill_id`) REFERENCES `skills` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `user_job_recommendations` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` BIGINT UNSIGNED NOT NULL,
  `job_post_id` BIGINT UNSIGNED NOT NULL,
  `ai_evaluation_id` BIGINT UNSIGNED DEFAULT NULL,
  `score` DECIMAL(5,2) NOT NULL,
  `reason` TEXT DEFAULT NULL,
  `missing_skills` JSON DEFAULT NULL,
  `generated_by` ENUM('RULES','AI') NOT NULL DEFAULT 'AI',
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_user_job_reco` (`user_id`, `job_post_id`),
  KEY `idx_reco_user_score` (`user_id`, `score` DESC),
  KEY `idx_reco_job` (`job_post_id`),
  KEY `idx_reco_ai_eval` (`ai_evaluation_id`),
  CONSTRAINT `fk_reco_user`
    FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_reco_job`
    FOREIGN KEY (`job_post_id`) REFERENCES `job_posts` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_reco_ai_eval`
    FOREIGN KEY (`ai_evaluation_id`) REFERENCES `ai_evaluations` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 7) SOLICITUDES EMPRESA -> CANDIDATO
-- =========================================================
CREATE TABLE `company_candidate_requests` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `company_user_id` BIGINT UNSIGNED NOT NULL,
  `engineer_user_id` BIGINT UNSIGNED NOT NULL,
  `job_post_id` BIGINT UNSIGNED DEFAULT NULL,
  `application_id` BIGINT UNSIGNED DEFAULT NULL,
  `request_type` ENUM('UPLOAD_CV','SCHEDULE_MEETING','REQUEST_DOCUMENTS','CUSTOM_MESSAGE') NOT NULL,
  `title` VARCHAR(180) NOT NULL,
  `details` TEXT DEFAULT NULL,
  `meeting_at` DATETIME DEFAULT NULL,
  `status` ENUM('Pending','Accepted','Declined','Completed','Cancelled') NOT NULL DEFAULT 'Pending',
  `candidate_response` TEXT DEFAULT NULL,
  `responded_at` DATETIME DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_ccr_company_status` (`company_user_id`, `status`),
  KEY `idx_ccr_engineer_status` (`engineer_user_id`, `status`),
  KEY `idx_ccr_job` (`job_post_id`),
  KEY `idx_ccr_app` (`application_id`),
  CONSTRAINT `fk_ccr_company`
    FOREIGN KEY (`company_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_ccr_engineer`
    FOREIGN KEY (`engineer_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_ccr_job`
    FOREIGN KEY (`job_post_id`) REFERENCES `job_posts` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_ccr_app`
    FOREIGN KEY (`application_id`) REFERENCES `applications` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 8) NOTIFICACIONES
-- =========================================================
CREATE TABLE `notifications` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` BIGINT UNSIGNED NOT NULL,
  `type` ENUM('APPLICATION_ACCEPTED','APPLICATION_REJECTED','APPLICATION_STATUS_CHANGED','COMPANY_REQUEST','MEETING_PROPOSED','CV_ANALYZED','NEW_JOB_MATCH','AI_FEEDBACK','SYSTEM') NOT NULL,
  `title` VARCHAR(180) NOT NULL,
  `body` TEXT NOT NULL,
  `related_job_post_id` BIGINT UNSIGNED DEFAULT NULL,
  `related_application_id` BIGINT UNSIGNED DEFAULT NULL,
  `related_request_id` BIGINT UNSIGNED DEFAULT NULL,
  `metadata` JSON DEFAULT NULL,
  `priority` ENUM('LOW','NORMAL','HIGH') NOT NULL DEFAULT 'NORMAL',
  `is_read` TINYINT(1) NOT NULL DEFAULT 0,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `read_at` DATETIME DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_notifications_user_read` (`user_id`, `is_read`, `created_at` DESC),
  KEY `idx_notifications_type` (`type`),
  KEY `idx_notifications_job` (`related_job_post_id`),
  KEY `idx_notifications_app` (`related_application_id`),
  KEY `idx_notifications_req` (`related_request_id`),
  CONSTRAINT `fk_notif_user`
    FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_notif_job`
    FOREIGN KEY (`related_job_post_id`) REFERENCES `job_posts` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_notif_app`
    FOREIGN KEY (`related_application_id`) REFERENCES `applications` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_notif_req`
    FOREIGN KEY (`related_request_id`) REFERENCES `company_candidate_requests` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 9) CHAT IA
-- =========================================================
CREATE TABLE `ai_conversations` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `owner_user_id` BIGINT UNSIGNED NOT NULL,
  `owner_role` ENUM('Usuario','Empresa','Admin') NOT NULL,
  `scope` ENUM('APP_HELP','CV_FEEDBACK','JOB_HELP','RECOMMENDATIONS','HIRING_HELP','ADMIN_HELP') NOT NULL,
  `title` VARCHAR(180) DEFAULT NULL,
  `context` JSON DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_ai_conv_owner` (`owner_user_id`),
  KEY `idx_ai_conv_scope` (`scope`),
  CONSTRAINT `fk_ai_conv_owner`
    FOREIGN KEY (`owner_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `ai_messages` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `conversation_id` BIGINT UNSIGNED NOT NULL,
  `sender` ENUM('USER','AI','SYSTEM') NOT NULL,
  `message` TEXT NOT NULL,
  `model_name` VARCHAR(120) DEFAULT NULL,
  `token_usage` INT DEFAULT NULL,
  `metadata` JSON DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_ai_msg_conv` (`conversation_id`, `created_at`),
  CONSTRAINT `fk_ai_msg_conv`
    FOREIGN KEY (`conversation_id`) REFERENCES `ai_conversations` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- =========================================================
-- 10) ADMINISTRACION
-- =========================================================
CREATE TABLE `admin_ai_settings` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `setting_key` VARCHAR(120) NOT NULL,
  `setting_value` TEXT NOT NULL,
  `setting_group` VARCHAR(80) DEFAULT 'GENERAL',
  `description` TEXT DEFAULT NULL,
  `updated_by` BIGINT UNSIGNED DEFAULT NULL,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_ai_setting_key` (`setting_key`),
  KEY `idx_ai_setting_group` (`setting_group`),
  KEY `idx_ai_setting_user` (`updated_by`),
  CONSTRAINT `fk_ai_setting_user`
    FOREIGN KEY (`updated_by`) REFERENCES `users` (`id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `admin_audit_logs` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `admin_user_id` BIGINT UNSIGNED NOT NULL,
  `action` VARCHAR(120) NOT NULL,
  `target_type` VARCHAR(60) DEFAULT NULL,
  `target_id` BIGINT UNSIGNED DEFAULT NULL,
  `details` JSON DEFAULT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_admin_audit_admin` (`admin_user_id`),
  KEY `idx_admin_audit_action` (`action`),
  KEY `idx_admin_audit_target` (`target_type`, `target_id`),
  CONSTRAINT `fk_admin_audit_logs_admin`
    FOREIGN KEY (`admin_user_id`) REFERENCES `users` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

SET FOREIGN_KEY_CHECKS = 1;

-- =========================================================
-- 11) DATOS DEMO PARA PROBAR LA APP
-- Las claves estan hasheadas con bcrypt.
-- Usuarios demo:
--   user_demo / user123
--   empresa_demo / empresa123
--   admin_demo / admin123
-- =========================================================
INSERT INTO `users`
(`id`, `role`, `username`, `email`, `password_hash`, `display_name`, `is_active`, `is_online`, `last_seen_at`)
VALUES
(1, 'Admin', 'admin_demo', 'admin@demo.com', '$2b$12$wte/heBZRHdfN.wUVYh..eDDH9zMKYNLZmYTxAzJa9d4jln6rT2e2', 'Admin Demo', 1, 0, NULL),
(2, 'Empresa', 'empresa_demo', 'empresa@demo.com', '$2b$12$raH6POiTJfPVJXyHbA3Qd.68/pJOurxW7taFV1ccFaJug7/9jcCBK', 'Empresa Demo', 1, 0, NULL),
(3, 'Usuario', 'user_demo', 'user@demo.com', '$2b$12$G2RHry4EdWyziQPBqSjrTe78tXAmPcXRN9vk.kR3MAFr1594NjEIG', 'Usuario Demo', 1, 0, NULL),
(4, 'Usuario', 'sofia_dev', 'sofia@demo.com', '$2b$12$8.GvpZ/J4xon1Z5uCGTmSeE7AFkPcUZQdhjtO0UAEv6UiiSYF2jZO', 'Sofia Diaz', 1, 1, NOW()),
(5, 'Usuario', 'marcos_cpp', 'marcos@demo.com', '$2b$12$MFr8CKoNEw8ezUJNgyrg9.LVSo.edng6I5YQO2Jd0PvDVeh53zhQW', 'Marcos Gomez', 1, 1, NOW());

INSERT INTO `company_profiles`
(`user_id`, `company_name`, `contact_phone`)
VALUES
(2, 'Empresa Demo', '+54 351 000000');

INSERT INTO `engineer_profiles`
(`user_id`, `first_name`, `last_name`, `age`, `main_programming_language`, `country`)
VALUES
(3, 'Usuario', 'Demo', 24, 'Python', 'Argentina'),
(4, 'Sofia', 'Diaz', 27, 'Python', 'Argentina'),
(5, 'Marcos', 'Gomez', 29, 'C++', 'Argentina');

INSERT INTO `skills` (`id`, `name`, `category`) VALUES
(1, 'Python', 'Backend'),
(2, 'FastAPI', 'Backend'),
(3, 'SQL', 'Database'),
(4, 'Docker', 'DevOps'),
(5, 'Testing', 'Quality'),
(6, 'C++', 'Desktop'),
(7, 'Qt', 'Desktop'),
(8, 'JavaScript', 'Frontend'),
(9, 'Java', 'Backend'),
(10, 'Rust', 'Backend'),
(11, 'Go', 'Backend'),
(12, 'APIs REST', 'Backend');

INSERT INTO `user_skills`
(`user_id`, `skill_id`, `level`, `years_experience`, `source`, `confidence`)
VALUES
(3, 1, 'Intermediate', 1.5, 'CV_AI', 90.00),
(3, 3, 'Intermediate', 1.0, 'CV_AI', 85.00),
(4, 1, 'Advanced', 3.0, 'CV_AI', 95.00),
(4, 2, 'Intermediate', 2.0, 'CV_AI', 88.00),
(4, 3, 'Advanced', 3.0, 'CV_AI', 92.00),
(5, 6, 'Advanced', 4.0, 'CV_AI', 94.00),
(5, 7, 'Advanced', 3.5, 'CV_AI', 90.00);

INSERT INTO `job_posts`
(`id`, `company_user_id`, `title`, `description`, `required_skills_snapshot`, `min_years_experience`, `seniority`, `location_mode`, `city`, `country`, `status`, `published_at`)
VALUES
(1, 2, 'Backend Python', 'Desarrollo y mantenimiento de APIs REST con Python, FastAPI y SQL.', JSON_ARRAY('Python','FastAPI','SQL','Docker'), 1.0, 'Junior', 'Remoto', NULL, 'Argentina', 'Abierta', NOW()),
(2, 2, 'C++ Qt Developer', 'Desarrollo de aplicaciones desktop con C++ y Qt.', JSON_ARRAY('C++','Qt'), 2.0, 'SemiSenior', 'Hibrido', 'Cordoba', 'Argentina', 'Abierta', NOW()),
(3, 2, 'Data Analyst', 'Analisis de datos, consultas SQL y reportes para negocio.', JSON_ARRAY('SQL','Python'), 1.0, 'Junior', 'Remoto', NULL, 'Argentina', 'Pausada', NOW());

INSERT INTO `job_post_skills`
(`job_post_id`, `skill_id`, `min_level`, `is_required`, `weight`)
VALUES
(1, 1, 'Intermediate', 1, 2.00),
(1, 2, 'Beginner', 1, 1.50),
(1, 3, 'Intermediate', 1, 1.50),
(1, 4, 'Beginner', 0, 1.00),
(2, 6, 'Intermediate', 1, 2.00),
(2, 7, 'Intermediate', 1, 2.00),
(3, 3, 'Intermediate', 1, 2.00),
(3, 1, 'Beginner', 0, 1.00);

INSERT INTO `cv_documents`
(`id`, `engineer_user_id`, `file_name`, `original_file_name`, `file_url`, `mime_type`, `status`, `is_active`, `parsed_skills`, `uploaded_at`, `analyzed_at`)
VALUES
(1, 3, 'cv_fullstack_2026.pdf', 'cv_fullstack_2026.pdf', '/storage/cvs/cv_fullstack_2026.pdf', 'application/pdf', 'ANALYZED', 1, JSON_ARRAY('Python','SQL'), NOW(), NOW()),
(2, 4, 'cv_backend_python.pdf', 'cv_backend_python.pdf', '/storage/cvs/cv_backend_python.pdf', 'application/pdf', 'ANALYZED', 1, JSON_ARRAY('Python','FastAPI','SQL'), NOW(), NOW()),
(3, 5, 'cv_cpp_qt.pdf', 'cv_cpp_qt.pdf', '/storage/cvs/cv_cpp_qt.pdf', 'application/pdf', 'ANALYZED', 1, JSON_ARRAY('C++','Qt'), NOW(), NOW());

INSERT INTO `cv_ai_summaries`
(`cv_document_id`, `engineer_user_id`, `summary`, `profile_title`, `overall_score`, `strengths`, `weak_points`, `detected_skills`, `suggested_roles`, `training_suggestions`, `model_name`, `prompt_version`, `is_latest`)
VALUES
(1, 3, 'Perfil inicial con buena base en Python y SQL. Puede mejorar Docker, testing y experiencia en APIs productivas.', 'Backend Junior Python', 78.00, JSON_ARRAY('Python','SQL'), JSON_ARRAY('Docker','Testing','FastAPI avanzado'), JSON_ARRAY('Python','SQL'), JSON_ARRAY('Backend Python Junior','Data Analyst Junior'), JSON_ARRAY('Practicar FastAPI','Sumar Docker','Aprender testing de APIs'), 'gpt-4.1-mini', 'cv-summary-v1', 1),
(2, 4, 'Perfil backend fuerte para Python, FastAPI y SQL. Buena candidata para APIs REST.', 'Backend Python SemiSenior', 92.00, JSON_ARRAY('Python','FastAPI','SQL'), JSON_ARRAY('Docker avanzado','Testing'), JSON_ARRAY('Python','FastAPI','SQL'), JSON_ARRAY('Backend Python','API Developer'), JSON_ARRAY('Profundizar Docker','Testing de integracion'), 'gpt-4.1-mini', 'cv-summary-v1', 1),
(3, 5, 'Perfil fuerte para aplicaciones desktop con C++ y Qt.', 'C++ Qt Developer', 88.00, JSON_ARRAY('C++','Qt'), JSON_ARRAY('Testing automatizado','CI/CD'), JSON_ARRAY('C++','Qt'), JSON_ARRAY('C++ Developer','Qt Developer'), JSON_ARRAY('Unit testing en C++','CI/CD'), 'gpt-4.1-mini', 'cv-summary-v1', 1);

INSERT INTO `applications`
(`id`, `job_post_id`, `engineer_user_id`, `company_user_id`, `cv_document_id`, `status`, `candidate_message`, `company_message`, `applied_at`, `reviewed_at`, `selected_at`, `rejected_at`)
VALUES
(1, 1, 4, 2, 2, 'Seleccionada', 'Me interesa el puesto Backend Python.', 'Perfil recomendado para avanzar.', NOW(), NOW(), NOW(), NULL),
(2, 1, 3, 2, 1, 'EnRevision', 'Me gustaria postularme al puesto.', NULL, NOW(), NOW(), NULL, NULL),
(3, 2, 5, 2, 3, 'Postulada', 'Tengo experiencia con C++ y Qt.', NULL, NOW(), NULL, NULL, NULL);

INSERT INTO `application_status_history`
(`application_id`, `old_status`, `new_status`, `changed_by_user_id`, `note`)
VALUES
(1, 'Postulada', 'EnRevision', 2, 'La empresa comenzo la revision.'),
(1, 'EnRevision', 'Seleccionada', 2, 'Candidata seleccionada para avanzar.'),
(2, 'Postulada', 'EnRevision', 2, 'Pendiente de revision por la empresa.');

INSERT INTO `ai_evaluations`
(`application_id`, `engineer_user_id`, `company_user_id`, `job_post_id`, `cv_document_id`, `model_name`, `prompt_version`, `score`, `recommendation`, `strengths`, `missing_skills`, `training_suggestions`, `summary_for_user`, `summary_for_company`, `is_latest`)
VALUES
(1, 4, 2, 1, 2, 'gpt-4.1-mini', 'job-match-v1', 96.00, 'Recomendada', JSON_ARRAY('Python','FastAPI','SQL'), JSON_ARRAY('Docker avanzado'), JSON_ARRAY('Reforzar Docker y testing'), 'Tenes muy buen match con Backend Python.', 'Candidata apta para Backend Python. Buen perfil tecnico.', 1),
(2, 3, 2, 1, 1, 'gpt-4.1-mini', 'job-match-v1', 82.00, 'RecomendadaConCapacitacion', JSON_ARRAY('Python','SQL'), JSON_ARRAY('FastAPI','Docker','Testing'), JSON_ARRAY('Aprender FastAPI','Practicar Docker','Sumar tests'), 'Te conviene este puesto, pero deberias reforzar FastAPI, Docker y testing.', 'Candidato con base adecuada, requiere capacitacion en herramientas del stack.', 1),
(3, 5, 2, 2, 3, 'gpt-4.1-mini', 'job-match-v1', 91.00, 'Recomendada', JSON_ARRAY('C++','Qt'), JSON_ARRAY('Testing C++'), JSON_ARRAY('Unit testing en C++'), 'Tu perfil coincide muy bien con el puesto C++ Qt.', 'Candidato apto para C++ Qt Developer.', 1);

INSERT INTO `user_job_recommendations`
(`user_id`, `job_post_id`, `ai_evaluation_id`, `score`, `reason`, `missing_skills`, `generated_by`)
VALUES
(3, 1, 2, 82.00, 'Buen match por Python y SQL. Faltan FastAPI, Docker y testing.', JSON_ARRAY('FastAPI','Docker','Testing'), 'AI'),
(4, 1, 1, 96.00, 'Excelente match para Backend Python.', JSON_ARRAY('Docker avanzado'), 'AI'),
(5, 2, 3, 91.00, 'Muy buen match para C++ Qt Developer.', JSON_ARRAY('Testing C++'), 'AI');

INSERT INTO `company_candidate_requests`
(`company_user_id`, `engineer_user_id`, `job_post_id`, `application_id`, `request_type`, `title`, `details`, `meeting_at`, `status`)
VALUES
(2, 4, 1, 1, 'SCHEDULE_MEETING', 'Reunion tecnica', 'Coordinar una entrevista tecnica para Backend Python.', '2026-05-30 10:00:00', 'Accepted'),
(2, 3, 1, 2, 'REQUEST_DOCUMENTS', 'Enviar documentacion adicional', 'Enviar certificado o portfolio si lo tiene disponible.', NULL, 'Pending'),
(2, 5, 2, 3, 'CUSTOM_MESSAGE', 'Consulta sobre experiencia Qt', 'Contar brevemente proyectos realizados con Qt.', NULL, 'Pending');

INSERT INTO `notifications`
(`user_id`, `type`, `title`, `body`, `related_job_post_id`, `related_application_id`, `priority`, `is_read`)
VALUES
(4, 'APPLICATION_ACCEPTED', 'Postulacion seleccionada', 'La empresa selecciono tu postulacion para Backend Python.', 1, 1, 'HIGH', 0),
(3, 'APPLICATION_STATUS_CHANGED', 'Postulacion en revision', 'Tu postulacion a Backend Python esta en revision.', 1, 2, 'NORMAL', 0),
(3, 'COMPANY_REQUEST', 'La empresa solicito mas datos', 'La empresa te pidio documentacion adicional.', 1, 2, 'NORMAL', 0);

INSERT INTO `ai_conversations`
(`id`, `owner_user_id`, `owner_role`, `scope`, `title`, `context`)
VALUES
(1, 3, 'Usuario', 'RECOMMENDATIONS', 'Chat IA usuario demo', JSON_OBJECT('purpose','job_recommendations')),
(2, 2, 'Empresa', 'HIRING_HELP', 'Chat IA empresa demo', JSON_OBJECT('purpose','candidate_screening'));

INSERT INTO `ai_messages`
(`conversation_id`, `sender`, `message`, `model_name`)
VALUES
(1, 'USER', 'Hola IA, que puesto me conviene con Python y SQL?', NULL),
(1, 'AI', 'Con tu CV te conviene Backend Python. Para mejorar el match, suma FastAPI, Docker y testing.', 'gpt-4.1-mini'),
(2, 'USER', 'Que skills priorizo para Backend Python?', NULL),
(2, 'AI', 'Priorizaria FastAPI, SQL, APIs REST, testing y Docker.', 'gpt-4.1-mini');

INSERT INTO `admin_ai_settings`
(`setting_key`, `setting_value`, `setting_group`, `description`, `updated_by`)
VALUES
('AI_MODEL_ACTIVE', 'gpt-5.4-mini', 'MODEL', 'Modelo activo usado por la API de IA.', 1),
('AI_TEMPERATURE', '1', 'MODEL', 'Temperatura para respuestas estables.', 1),
('CV_SUMMARY_ENABLED', 'true', 'FEATURE', 'Habilita resumen de CV por IA.', 1),
('CHAT_ENABLED', 'true', 'FEATURE', 'Habilita chatbot para usuario y empresa.', 1),
('PROMPT_CV_SUMMARY', 'Resume fortalezas, gaps y recomendacion final para el CV.', 'PROMPT', 'Prompt principal para analisis de CV.', 1),
('PROMPT_JOB_MATCH', 'Compara el CV contra el puesto y devuelve score, fortalezas y skills faltantes.', 'PROMPT', 'Prompt principal para matching CV vs puesto.', 1);

INSERT INTO `admin_audit_logs`
(`admin_user_id`, `action`, `target_type`, `target_id`, `details`)
VALUES
(1, 'DATABASE_INITIALIZED', 'DATABASE', NULL, JSON_OBJECT('version','final-v1','description','Base inicial limpia para sistema de gestion de CVs e IA'));

-- =========================================================
-- 12) NOTAS DE IMPLEMENTACION
-- =========================================================
-- 1. No existe columna password en texto plano. FastAPI debe verificar password_hash con bcrypt.checkpw.
-- 2. Los admins se limitan desde FastAPI: no permitir mas de 3 users activos con role='Admin'.
-- 3. Para notificaciones en tiempo real:
--    - Simple: polling desde Qt cada 10/15 segundos a /notifications/unread
--    - Mejor: WebSocket desde Qt contra FastAPI
-- 4. required_skills_snapshot es solo una foto/cache para IA o UI.
--    La fuente normalizada real esta en job_post_skills.
-- 5. cv_ai_summaries analiza el CV en general.
--    ai_evaluations analiza CV vs puesto/postulacion.
-- =========================================================
