# 4Eng Login - Qt Desktop App

## Descripción general

Este proyecto es una aplicación de escritorio escrita en C++ sobre Qt 6. Está pensada como un portal de acceso para tres roles:

- Usuario
- Empresa
- Administrador

La app combina:

- interfaz moderna estilo dark
- conexión real a una API remota
- caché local SQLite
- navegación por pestañas
- tarjetas de contenido para listados, detalles y chat

## Estructura principal

### `main.cpp`

- Inicializa `QApplication` y aplica un estilo global (`Fusion`).
- Define un stylesheet general para botones, labels, cuadros de texto y tarjetas.
- Crea y muestra `MainWindow`.

### `mainwindow.cpp` / `mainwindow.h`

- Implementa la ventana de login principal.
- Maneja el formulario de usuario y contraseña.
- Usa `AdminDB` para inicializar SQLite local.
- Soporta auto-login con sesión almacenada.
- Envía el usuario a la ventana correcta según su rol.

### `userwindow.cpp` / `userwindow.h`

- Implementa la vista del usuario final.
- Presenta un menú izquierdo con pestañas.
- Usa `QStackedWidget` para cambiar entre:
  - Perfil
  - CV + IA
  - Puestos
  - Postulaciones
  - Notificaciones
  - Chat IA
- Muestra listados con `QListWidget` y tarjetas personalizadas.
- Agrega un panel de detalle debajo de cada lista para mostrar la información seleccionada.

### `empresawindow.cpp` / `empresawindow.h`

- Implementa la vista de empresa.
- Tiene pestañas para:
  - Perfil empresa
  - Mis publicaciones
  - Postulantes
  - Nueva publicación
  - Chat IA
- Usa un detalle inferior tipo tarjeta para publicaciones y postulantes, igual que el usuario.
- Ofrece acciones de empresa como ranking IA, aceptar/rechazar postulantes y pedir reuniones.

## Diseño Qt y UI

### Widgets y layouts

- `QMainWindow` para cada ventana principal.
- `QWidget` como contenedor central.
- `QVBoxLayout` y `QHBoxLayout` para ordenar la interfaz.
- `QListWidget` para listados de tarjetas.
- `QTextEdit` para contenidos de texto largos y detalles.
- `QStackedWidget` para navegación de secciones.
- `QFrame` y `QLabel` para tarjetas y encabezados.

### Estilos

- El proyecto aplica estilos con `setStyleSheet` en múltiples lugares.
- Se usan tarjetas oscuras con bordes redondeados y sombras suaves.
- Los roles de UI tienen paletas distintas: violeta para usuario y azul para empresa.
- Se eliminaron bordes directos en labels dentro de los cards para una apariencia más limpia.

## Programación orientada a objetos en Qt

### Encapsulación

- Cada ventana es una clase propia derivada de `QMainWindow`.
- La lógica de UI, eventos y actualizaciones queda contenida dentro de cada clase.
- `ApiClient` y `AdminDB` actúan como servicios separados.

### Responsabilidad única

- `MainWindow` gestiona el acceso y la selección de rol.
- `UserWindow` gestiona la experiencia del postulante.
- `EmpresaWindow` gestiona la experiencia de la empresa.
- `ApiClient` maneja las llamadas HTTP y autenticación.
- `AdminDB` maneja la persistencia local en SQLite.

### Reutilización y funciones auxiliares

- Se definen helpers como `createCardWidget()` para construir tarjetas coherentes.
- Existen métodos utilitarios de formato JSON (`jtext`, `valueToText`, `itemJson`).
- Se reutiliza el mismo patrón de listados y detalle entre usuario y empresa.

### Señales y slots

- Se usan conexiones `connect(...)` para manejar eventos de botones y cambios de selección.
- Ejemplos:
  - botón de login
  - `currentItemChanged` en `QListWidget`
  - temporizadores (`QTimer`) para refresco de notificaciones

### Datos y modelo ligero

- Los objetos JSON se guardan en `QListWidgetItem::setData(Qt::UserRole + 1)`.
- Esto permite mantener los datos completos asociados a cada tarjeta sin recargar la API.
- La UI consume esos datos para mostrar detalles y acciones.

## Funcionalidades principales

- Login real contra API y manejo de token.
- Auto-login con sesión almacenada en SQLite cuando el token sigue válido.
- Caché local de publicaciones y postulaciones para trabajo offline parcial.
- Listados en tarjetas con detalles debajo al seleccionar.
- Chat IA con renderizado HTML de burbujas.
- Notificaciones internas con popup de estilo card.
- Distintas ventanas según el rol del usuario.

## Archivos clave modificados

- `main.cpp`
- `mainwindow.cpp`
- `mainwindow.h`
- `userwindow.cpp`
- `userwindow.h`
- `empresawindow.cpp`
- `empresawindow.h`

## Cómo compilar

1. Abrir el proyecto `.pro` en Qt Creator.
2. Configurar kit de compilación Qt 6.
3. Compilar y ejecutar.

> Si estás usando el build generado, asegúrate de limpiar y recompilar después de cambiar código.

## Notas finales

Este proyecto es un ejemplo completo de aplicación Qt con una arquitectura orientada a objetos clara y modular. La separación de ventanas por rol, la encapsulación del acceso a datos, y el uso de widgets Qt nativos hacen que el código sea fácil de mantener y extender.


