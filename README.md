# 4Eng - Plataforma de Oportunidades Laborales para Ingenieros

**Repositorio para la materia de POO**

**Integrantes:** 
- DRAKO LUSICIC (Manejo de servidor - Desktop)
- NAHUEL PINEDA (FrontEnd - Desktop)
- AUGUSTO LANDERRECHE (BackEnd - Desktop)

**Profesor:** CESAR ALEJANDRO OSIMANI

---

## 📋 Descripción del Proyecto

**4Eng** es una plataforma integral diseñada para conectar programadores e ingenieros con oportunidades laborales, proporcionando herramientas impulsadas por IA para mejorar el proceso de selección y desarrollo profesional.

### 🎯 Objetivos Principales

1. **Bolsa de Trabajo**: Plataforma donde ingenieros y programadores pueden visualizar ofertas laborales y postularse a las mismas
2. **Análisis de CV con IA**: Integración con OpenAI GPT-4.5 mini para:
   - Analizar currículums de candidatos
   - Identificar fortalezas y debilidades
   - Proporcionar retroalimentación detallada
3. **Portal Empresarial**: Espacio para empresas para:
   - Crear y publicar ofertas laborales
   - Generar rankings automáticos de candidatos mediante IA
   - Resumir y analizar currículums
   - Tomar decisiones informadas basadas en datos
4. **Asistencia IA Integrada**: Chat con IA para resolver dudas y consultas
5. **Reducción de Carga de Trabajo**: Automatización del análisis de candidatos y selección mediante IA

---

## 🏗️ Arquitectura del Proyecto

### Estructura General

```
4Eng/
├── Login/                          # Aplicación Qt Desktop (C++)
│   ├── mainwindow.cpp/h           # Ventana de login principal
│   ├── userwindow.cpp/h           # Interfaz para postulantes
│   ├── empresawindow.cpp/h        # Interfaz para empresas
│   ├── adminwindow.cpp/h          # Interfaz administrativa
│   ├── registerwindow.cpp/h       # Ventana de registro
│   ├── api_client.cpp/h           # Cliente HTTP/API
│   ├── localdbmanager.cpp/h       # Gestor de BD local SQLite
│   ├── main.cpp                   # Punto de entrada
│   └── 4eng-Login.pro             # Configuración del proyecto Qt
│
└── vps/backend/                    # Backend Python/FastAPI
    ├── app/
    │   ├── main.py               # API REST principal
    │   ├── auth/                 # Módulo de autenticación
    │   ├── profiles/             # Perfiles de usuarios y empresas
    │   ├── jobs/                 # Gestión de ofertas laborales
    │   ├── applications/         # Gestión de postulaciones
    │   ├── cv/                   # Procesamiento de CVs
    │   └── chat/                 # Chat IA
    ├── ia/                       # Módulo de inteligencia artificial
    │   ├── cv_analyzer.py       # Análisis de CVs con GPT-4.5 mini
    │   ├── ranking_engine.py    # Motor de ranking de candidatos
    │   └── chat_ai.py           # Asistente de chat
    ├── Dockerfile               # Contenedor Docker
    └── requirements.txt         # Dependencias Python
```

---

## 💻 Tecnologías Utilizadas

### Frontend (Desktop)
- **Qt 6** - Framework de interfaz gráfica multiplataforma
- **C++17** - Lenguaje de programación
- **SQLite** - Base de datos local para caché
- **QNetworkManager** - Cliente HTTP para comunicación con API

### Backend
- **Python 3.11** - Lenguaje de programación
- **FastAPI** - Framework web moderno y rápido
- **MySQL/MariaDB** - Base de datos principal
- **OpenAI API** - Integración con GPT-4.5 mini
- **Docker** - Containerización

### Inteligencia Artificial
- **OpenAI GPT-4.5 mini** - Modelo de lenguaje para análisis y ranking
- **Tesseract OCR** - Reconocimiento óptico de caracteres
- **PyPDF2/python-docx** - Procesamiento de documentos (PDF, DOCX)

---

## 🎨 Características Principales

### Para Postulantes (Ingenieros/Programadores)

1. **Gestión de Perfil**
   - Información personal y profesional
   - Lenguaje de programación principal
   - Datos de contacto

2. **Carga de CV**
   - Soporte para PDF y DOCX
   - Procesamiento automático con OCR
   - Almacenamiento seguro

3. **Análisis de CV con IA**
   - Score general de CV
   - Resumen automático
   - Identificación de fortalezas
   - Detección de debilidades/gaps
   - Extracción de skills detectadas

4. **Búsqueda de Empleos**
   - Visualización de ofertas disponibles
   - Filtrado por requisitos
   - Descripción detallada de puestos

5. **Postulaciones**
   - Aplicar a puestos con un clic
   - Seguimiento de estado
   - Historial de postulaciones

6. **Asistente IA**
   - Chat integrado para resolver dudas
   - Consultas sobre CVs y postulaciones
   - Asesoramiento profesional

### Para Empresas

1. **Gestión de Perfil Corporativo**
   - Información de la empresa
   - Datos de contacto
   - Configuración de notificaciones

2. **Publicación de Ofertas**
   - Crear nuevas ofertas laborales
   - Especificar requisitos y skills
   - Descripción detallada del puesto
   - Nivel de seniority

3. **Gestión de Postulantes**
   - Ver candidatos por oferta
   - Acceso a análisis de CV (resumen, fortalezas, gaps)
   - Información de contacto del candidato

4. **Ranking Automático con IA**
   - Generación de rankings por oferta
   - Puntuación de candidatos
   - Recomendaciones automáticas
   - Análisis de compatibilidad (fortalezas, debilidades, capacitación sugerida)
   - Resumen empresarial de candidatos

5. **Acciones sobre Candidatos**
   - Aceptar/rechazar postulaciones
   - Solicitar reuniones
   - Enviar mensajes

---

## 🔐 Seguridad y Autenticación

- **JWT (JSON Web Tokens)** para autenticación
- **Roles basados en acceso** (Usuario, Empresa, Admin)
- **Encriptación de contraseñas** con hashing seguro
- **Validación de tokens** en cada request
- **Almacenamiento local seguro** con SQLite
- **Caché de sesión** para auto-login

---

## 🖥️ Interfaz de Usuario

### Diseño
- **Tema Dark Mode**: Interfaz moderna con paleta de colores oscuros
- **Diseño Responsivo**: Adaptable a diferentes tamaños de pantalla
- **Tarjetas de Contenido**: Presentación clara de información
- **Navegación Intuitiva**: Menús laterales con pestañas

### Componentes Principales

#### Login
- Formulario de usuario/contraseña
- Opción de registro
- Auto-login con sesión guardada
- Estado de conexión local SQLite

#### User Window (Postulante)
- Menú lateral con pestañas:
  - Perfil personal
  - CV + Análisis IA
  - Búsqueda de puestos
  - Mis postulaciones
  - Notificaciones
  - Chat IA
- Panel inferior con detalles de la selección

#### Empresa Window
- Menú lateral con pestañas:
  - Perfil corporativo
  - Mis publicaciones
  - Listado de postulantes
  - Nueva publicación
  - Chat IA
- Panel inferior con detalles de publicaciones/postulantes

---

## 🔄 Flujo de Datos

```
┌─────────────────────────────────────────────────────────┐
│           Desktop Application (Qt/C++)                  │
│  (Login, UserWindow, EmpresaWindow, AdminWindow)        │
│                  SQLite (caché local)                   │
└────────────────────┬────────────────────────────────────┘
                     │ HTTPS (puerto 443)
                     │
┌────────────────────▼────────────────────────────────────┐
│             Nginx (Proxy Reverso / TLS)                 │
└────────────────────┬────────────────────────────────────┘
                     │ HTTP/REST 
                     │
┌────────────────────▼────────────────────────────────────┐
│         Backend API (FastAPI/Python - Uvicorn)          │
│  ┌────────────────┐  ┌──────────────┐  ┌─────────────┐  │
│  │ Authentication │  │ Job Mgmt     │  │ Applications│  │
│  ├────────────────┤  ├──────────────┤  ├─────────────┤  │
│  │ Profiles       │  │ CV Processing│  │ AI Analysis │  │
│  └────────────────┘  └──────────────┘  └─────────────┘  │
└────────────────────┬──────────────────┬─────────────────┘
                     │                  │
        ┌────────────▼─┐      ┌────────▼──────────┐
        │ MySQL/MariaDB│      │ OpenAI API GPT-4.5│
        │   (BD)       │      │   (IA Analysis)   │
        └──────────────┘      └───────────────────┘
```

---

## 📱 API Endpoints Principales

### Autenticación
- `POST /auth/register` - Registro de nuevos usuarios
- `POST /auth/login` - Autenticación de usuarios
- `GET /auth/me` - Obtener datos del usuario actual

### Perfiles
- `POST /profile/engineer` - Crear perfil de ingeniero
- `POST /profile/company` - Crear perfil de empresa
- `GET /profile/engineer/{id}` - Obtener perfil de ingeniero
- `GET /profile/company/{id}` - Obtener perfil de empresa

### CVs
- `POST /cv/upload` - Subir CV (PDF/DOCX)
- `GET /cv/{id}` - Obtener información de CV
- `POST /cv/{id}/analyze` - Analizar CV con IA

### Ofertas Laborales
- `POST /jobs` - Crear nueva oferta
- `GET /jobs` - Listar ofertas disponibles
- `GET /jobs/{id}` - Obtener detalles de oferta
- `PUT /jobs/{id}` - Actualizar oferta

### Postulaciones
- `POST /applications` - Crear postulación
- `GET /applications` - Listar postulaciones del usuario
- `GET /applications/{id}` - Obtener detalles de postulación
- `PUT /applications/{id}` - Actualizar estado de postulación

### IA y Análisis
- `POST /ai/rank-candidates` - Generar ranking de candidatos
- `POST /ai/analyze-cv` - Análisis detallado de CV
- `POST /ai/chat` - Chat con asistente IA

---

## 🚀 Instalación y Ejecución

### Requisitos Previos
- Qt 6.x instalado
- Python 3.11+
- MySQL/MariaDB
- Docker (opcional)

### Backend
```bash
cd vps/backend
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

### Desktop (Qt)
```bash
cd Login
qmake -project
qmake 4eng-Login.pro
make
./4eng-Login
```

### Con Docker
```bash
cd vps/backend
docker build -t 4eng-backend .
docker run -p 8000:8000 4eng-backend
```

---

## 🧪 Ramas del Repositorio

- **main**: Código funcional estable y seguro
- **develop**: Rama de desarrollo con pruebas en progreso
- **feature/***: Nuevas características en desarrollo
- **hotfix/***: Correcciones de bugs críticos

---

## 📚 Programación Orientada a Objetos (POO)

El proyecto implementa principios fundamentales de POO:

### Encapsulación
- Cada ventana es una clase independiente derivada de `QMainWindow`
- Servicios separados: `ApiClient` y `LocalDbManager`
- Lógica encapsulada dentro de cada componente

### Herencia
- Ventanas heredan de `QMainWindow` y `QWidget`
- Reutilización de código a través de clases base

### Polimorfismo
- Diferentes tipos de usuarios (Usuario, Empresa, Admin) con interfaces propias
- Métodos virtuales para comportamiento específico por rol

### Abstracción
- APIs y servicios abstractos separados de la lógica de negocio
- Interfaces limpias para interacción cliente-servidor

---

## 🤝 Contribuciones

Para contribuir al proyecto:
1. Crear una rama feature desde `develop`
2. Realizar cambios y pruebas
3. Crear un Pull Request para revisión
4. Una vez aprobado, fusionar a `develop` y luego a `main`

---

## 📄 Licencia

Este proyecto fue desarrollado como trabajo académico para la materia POO bajo supervisión del profesor CESAR ALEJANDRO OSIMANI.

---

## 📞 Contacto y Soporte

Para preguntas o reportar issues:
- Abrir un issue en el repositorio
- Contactar a los integrantes del proyecto
- Consultar la documentación del backend en `/vps/backend`

---

**Última actualización:** Junio 2026
