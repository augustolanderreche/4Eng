# vps — Instrucciones de despliegue del backend

Descripción

La carpeta `vps/` contiene el backend Python, Dockerfiles para el servicio y Nginx, y el script SQL de inicialización (`vps/db/init.sql`). Este documento explica cómo construir y ejecutar el backend localmente o con Docker.

Contenido relevante

- `vps/backend/` : código del backend (API). Archivo principal: `vps/backend/app/main.py`.
- `vps/backend/requirements.txt` : dependencias Python.
- `vps/db/init.sql` : script SQL para crear tablas y datos iniciales.
- `vps/nginx/` : configuración y Dockerfile para Nginx (proxy reverso).

Ejecutar localmente (sin Docker)

1. Crear y activar un entorno virtual de Python 3.11+.
2. Instalar dependencias:

   pip install -r vps/backend/requirements.txt

3. Inicializar la base de datos: ejecutar `vps/db/init.sql` en su servidor SQL (Postgres/MySQL/MariaDB) o adaptar para SQLite.
4. Ejecutar el servidor:

   cd vps/backend
   uvicorn app.main:app --host 0.0.0.0 --port 8000

Construir y ejecutar con Docker

1. Backend: desde `vps/backend/`:

   docker build -t 4eng-backend .
   docker run -d --name 4eng-backend -p 8000:8000 \
     -e DATABASE_URL="<cadena_de_conexion>" \
     -e OTHER_ENV=valor \
     4eng-backend

2. Nginx: desde `vps/nginx/` construir y ejecutar (sigue el mismo patrón). Asegúrate de actualizar `nginx.conf` con la dirección del backend.

Inicializar la base de datos

- Ejecutar `vps/db/init.sql` en la base de datos objetivo antes de arrancar el backend (o montar un contenedor de BD y ejecutar el script dentro).

Variables de entorno recomendadas

- `DATABASE_URL` : cadena de conexión a la BD (Postgres/MySQL).
- `SECRET_KEY` : clave para firmas y JWT.
- `ENV` : `development` o `production`.

Depuración y logs

- Ver logs del backend: `docker logs -f 4eng-backend` o revisar la salida de `uvicorn` si se ejecuta localmente.
- Para inspeccionar un contenedor: `docker exec -it 4eng-backend /bin/sh`.

Notas

- Adapte `vps/db/init.sql` al motor elegido si es necesario.
- Verificar puertos y reglas de firewall al desplegar en VPS real.
- Para producción, use orquestación (Docker Compose / Kubernetes) y configure certificados TLS para Nginx.