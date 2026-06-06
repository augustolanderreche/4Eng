
from datetime import datetime, timedelta, timezone
import os
import shutil
import hashlib
import uuid
import json
from typing import Any
from fastapi import (
    FastAPI,
    HTTPException,
    Request,
    status,
    UploadFile,
    File,
    Form
)
from fastapi.middleware.cors import CORSMiddleware
from jose import JWTError, jwt
import mysql.connector
from pydantic import BaseModel
import bcrypt

try:
    from ia.extractor import extraer_texto
    from ia.analyzer import analizar_cv as analizar_cv_con_ia
    IA_AVAILABLE = True
    IA_IMPORT_ERROR = None
except Exception as exc:
    extraer_texto = None
    analizar_cv_con_ia = None
    IA_AVAILABLE = False
    IA_IMPORT_ERROR = str(exc)
app = FastAPI(
    title="4eng API",
    root_path="/api"
)
JWT_SECRET = os.getenv(
    "JWT_SECRET",
    "dev_secret_change_me"
)
JWT_EXPIRE_MINUTES = 60
UPLOAD_DIR = "uploads"
os.makedirs(
    UPLOAD_DIR,
    exist_ok=True
)
DB_CONFIG = {
    "host": os.getenv("DB_HOST", "127.0.0.1"),
    "port": int(os.getenv("DB_PORT", "3306")),
    "user": os.getenv("DB_USER", "qtuser"),
    "password": os.getenv("DB_PASSWORD", "qtpass"),
    "database": os.getenv("DB_NAME", "vps-poo"),
}
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)
class RegisterRequest(BaseModel):
    role: str
    username: str
    password: str
    email: str
    display_name: str
    programming_language: str | None = None
    age: int | None = None
class LoginRequest(BaseModel):
    username: str
    password: str
class LoginResponse(BaseModel):
    access_token: str
    token_type: str
    user: dict
class EngineerProfileRequest(BaseModel):
    first_name: str
    last_name: str
    age: int | None = None
    phone: str | None = None
    country: str | None = None
    main_programming_language: str | None = None

class CompanyProfileRequest(BaseModel):
    company_name: str
    contact_phone: str | None = None

class JobPostRequest(BaseModel):
    title: str
    description: str
    required_skills: list[str] | None = None
    min_years_experience: float | None = None
    location_mode: str = "Remoto"
    city: str | None = None
    country: str | None = None

class ApplicationRequest(BaseModel):
    job_post_id: int

class SkillRequest(BaseModel):
    name: str

class UserSkillRequest(BaseModel):
    skill_id: int

class JobSkillRequest(BaseModel):
    job_post_id: int
    skill_id: int

class AdminAiSettingUpdateRequest(BaseModel):
    setting_key: str
    setting_value: str


class CompanyApplicationStatusRequest(BaseModel):
    status: str
    message: str | None = None


class CompanyCandidateRequestPayload(BaseModel):
    request_type: str
    title: str | None = None
    details: str | None = None
    meeting_at: str | None = None


class ChatMessageRequest(BaseModel):
    conversation_id: int | None = None
    message: str
    scope: str = "APP_HELP"
    context: dict[str, Any] | None = None


class ChatMessageResponse(BaseModel):
    conversation_id: int
    reply: str
    model_name: str | None = None
    token_usage: int | None = None
    fallback: bool = False

def get_db():
    return mysql.connector.connect(
        **DB_CONFIG
    )

def get_user_by_username(username: str):
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT *
            FROM users
            WHERE username = %s
            LIMIT 1
            """,
            (username,)
        )
        return cursor.fetchone()
    finally:
        conn.close()
def get_user_by_email(email: str):
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT *
            FROM users
            WHERE email = %s
            LIMIT 1
            """,
            (email,)
        )
        return cursor.fetchone()
    finally:
        conn.close()


def hash_password(password: str):
    return bcrypt.hashpw(
        password.encode("utf-8"),
        bcrypt.gensalt()
    ).decode("utf-8")

def verify_password(
    plain_password: str,
    hashed_password: str
):
    return bcrypt.checkpw(
        plain_password.encode("utf-8"),
        hashed_password.encode("utf-8")
    )

def create_access_token(payload: dict):
    now = datetime.now(
        tz=timezone.utc
    )
    exp = now + timedelta(
        minutes=JWT_EXPIRE_MINUTES
    )
    to_encode = payload.copy()
    to_encode.update({
        "exp": exp
    })
    return jwt.encode(
        to_encode,
        JWT_SECRET,
        algorithm="HS256"
    )
def decode_token(token: str):
    try:
        return jwt.decode(
            token,
            JWT_SECRET,
            algorithms=["HS256"]
        )
    except JWTError:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Token invalido"
        )

def get_token(request: Request):
    auth_header = request.headers.get("authorization", "")

    if auth_header.lower().startswith("bearer "):
        return auth_header.split(" ", 1)[1].strip()

    # El frontend actual puede mandar el JWT en X-Access-Token,
    # porque Authorization puede estar usado por Basic Auth en Nginx.
    x_access_token = request.headers.get("x-access-token", "")

    if x_access_token.lower().startswith("bearer "):
        return x_access_token.split(" ", 1)[1].strip()

    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Token faltante"
    )

def require_admin(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    if payload.get("role") != "Admin":
        raise HTTPException(
            status_code=403,
            detail="Solo administradores"
        )
    return payload


def require_company(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    if payload.get("role") != "Empresa":
        raise HTTPException(
            status_code=403,
            detail="Solo empresas"
        )
    return payload


def _safe_json_loads(value, fallback=None):
    if fallback is None:
        fallback = []
    if value is None:
        return fallback
    if isinstance(value, (dict, list)):
        return value
    try:
        return json.loads(value)
    except Exception:
        return fallback


def _json_text(value):
    return json.dumps(value, ensure_ascii=False)


def get_ai_setting(conn, key: str, default_value: str):
    cursor = conn.cursor(dictionary=True)
    cursor.execute(
        """
        SELECT setting_value
        FROM admin_ai_settings
        WHERE setting_key = %s
        LIMIT 1
        """,
        (key,)
    )
    row = cursor.fetchone()
    if not row or row.get("setting_value") is None:
        return default_value
    return str(row["setting_value"])


def _ai_enabled(value: str | None):
    return str(value or "").strip().lower() in {
        "1",
        "true",
        "yes",
        "si",
        "sí",
        "enabled",
        "on"
    }


def _normalize_chat_scope(scope: str | None):
    allowed = {
        "APP_HELP",
        "CV_FEEDBACK",
        "JOB_HELP",
        "RECOMMENDATIONS",
        "HIRING_HELP",
        "ADMIN_HELP"
    }
    value = str(scope or "APP_HELP").strip().upper()
    return value if value in allowed else "APP_HELP"


def _build_chat_system_prompt(role: str, scope: str):
    base = (
        "Sos un asistente de IA integrado en el sistema 4eng. "
        "El sistema gestiona CVs, usuarios ingenieros, empresas, publicaciones laborales, "
        "postulaciones, análisis de CV y recomendaciones. "
        "Respondé siempre en español, con claridad, criterio profesional y sin inventar datos. "
        "Si falta contexto, pedí la información necesaria. "
    )

    if role == "Usuario":
        return (
            base +
            "Estás ayudando a un candidato/ingeniero. Podés orientar sobre mejora de CV, "
            "fortalezas, debilidades, entrevistas, capacitaciones y puestos compatibles."
        )

    if role == "Empresa":
        return (
            base +
            "Estás ayudando a una empresa reclutadora. Podés orientar sobre perfiles, "
            "comparación de candidatos, brechas técnicas, entrevistas y mensajes profesionales."
        )

    if role == "Admin":
        return (
            base +
            "Estás ayudando a un administrador del sistema. Podés explicar funcionamiento, "
            "configuración de IA, usuarios, empresas, postulaciones y estado general del sistema."
        )

    return base


def _call_openai_chat(model_name: str, messages: list[dict]):
    from openai import OpenAI

    client = OpenAI(
        api_key=os.getenv("OPENAI_API_KEY")
    )

    # No enviamos temperature: algunos modelos nuevos solo aceptan el default.
    response = client.chat.completions.create(
        model=model_name,
        messages=messages
    )

    reply = response.choices[0].message.content or ""

    token_usage = None
    try:
        token_usage = response.usage.total_tokens
    except Exception:
        token_usage = None

    return reply, token_usage


def _truncate_text(value: str | None, max_chars: int = 6000):
    text = (value or "").strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "\n\n[Texto recortado por límite de contexto]"


def _safe_json_loads(value):
    if value is None:
        return None
    if isinstance(value, (dict, list)):
        return value
    try:
        return json.loads(value)
    except Exception:
        return value


def _get_user_cv_context(conn, user_id: int, role: str, max_cvs: int = 3):
    """
    Devuelve contexto textual de los CV ya subidos por el usuario logueado.
    Se usa solo para rol Usuario para evitar que una empresa/admin lea CVs ajenos.
    """
    if role != "Usuario":
        return ""

    cursor = conn.cursor(dictionary=True)
    cursor.execute(
        """
        SELECT
            cv.id,
            cv.original_file_name,
            cv.file_name,
            cv.status,
            cv.extracted_text,
            cv.uploaded_at,
            cv.analyzed_at,
            s.summary,
            s.overall_score,
            s.strengths,
            s.weak_points,
            s.detected_skills,
            s.suggested_roles,
            s.training_suggestions
        FROM cv_documents cv
        LEFT JOIN cv_ai_summaries s
            ON s.cv_document_id = cv.id
            AND s.is_latest = 1
        WHERE cv.engineer_user_id = %s
        ORDER BY cv.is_active DESC, cv.uploaded_at DESC, cv.id DESC
        LIMIT %s
        """,
        (
            user_id,
            max_cvs
        )
    )

    rows = cursor.fetchall()

    if not rows:
        return ""

    blocks = []

    for row in rows:
        name = row.get("original_file_name") or row.get("file_name") or "CV sin nombre"

        block = [
            f"CV ID: {row.get('id')}",
            f"Archivo: {name}",
            f"Estado: {row.get('status')}",
            f"Subido: {row.get('uploaded_at')}",
        ]

        if row.get("summary"):
            block.append(f"Resumen IA guardado: {row.get('summary')}")

        if row.get("overall_score") is not None:
            block.append(f"Score IA guardado: {row.get('overall_score')}")

        for label, key in [
            ("Fortalezas", "strengths"),
            ("Debilidades", "weak_points"),
            ("Skills detectadas", "detected_skills"),
            ("Roles sugeridos", "suggested_roles"),
            ("Capacitaciones sugeridas", "training_suggestions"),
        ]:
            value = _safe_json_loads(row.get(key))
            if value:
                block.append(f"{label}: {value}")

        extracted_text = _truncate_text(row.get("extracted_text"), 3500)
        if extracted_text:
            block.append("Texto extraído del CV:\n" + extracted_text)

        blocks.append("\n".join(str(x) for x in block if x is not None))

    return "\n\n---\n\n".join(blocks)


def _save_chat_pdf_and_extract(archivo: UploadFile | None):
    if archivo is None:
        return None, None, None

    filename = archivo.filename or "documento.pdf"
    extension = os.path.splitext(filename)[1].lower()

    if extension != ".pdf":
        raise HTTPException(
            status_code=400,
            detail="El chat solo acepta archivos PDF."
        )

    safe_filename = os.path.basename(filename)
    unique_name = f"chat_{uuid.uuid4()}_{safe_filename}"
    save_path = os.path.join(UPLOAD_DIR, unique_name)

    with open(save_path, "wb") as buffer:
        shutil.copyfileobj(archivo.file, buffer)

    if not IA_AVAILABLE or extraer_texto is None:
        raise HTTPException(
            status_code=503,
            detail=f"No se puede leer el PDF: módulo extractor no disponible. {IA_IMPORT_ERROR}"
        )

    try:
        extracted_text = extraer_texto(save_path)
    except Exception as exc:
        raise HTTPException(
            status_code=400,
            detail=f"No se pudo extraer texto del PDF adjunto: {str(exc)}"
        )

    if not (extracted_text or "").strip():
        raise HTTPException(
            status_code=400,
            detail="No se pudo extraer texto del PDF adjunto."
        )

    return unique_name, save_path, extracted_text


def _parse_chat_context(context):
    if context is None:
        return {}
    if isinstance(context, dict):
        return context
    text = str(context or "").strip()
    if not text:
        return {}
    try:
        value = json.loads(text)
        return value if isinstance(value, dict) else {}
    except Exception:
        return {
            "raw_context": text
        }


def _process_chat_message(
    request: Request,
    conversation_id: int | None,
    message: str,
    scope: str,
    context: dict[str, Any] | None,
    attached_pdf_name: str | None = None,
    attached_pdf_text: str | None = None
):
    token = get_token(request)
    payload = decode_token(token)

    user_id = int(payload["user_id"])
    role = payload.get("role", "Usuario")
    scope = _normalize_chat_scope(scope)
    user_message = (message or "").strip()

    if not user_message:
        raise HTTPException(
            status_code=400,
            detail="El mensaje no puede estar vacío"
        )

    conn = get_db()

    try:
        chat_enabled = get_ai_setting(conn, "CHAT_ENABLED", "true")

        if not _ai_enabled(chat_enabled):
            raise HTTPException(
                status_code=403,
                detail="El chat IA está deshabilitado por configuración"
            )

        model_name = get_ai_setting(
            conn,
            "AI_MODEL_ACTIVE",
            os.getenv("OPENAI_MODEL", "gpt-5.4-mini")
        )

        model_name = os.getenv("OPENAI_MODEL", model_name)

        cursor = conn.cursor(dictionary=True)

        if conversation_id:
            cursor.execute(
                """
                SELECT id, owner_user_id, owner_role, scope, title, context
                FROM ai_conversations
                WHERE id = %s
                  AND owner_user_id = %s
                LIMIT 1
                """,
                (
                    conversation_id,
                    user_id
                )
            )

            conversation = cursor.fetchone()

            if not conversation:
                raise HTTPException(
                    status_code=404,
                    detail="Conversación no encontrada"
                )

        else:
            title = user_message[:80]

            cursor.execute(
                """
                INSERT INTO ai_conversations
                (
                    owner_user_id,
                    owner_role,
                    scope,
                    title,
                    context
                )
                VALUES
                (
                    %s,
                    %s,
                    %s,
                    %s,
                    %s
                )
                """,
                (
                    user_id,
                    role,
                    scope,
                    title,
                    json.dumps(context or {}, ensure_ascii=False)
                )
            )

            conversation_id = cursor.lastrowid

        user_metadata = {
            "scope": scope,
            "role": role,
            "has_attached_pdf": bool(attached_pdf_text),
            "attached_pdf_name": attached_pdf_name
        }

        stored_user_message = user_message
        if attached_pdf_text:
            stored_user_message += f"\n\n[PDF adjunto: {attached_pdf_name}]"

        cursor.execute(
            """
            INSERT INTO ai_messages
            (
                conversation_id,
                sender,
                message,
                model_name,
                metadata
            )
            VALUES
            (
                %s,
                'USER',
                %s,
                NULL,
                %s
            )
            """,
            (
                conversation_id,
                stored_user_message,
                json.dumps(user_metadata, ensure_ascii=False)
            )
        )

        conn.commit()

        cursor.execute(
            """
            SELECT sender, message
            FROM ai_messages
            WHERE conversation_id = %s
            ORDER BY created_at DESC, id DESC
            LIMIT 12
            """,
            (conversation_id,)
        )

        rows = cursor.fetchall()
        rows.reverse()

        system_prompt = _build_chat_system_prompt(role, scope)

        cv_context = _get_user_cv_context(conn, user_id, role)

        if cv_context:
            system_prompt += (
                "\n\nContexto de CVs ya subidos por el usuario que está hablando. "
                "Usá esta información cuando sea relevante y aclará cuando una recomendación se base en estos CVs:\n"
                + cv_context
            )

        if attached_pdf_text:
            system_prompt += (
                f"\n\nEl usuario adjuntó un PDF llamado '{attached_pdf_name}'. "
                "Leé este contenido y usalo como contexto principal si la pregunta se refiere al archivo:\n"
                + _truncate_text(attached_pdf_text, 9000)
            )

        messages = [
            {
                "role": "system",
                "content": system_prompt
            }
        ]

        for row in rows:
            sender = row.get("sender")
            content = row.get("message") or ""

            if sender == "USER":
                messages.append(
                    {
                        "role": "user",
                        "content": content
                    }
                )

            elif sender == "AI":
                messages.append(
                    {
                        "role": "assistant",
                        "content": content
                    }
                )

        fallback = False

        try:
            reply, token_usage = _call_openai_chat(
                model_name,
                messages
            )

        except Exception as exc:
            fallback = True
            token_usage = None
            reply = (
                "No pude consultar OpenAI en este momento. "
                "Dejo una respuesta de respaldo para que el chat siga funcionando. "
                f"Detalle técnico: {str(exc)}"
            )

        cursor.execute(
            """
            INSERT INTO ai_messages
            (
                conversation_id,
                sender,
                message,
                model_name,
                token_usage,
                metadata
            )
            VALUES
            (
                %s,
                'AI',
                %s,
                %s,
                %s,
                %s
            )
            """,
            (
                conversation_id,
                reply,
                model_name,
                token_usage,
                json.dumps(
                    {
                        "fallback": fallback,
                        "scope": scope,
                        "used_user_cv_context": bool(cv_context),
                        "used_attached_pdf": bool(attached_pdf_text),
                        "attached_pdf_name": attached_pdf_name
                    },
                    ensure_ascii=False
                )
            )
        )

        cursor.execute(
            """
            UPDATE ai_conversations
            SET updated_at = CURRENT_TIMESTAMP
            WHERE id = %s
            """,
            (conversation_id,)
        )

        conn.commit()

        return {
            "conversation_id": conversation_id,
            "reply": reply,
            "model_name": model_name,
            "token_usage": token_usage,
            "fallback": fallback
        }

    finally:
        conn.close()


def _candidate_status_notification(status_value: str):
    mapping = {
        "Seleccionada": ("APPLICATION_ACCEPTED", "Fuiste seleccionado"),
        "Rechazada": ("APPLICATION_REJECTED", "Tu postulación fue rechazada"),
        "DatosSolicitados": ("COMPANY_REQUEST", "La empresa solicitó más información"),
        "ReunionSolicitada": ("MEETING_PROPOSED", "La empresa solicitó una reunión"),
        "EnRevision": ("APPLICATION_STATUS_CHANGED", "Tu postulación está en revisión"),
        "Postulada": ("APPLICATION_STATUS_CHANGED", "Tu postulación fue actualizada"),
        "Cancelada": ("APPLICATION_STATUS_CHANGED", "Tu postulación fue cancelada"),
    }
    return mapping.get(status_value, ("APPLICATION_STATUS_CHANGED", "Tu postulación fue actualizada"))


def _normalize_application_status(value: str):
    cleaned = (value or "").strip()
    allowed = {
        "Postulada",
        "EnRevision",
        "DatosSolicitados",
        "ReunionSolicitada",
        "Seleccionada",
        "Rechazada",
        "Cancelada"
    }
    if cleaned not in allowed:
        raise HTTPException(
            status_code=400,
            detail="Estado inválido"
        )
    return cleaned


def _normalize_candidate_request_type(value: str):
    cleaned = (value or "").strip().upper()
    allowed = {
        "UPLOAD_CV",
        "SCHEDULE_MEETING",
        "REQUEST_DOCUMENTS",
        "CUSTOM_MESSAGE"
    }
    if cleaned not in allowed:
        raise HTTPException(
            status_code=400,
            detail="Tipo de solicitud inválido"
        )
    return cleaned


def _candidate_row_to_ranking_item(row: dict, position: int):
    cv_score = row.get("cv_overall_score")
    try:
        cv_score = float(cv_score) if cv_score is not None else None
    except Exception:
        cv_score = None

    status_bonus = {
        "Seleccionada": 5,
        "EnRevision": 3,
        "Postulada": 0,
        "DatosSolicitados": -1,
        "ReunionSolicitada": 2,
        "Rechazada": -15,
        "Cancelada": -20
    }.get(row.get("status"), 0)

    base_score = cv_score if cv_score is not None else 50
    final_score = max(0, min(100, base_score + status_bonus))

    return {
        "position": position,
        "application_id": row.get("application_id"),
        "engineer_user_id": row.get("engineer_user_id"),
        "candidate_name": row.get("candidate_name"),
        "candidate_email": row.get("candidate_email"),
        "job_post_id": row.get("job_post_id"),
        "job_title": row.get("job_title"),
        "status": row.get("status"),
        "cv_document_id": row.get("cv_document_id"),
        "cv_file_name": row.get("cv_file_name"),
        "score": round(final_score, 2),
        "recommendation": (
            "Recomendada"
            if final_score >= 75
            else "RecomendadaConCapacitacion"
            if final_score >= 50
            else "NoRecomendada"
        ),
        "summary_for_company": row.get("cv_summary") or "Sin resumen IA previo del CV.",
        "strengths": _safe_json_loads(row.get("cv_strengths"), []),
        "missing_skills": _safe_json_loads(row.get("cv_weak_points"), []),
        "training_suggestions": _safe_json_loads(row.get("cv_training_suggestions"), [])
    }


def _rank_candidates_with_openai(model_name: str, job: dict, candidates: list[dict]):
    from openai import OpenAI

    client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))

    prompt = {
        "instrucciones": (
            "Ordena candidatos para el puesto. Devuelve SOLO JSON válido. "
            "Cada candidato debe conservar application_id. "
            "Usa score 0-100, recommendation: Recomendada, RecomendadaConCapacitacion o NoRecomendada."
        ),
        "puesto": job,
        "candidatos": candidates
    }

    response = client.chat.completions.create(
        model=model_name,
        messages=[
            {
                "role": "system",
                "content": "Sos un especialista de selección IT. Respondé solo JSON válido."
            },
            {
                "role": "user",
                "content": json.dumps(prompt, ensure_ascii=False)
            }
        ]
    )

    content = response.choices[0].message.content or ""
    parsed = json.loads(content)
    if isinstance(parsed, dict) and isinstance(parsed.get("ranking"), list):
        return parsed["ranking"], content
    if isinstance(parsed, list):
        return parsed, content
    raise ValueError("La IA no devolvió un ranking JSON válido.")

def normalize_location_mode(location_mode: str) -> str:
    value = (location_mode or "").strip().lower()
    if value in ["remoto"]:
        return "Remoto"
    if value in ["hibrido", "híbrido"]:
        return "Hibrido"
    if value in ["presencial"]:
        return "Presencial"
    return "Remoto"

@app.get("/health")
def health():
    return {
        "status": "ok"
    }

@app.post("/auth/register")
def register(payload: RegisterRequest):
    existing_user = get_user_by_username(
        payload.username
    )
    if existing_user:
        raise HTTPException(
            status_code=400,
            detail="El usuario ya existe"
        )
    existing_email = get_user_by_email(
        payload.email
    )
    if existing_email:
        raise HTTPException(
            status_code=400,
            detail="El email ya existe"
        )
    hashed_password = hash_password(
        payload.password
    )
    conn = get_db()
    try:
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO users
            (
                role,
                username,
                password_hash,
                email,
                display_name
            )
            VALUES
            (
                %s,%s,%s,%s,%s
            )
            """,
            (
                payload.role,
                payload.username,
                hashed_password,
                payload.email,
                payload.display_name
            )
        )
        conn.commit()
        return {
            "message": "Usuario registrado"
        }
    finally:
        conn.close()

@app.post(
    "/auth/login",
    response_model=LoginResponse
)
def login(payload: LoginRequest):
    user = get_user_by_username(
        payload.username
    )
    if not user:
        raise HTTPException(
            status_code=401,
            detail="Usuario no encontrado"
        )
    if not verify_password(
        payload.password,
        user["password_hash"]
    ):
        raise HTTPException(
            status_code=401,
            detail="Credenciales invalidas"
        )
    if not user["is_active"]:
        raise HTTPException(
            status_code=401,
            detail="Usuario inactivo"
        )
    conn = get_db()
    try:

        cursor = conn.cursor()

        cursor.execute(
            """
            UPDATE users
            SET last_login = NOW()
            WHERE id = %s
            """,
            (user["id"],)
        )
        conn.commit()

    finally:
        conn.close()
    access_token = create_access_token({

        "sub": user["username"],
        "user_id": user["id"],
        "role": user["role"],
        "display_name": user["display_name"],
        "email": user["email"]

    })

    return {

        "access_token": access_token,

        "token_type": "bearer",

        "user": {

            "id": user["id"],
            "username": user["username"],
            "role": user["role"],
            "display_name": user["display_name"],
            "email": user["email"],
            "programming_language": user.get("programming_language"),
            "age": user.get("age")

        }
    }


@app.get("/auth/me")
def me(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    return {

        "username": payload.get("sub"),
        "user_id": payload.get("user_id"),
        "role": payload.get("role"),
        "display_name": payload.get("display_name"),
        "email": payload.get("email")
    }

@app.post("/profile/engineer")
def create_engineer_profile(
    request: Request,
    data: EngineerProfileRequest
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )
    conn = get_db()
    try:
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO engineer_profiles
            (
                user_id,
                first_name,
                last_name,
                age,
                phone,
                country,
                main_programming_language
            )
            VALUES
            (
                %s,%s,%s,%s,%s,%s,%s
            )
            ON DUPLICATE KEY UPDATE
                first_name = VALUES(first_name),
                last_name = VALUES(last_name),
                age = VALUES(age),
                phone = VALUES(phone),
                country = VALUES(country),
                main_programming_language = VALUES(main_programming_language)
            """,
            (
                payload["user_id"],
                data.first_name,
                data.last_name,
                data.age,
                data.phone,
                data.country,
                data.main_programming_language
            )
        )

        conn.commit()
        return {
            "message": "Perfil ingeniero creado"
        }
    finally:
        conn.close()

@app.post("/profile/company")
def create_company_profile(
    request: Request,
    data: CompanyProfileRequest
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Empresa":
        raise HTTPException(
            status_code=403,
            detail="Solo empresas"
        )
    conn = get_db()
    try:
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO company_profiles
            (
                user_id,
                company_name,
                contact_phone
            )
            VALUES
            (
                %s,%s,%s
            )
            ON DUPLICATE KEY UPDATE
                company_name = VALUES(company_name),
                contact_phone = VALUES(contact_phone)
            """,
            (
                payload["user_id"],
                data.company_name,
                data.contact_phone
            )
        )
        conn.commit()
        return {
            "message": "Perfil empresa creado"
        }

    finally:

        conn.close()

@app.post("/cv/upload")
def upload_cv(
    request: Request,
    file: UploadFile = File(...)
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )
    allowed_extensions = [
        ".pdf",
        ".docx"
    ]
    extension = os.path.splitext(
        file.filename
    )[1].lower()
    if extension not in allowed_extensions:
        raise HTTPException(
            status_code=400,
            detail="Formato no permitido"
        )
    unique_name = (
        str(uuid.uuid4())
        + "_"
        + file.filename
    )
    save_path = os.path.join(
        UPLOAD_DIR,
        unique_name
    )
    with open(
        save_path,
        "wb"
    ) as buffer:
        shutil.copyfileobj(
            file.file,
            buffer
        )
    sha256 = hashlib.sha256()
    with open(
        save_path,
        "rb"
    ) as f:
        while chunk := f.read(4096):
            sha256.update(chunk)
    file_hash = sha256.hexdigest()
    conn = get_db()
    try:
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO cv_documents
            (
                engineer_user_id,
                file_name,
                file_url,
                file_sha256
            )
            VALUES
            (
                %s,%s,%s,%s
            )
            """,
            (
                payload["user_id"],
                unique_name,
                save_path,
                file_hash
            )
        )
        conn.commit()
        return {
            "message": "CV subido correctamente",
            "file_name": unique_name,
            "sha256": file_hash
        }
    finally:

        conn.close()



def _json_for_db(value):
    return json.dumps(value, ensure_ascii=False)


def _analysis_array(analysis: dict, key: str):
    value = analysis.get(key)
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def _analysis_score(analysis: dict):
    value = analysis.get("compatibilidad")
    try:
        if value is None:
            return None
        return float(value)
    except Exception:
        return None


@app.post("/cv/analyze")
async def analyze_cv_with_ai(
    request: Request,
    archivo: UploadFile = File(...),
    puesto: str = Form(...)
):
    require_ia_available()

    token = get_token(request)
    payload = decode_token(token)
    if payload.get("role") != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios pueden analizar CV"
        )

    if not puesto or not puesto.strip():
        raise HTTPException(
            status_code=400,
            detail="El puesto objetivo es obligatorio"
        )

    extensiones_validas = [
        ".pdf",
        ".png",
        ".jpg",
        ".jpeg"
    ]

    filename = archivo.filename or "cv"
    extension = os.path.splitext(filename)[1].lower()

    if extension not in extensiones_validas:
        raise HTTPException(
            status_code=400,
            detail="Formato no permitido para análisis. Usá PDF, PNG, JPG o JPEG."
        )

    safe_filename = os.path.basename(filename)
    unique_name = f"{uuid.uuid4()}_{safe_filename}"
    save_path = os.path.join(UPLOAD_DIR, unique_name)

    with open(save_path, "wb") as buffer:
        shutil.copyfileobj(archivo.file, buffer)

    sha256 = hashlib.sha256()
    file_size = 0
    with open(save_path, "rb") as f:
        while chunk := f.read(4096):
            file_size += len(chunk)
            sha256.update(chunk)
    file_hash = sha256.hexdigest()

    cv_document_id = None
    analysis_id = None

    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        cursor.execute(
            """
            UPDATE cv_documents
            SET is_active = 0
            WHERE engineer_user_id = %s
            """,
            (payload["user_id"],)
        )

        cursor.execute(
            """
            SELECT id
            FROM cv_documents
            WHERE engineer_user_id = %s
            AND file_sha256 = %s
            LIMIT 1
            """,
            (payload["user_id"], file_hash)
        )
        existing_cv = cursor.fetchone()

        if existing_cv:
            cv_document_id = existing_cv["id"]
            cursor.execute(
                """
                UPDATE cv_documents
                SET file_name = %s,
                    original_file_name = %s,
                    file_url = %s,
                    mime_type = %s,
                    file_size_bytes = %s,
                    status = 'PROCESSING',
                    is_active = 1,
                    error_message = NULL
                WHERE id = %s
                """,
                (
                    unique_name,
                    safe_filename,
                    save_path,
                    archivo.content_type,
                    file_size,
                    cv_document_id
                )
            )
        else:
            cursor.execute(
                """
                INSERT INTO cv_documents
                (
                    engineer_user_id,
                    file_name,
                    original_file_name,
                    file_url,
                    mime_type,
                    file_size_bytes,
                    file_sha256,
                    status,
                    is_active
                )
                VALUES
                (
                    %s,%s,%s,%s,%s,%s,%s,'PROCESSING',1
                )
                """,
                (
                    payload["user_id"],
                    unique_name,
                    safe_filename,
                    save_path,
                    archivo.content_type,
                    file_size,
                    file_hash
                )
            )
            cv_document_id = cursor.lastrowid

        conn.commit()

        texto_cv = extraer_texto(save_path)

        if len(texto_cv.strip()) < 30:
            cursor.execute(
                """
                UPDATE cv_documents
                SET status = 'FAILED',
                    error_message = %s
                WHERE id = %s
                """,
                (
                    "No se pudo extraer suficiente texto del archivo",
                    cv_document_id
                )
            )
            conn.commit()
            raise HTTPException(
                status_code=400,
                detail="No se pudo extraer suficiente texto del archivo"
            )

        resultado = analizar_cv_con_ia(texto_cv, puesto)
        if not isinstance(resultado, dict):
            resultado = {"respuesta_modelo": str(resultado)}

        cursor.execute(
            """
            UPDATE cv_documents
            SET status = 'ANALYZED',
                extracted_text = %s,
                parsed_skills = %s,
                analyzed_at = NOW(),
                error_message = NULL
            WHERE id = %s
            """,
            (
                texto_cv,
                _json_for_db(_analysis_array(resultado, "tecnologias")),
                cv_document_id
            )
        )

        cursor.execute(
            """
            UPDATE cv_ai_summaries
            SET is_latest = 0
            WHERE engineer_user_id = %s
            """,
            (payload["user_id"],)
        )

        cursor.execute(
            """
            INSERT INTO cv_ai_summaries
            (
                cv_document_id,
                engineer_user_id,
                summary,
                profile_title,
                overall_score,
                strengths,
                weak_points,
                detected_skills,
                suggested_roles,
                training_suggestions,
                model_name,
                prompt_version,
                raw_response,
                is_latest
            )
            VALUES
            (
                %s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,1
            )
            """,
            (
                cv_document_id,
                payload["user_id"],
                resultado.get("resumen") or resultado.get("summary") or "Análisis generado por IA",
                resultado.get("seniority"),
                _analysis_score(resultado),
                _json_for_db(_analysis_array(resultado, "fortalezas")),
                _json_for_db(_analysis_array(resultado, "debilidades")),
                _json_for_db(_analysis_array(resultado, "tecnologias")),
                _json_for_db(_analysis_array(resultado, "roles_sugeridos")),
                _json_for_db(_analysis_array(resultado, "recomendaciones_capacitacion")),
                os.getenv("OPENAI_MODEL", "gpt-5.4-mini"),
                "cv_analyzer_v1",
                _json_for_db(resultado),
                
            )
        )
        analysis_id = cursor.lastrowid

        conn.commit()

        return {
            "message": "CV analizado correctamente",
            "cv_document_id": cv_document_id,
            "analysis_id": analysis_id,
            "file_name": unique_name,
            "original_file_name": safe_filename,
            "sha256": file_hash,
            "puesto": puesto,
            "analysis": resultado,
            "preview_texto": texto_cv[:1000]
        }

    except HTTPException:
        raise
    except Exception as exc:
        if cv_document_id:
            try:
                cursor = conn.cursor()
                cursor.execute(
                    """
                    UPDATE cv_documents
                    SET status = 'FAILED',
                        error_message = %s
                    WHERE id = %s
                    """,
                    (str(exc), cv_document_id)
                )
                conn.commit()
            except Exception:
                pass

        raise HTTPException(
            status_code=500,
            detail=f"Error analizando CV: {str(exc)}"
        )
    finally:
        conn.close()


@app.get("/cv/my")
def my_cvs(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    if payload.get("role") != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )

    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                id,
                file_name,
                original_file_name,
                status,
                is_active,
                uploaded_at,
                analyzed_at,
                error_message
            FROM cv_documents
            WHERE engineer_user_id = %s
            ORDER BY uploaded_at DESC
            """,
            (payload["user_id"],)
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/cv/my/latest-analysis")
def my_latest_cv_analysis(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    if payload.get("role") != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )

    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                s.id AS analysis_id,
                s.cv_document_id,
                d.file_name,
                d.original_file_name,
                d.status AS cv_status,
                s.summary,
                s.profile_title,
                s.overall_score,
                s.strengths,
                s.weak_points,
                s.detected_skills,
                s.suggested_roles,
                s.training_suggestions,
                s.raw_response,
                s.created_at
            FROM cv_ai_summaries s
            INNER JOIN cv_documents d
                ON d.id = s.cv_document_id
            WHERE s.engineer_user_id = %s
            AND s.is_latest = 1
            ORDER BY s.created_at DESC
            LIMIT 1
            """,
            (payload["user_id"],)
        )
        row = cursor.fetchone()
        if not row:
            raise HTTPException(
                status_code=404,
                detail="No hay análisis de CV todavía"
            )
        return row
    finally:
        conn.close()


@app.post("/jobs/create")
def create_job(
    request: Request,
    data: JobPostRequest
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Empresa":
        raise HTTPException(
            status_code=403,
            detail="Solo empresas"
        )
    conn = get_db()
    try:
        required_skills_snapshot = json.dumps(data.required_skills or [])
        safe_location_mode = normalize_location_mode(data.location_mode)
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO job_posts
            (
                company_user_id,
                title,
                description,
                required_skills_snapshot,
                min_years_experience,
                location_mode,
                city,
                country
            )
            VALUES
            (
                %s,%s,%s,%s,%s,%s,%s,%s
            )
            """,
            (
                payload["user_id"],
                data.title,
                data.description,
                required_skills_snapshot,
                data.min_years_experience,
                safe_location_mode,
                data.city,
                data.country
            )
        )
        conn.commit()
        return {
            "message": "Oferta laboral creada"
        }
    finally:

        conn.close()

@app.post("/applications/apply")
def apply_to_job(
    request: Request,
    data: ApplicationRequest
):

    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT id
            FROM applications
            WHERE engineer_user_id = %s
            AND job_post_id = %s
            LIMIT 1
            """,
            (
                payload["user_id"],
                data.job_post_id
            )
        )
        existing = cursor.fetchone()
        if existing:
            raise HTTPException(
                status_code=400,
                detail="Ya aplicaste a esta oferta"
            )
        cursor.execute(
            """
            SELECT
                j.id,
                j.company_user_id,
                j.title AS job_title,
                COALESCE(cp.company_name, company.display_name, company.username) AS company_name
            FROM job_posts j
            LEFT JOIN users company
                ON company.id = j.company_user_id
            LEFT JOIN company_profiles cp
                ON cp.user_id = company.id
            WHERE j.id = %s
            LIMIT 1
            """,
            (data.job_post_id,)
        )
        job = cursor.fetchone()
        if not job:
            raise HTTPException(
                status_code=404,
                detail="La oferta laboral no existe"
            )

        cursor.execute(
            """
            SELECT
                id,
                COALESCE(display_name, username) AS candidate_name,
                email
            FROM users
            WHERE id = %s
            LIMIT 1
            """,
            (payload["user_id"],)
        )
        candidate = cursor.fetchone() or {}

        cursor.execute(
            """
            SELECT id
            FROM cv_documents
            WHERE engineer_user_id = %s
            AND is_active = 1
            ORDER BY uploaded_at DESC
            LIMIT 1
            """,
            (payload["user_id"],)
        )
        active_cv = cursor.fetchone()
        active_cv_id = active_cv["id"] if active_cv else None

        cursor.execute(
            """
            INSERT INTO applications
            (
                engineer_user_id,
                job_post_id,
                company_user_id,
                cv_document_id
            )
            VALUES
            (
                %s,%s,%s,%s
            )
            """,
            (
                payload["user_id"],
                data.job_post_id,
                job["company_user_id"],
                active_cv_id
            )
        )
        application_id = cursor.lastrowid

        cursor.execute(
            """
            INSERT INTO notifications
            (
                user_id,
                type,
                title,
                body,
                related_job_post_id,
                related_application_id,
                metadata,
                priority
            )
            VALUES
            (
                %s,'SYSTEM',%s,%s,%s,%s,%s,'HIGH'
            )
            """,
            (
                job["company_user_id"],
                "Nueva postulación recibida",
                f"{candidate.get('candidate_name') or 'Un candidato'} se postuló al puesto {job.get('job_title') or data.job_post_id}.",
                data.job_post_id,
                application_id,
                _json_text(
                    {
                        "event": "new_application",
                        "candidate_user_id": payload["user_id"],
                        "candidate_name": candidate.get("candidate_name"),
                        "candidate_email": candidate.get("email"),
                        "job_title": job.get("job_title"),
                        "cv_document_id": active_cv_id
                    }
                )
            )
        )

        conn.commit()
        return {
            "message": "Postulacion enviada",
            "application_id": application_id,
            "cv_document_id": active_cv_id
        }
    finally:
        conn.close()

@app.post("/skills/create")
def create_skill(
    request: Request,
    data: SkillRequest
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Admin":
        raise HTTPException(
            status_code=403,
            detail="Solo administradores"
        )
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )

        cursor.execute(
            """
            SELECT id
            FROM skills
            WHERE name = %s
            LIMIT 1
            """,
            (data.name,)
        )
        existing = cursor.fetchone()
        if existing:
            raise HTTPException(
                status_code=400,
                detail="Skill ya existe"
            )
        cursor.execute(
            """
            INSERT INTO skills
            (
                name
            )
            VALUES
            (
                %s
            )
            """,
            (data.name,)
        )
        conn.commit()
        return {
            "message": "Skill creada"
        }
    finally:
        conn.close()

@app.post("/users/skills/add")
def add_user_skill(
    request: Request,
    data: UserSkillRequest
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT id
            FROM user_skills
            WHERE user_id = %s
            AND skill_id = %s
            LIMIT 1
            """,
            (
                payload["user_id"],
                data.skill_id
            )
        )
        existing = cursor.fetchone()
        if existing:
            raise HTTPException(
                status_code=400,
                detail="Skill ya agregada"
            )
        cursor.execute(
            """
            INSERT INTO user_skills
            (
                user_id,
                skill_id
            )
            VALUES
            (
                %s,%s
            )
            """,
            (
                payload["user_id"],
                data.skill_id
            )
        )
        conn.commit()
        return {
            "message": "Skill agregada al usuario"
        }
    finally:

        conn.close()

@app.post("/jobs/skills/add")
def add_job_skill(
    request: Request,
    data: JobSkillRequest
):
    token = get_token(request)
    payload = decode_token(token)
    if payload["role"] != "Empresa":
        raise HTTPException(
            status_code=403,
            detail="Solo empresas"
        )
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            INSERT INTO job_post_skills
            (
                job_post_id,
                skill_id
            )
            VALUES
            (
                %s,%s
            )
            """,
            (
                data.job_post_id,
                data.skill_id
            )
        )
        conn.commit()
        return {
            "message": "Skill agregada a oferta"
        }
    finally:
        conn.close()

@app.get("/jobs/list")
def list_jobs():
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT *, required_skills_snapshot AS required_skills
            FROM job_posts
            ORDER BY created_at DESC
            """
        )
        jobs = cursor.fetchall()
        return jobs
    finally:
        conn.close()

@app.get("/applications/my")
def my_applications(request: Request):
    token = get_token(request)
    payload = decode_token(token)

    if payload.get("role") != "Usuario":
        raise HTTPException(
            status_code=403,
            detail="Solo usuarios"
        )

    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                a.id,
                a.job_post_id,
                a.company_user_id,
                a.cv_document_id,
                a.status,
                a.candidate_message,
                a.company_message,
                a.rejection_reason,
                a.applied_at,
                a.reviewed_at,
                a.selected_at,
                a.rejected_at,
                a.updated_at,
                j.title AS job_title,
                j.description AS job_description,
                j.location_mode,
                j.city,
                j.country,
                COALESCE(cp.company_name, company.display_name) AS company_name,
                cv.file_name AS cv_file_name,
                cv.status AS cv_status,
                cv.uploaded_at AS cv_uploaded_at
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            LEFT JOIN users company
                ON company.id = a.company_user_id
            LEFT JOIN company_profiles cp
                ON cp.user_id = company.id
            LEFT JOIN cv_documents cv
                ON cv.id = a.cv_document_id
            WHERE a.engineer_user_id = %s
            ORDER BY a.applied_at DESC, a.id DESC
            """,
            (payload["user_id"],)
        )
        applications = cursor.fetchall()
        return applications
    finally:
        conn.close()


@app.get("/company/jobs/my")
def company_jobs_my(request: Request):
    payload = require_company(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                j.*,
                j.required_skills_snapshot AS required_skills,
                COUNT(a.id) AS applications_count,
                SUM(CASE WHEN a.status = 'Seleccionada' THEN 1 ELSE 0 END) AS selected_count,
                SUM(CASE WHEN a.status = 'Rechazada' THEN 1 ELSE 0 END) AS rejected_count
            FROM job_posts j
            LEFT JOIN applications a
                ON a.job_post_id = j.id
            WHERE j.company_user_id = %s
            GROUP BY j.id
            ORDER BY j.created_at DESC, j.id DESC
            """,
            (payload["user_id"],)
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/company/applications/my")
def my_company_applications(request: Request):
    payload = require_company(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                a.id AS application_id,
                a.id,
                a.job_post_id,
                a.engineer_user_id,
                a.company_user_id,
                a.cv_document_id,
                a.status,
                a.candidate_message,
                a.company_message,
                a.rejection_reason,
                a.applied_at,
                a.reviewed_at,
                a.selected_at,
                a.rejected_at,
                a.updated_at,
                j.title AS job_title,
                j.description AS job_description,
                j.required_skills_snapshot AS required_skills,
                j.seniority AS job_seniority,
                j.min_years_experience,
                candidate.username AS candidate_username,
                candidate.email AS candidate_email,
                candidate.display_name AS candidate_name,
                ep.first_name,
                ep.last_name,
                ep.age,
                ep.phone AS candidate_phone,
                ep.country AS candidate_country,
                ep.main_programming_language,
                cv.file_name AS cv_file_name,
                cv.original_file_name AS cv_original_file_name,
                cv.status AS cv_status,
                cv.uploaded_at AS cv_uploaded_at,
                cv.analyzed_at AS cv_analyzed_at,
                cv.extracted_text AS cv_extracted_text,
                cvs.summary AS cv_summary,
                cvs.overall_score AS cv_overall_score,
                cvs.strengths AS cv_strengths,
                cvs.weak_points AS cv_weak_points,
                cvs.detected_skills AS cv_detected_skills,
                cvs.training_suggestions AS cv_training_suggestions,
                eval.score AS latest_rank_score,
                eval.recommendation AS latest_recommendation,
                eval.summary_for_company AS latest_company_summary
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            INNER JOIN users candidate
                ON candidate.id = a.engineer_user_id
            LEFT JOIN engineer_profiles ep
                ON ep.user_id = candidate.id
            LEFT JOIN cv_documents cv
                ON cv.id = a.cv_document_id
            LEFT JOIN cv_ai_summaries cvs
                ON cvs.cv_document_id = cv.id
                AND cvs.is_latest = 1
            LEFT JOIN ai_evaluations eval
                ON eval.application_id = a.id
                AND eval.is_latest = 1
            WHERE a.company_user_id = %s
            ORDER BY a.applied_at DESC, a.id DESC
            """,
            (payload["user_id"],)
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/company/jobs/{job_id}/applications")
def company_job_applications(job_id: int, request: Request):
    payload = require_company(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        cursor.execute(
            """
            SELECT id, title, description, required_skills_snapshot AS required_skills
            FROM job_posts
            WHERE id = %s
              AND company_user_id = %s
            LIMIT 1
            """,
            (job_id, payload["user_id"])
        )
        job = cursor.fetchone()
        if not job:
            raise HTTPException(
                status_code=404,
                detail="Publicación no encontrada para esta empresa"
            )

        cursor.execute(
            """
            SELECT
                a.id AS application_id,
                a.id,
                a.job_post_id,
                a.engineer_user_id,
                a.company_user_id,
                a.cv_document_id,
                a.status,
                a.candidate_message,
                a.company_message,
                a.rejection_reason,
                a.applied_at,
                a.reviewed_at,
                a.selected_at,
                a.rejected_at,
                a.updated_at,
                j.title AS job_title,
                j.description AS job_description,
                j.required_skills_snapshot AS required_skills,
                candidate.username AS candidate_username,
                candidate.email AS candidate_email,
                candidate.display_name AS candidate_name,
                ep.first_name,
                ep.last_name,
                ep.age,
                ep.phone AS candidate_phone,
                ep.country AS candidate_country,
                ep.main_programming_language,
                cv.file_name AS cv_file_name,
                cv.original_file_name AS cv_original_file_name,
                cv.status AS cv_status,
                cv.uploaded_at AS cv_uploaded_at,
                cv.analyzed_at AS cv_analyzed_at,
                cv.extracted_text AS cv_extracted_text,
                cvs.summary AS cv_summary,
                cvs.overall_score AS cv_overall_score,
                cvs.strengths AS cv_strengths,
                cvs.weak_points AS cv_weak_points,
                cvs.detected_skills AS cv_detected_skills,
                cvs.training_suggestions AS cv_training_suggestions,
                eval.score AS latest_rank_score,
                eval.recommendation AS latest_recommendation,
                eval.summary_for_company AS latest_company_summary
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            INNER JOIN users candidate
                ON candidate.id = a.engineer_user_id
            LEFT JOIN engineer_profiles ep
                ON ep.user_id = candidate.id
            LEFT JOIN cv_documents cv
                ON cv.id = a.cv_document_id
            LEFT JOIN cv_ai_summaries cvs
                ON cvs.cv_document_id = cv.id
                AND cvs.is_latest = 1
            LEFT JOIN ai_evaluations eval
                ON eval.application_id = a.id
                AND eval.is_latest = 1
            WHERE a.company_user_id = %s
              AND a.job_post_id = %s
            ORDER BY a.applied_at DESC, a.id DESC
            """,
            (payload["user_id"], job_id)
        )
        return {
            "job": job,
            "applications": cursor.fetchall()
        }
    finally:
        conn.close()


@app.post("/company/jobs/{job_id}/rank-candidates")
def company_rank_candidates(job_id: int, request: Request):
    payload = require_company(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        cursor.execute(
            """
            SELECT
                id,
                title,
                description,
                required_skills_snapshot AS required_skills,
                seniority,
                min_years_experience,
                location_mode,
                city,
                country
            FROM job_posts
            WHERE id = %s
              AND company_user_id = %s
            LIMIT 1
            """,
            (job_id, payload["user_id"])
        )
        job = cursor.fetchone()

        if not job:
            raise HTTPException(
                status_code=404,
                detail="Publicación no encontrada para esta empresa"
            )

        cursor.execute(
            """
            SELECT
                a.id AS application_id,
                a.job_post_id,
                a.engineer_user_id,
                a.company_user_id,
                a.cv_document_id,
                a.status,
                j.title AS job_title,
                candidate.display_name AS candidate_name,
                candidate.email AS candidate_email,
                cv.file_name AS cv_file_name,
                cvs.summary AS cv_summary,
                cvs.overall_score AS cv_overall_score,
                cvs.strengths AS cv_strengths,
                cvs.weak_points AS cv_weak_points,
                cvs.detected_skills AS cv_detected_skills,
                cvs.training_suggestions AS cv_training_suggestions
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            INNER JOIN users candidate
                ON candidate.id = a.engineer_user_id
            LEFT JOIN cv_documents cv
                ON cv.id = a.cv_document_id
            LEFT JOIN cv_ai_summaries cvs
                ON cvs.cv_document_id = cv.id
                AND cvs.is_latest = 1
            WHERE a.company_user_id = %s
              AND a.job_post_id = %s
            ORDER BY a.applied_at DESC, a.id DESC
            """,
            (payload["user_id"], job_id)
        )

        rows = cursor.fetchall()

        if not rows:
            return {
                "job": job,
                "ranking": [],
                "model_name": None,
                "fallback": False,
                "message": "La publicación todavía no tiene postulantes"
            }

        fallback_items = [
            _candidate_row_to_ranking_item(row, index + 1)
            for index, row in enumerate(rows)
        ]
        fallback_items.sort(key=lambda item: item["score"], reverse=True)
        for index, item in enumerate(fallback_items):
            item["position"] = index + 1

        model_name = get_ai_setting(
            conn,
            "AI_MODEL_ACTIVE",
            os.getenv("OPENAI_MODEL", "gpt-5.4-mini")
        )

        ranking = fallback_items
        fallback = False
        raw_response = None

        try:
            ai_ranking, raw_response = _rank_candidates_with_openai(
                model_name,
                job,
                fallback_items
            )
            normalized = []
            for index, item in enumerate(ai_ranking):
                if not isinstance(item, dict):
                    continue
                normalized.append({
                    "position": index + 1,
                    "application_id": item.get("application_id"),
                    "engineer_user_id": item.get("engineer_user_id"),
                    "candidate_name": item.get("candidate_name"),
                    "candidate_email": item.get("candidate_email"),
                    "score": float(item.get("score", 0) or 0),
                    "recommendation": item.get("recommendation") or "RecomendadaConCapacitacion",
                    "summary_for_company": item.get("summary_for_company") or item.get("summary") or "",
                    "strengths": item.get("strengths") or [],
                    "missing_skills": item.get("missing_skills") or [],
                    "training_suggestions": item.get("training_suggestions") or [],
                    "status": item.get("status"),
                    "job_post_id": job_id
                })
            if normalized:
                ranking = sorted(
                    normalized,
                    key=lambda item: item.get("score", 0),
                    reverse=True
                )
                for index, item in enumerate(ranking):
                    item["position"] = index + 1
        except Exception as exc:
            fallback = True
            raw_response = str(exc)

        cursor.execute(
            """
            UPDATE ai_evaluations
            SET is_latest = 0
            WHERE company_user_id = %s
              AND job_post_id = %s
            """,
            (payload["user_id"], job_id)
        )

        app_to_row = {
            int(row["application_id"]): row
            for row in rows
            if row.get("application_id") is not None
        }

        for item in ranking:
            app_id = item.get("application_id")
            try:
                app_id_int = int(app_id)
            except Exception:
                continue

            original = app_to_row.get(app_id_int)
            if not original:
                continue

            cursor.execute(
                """
                INSERT INTO ai_evaluations
                (
                    application_id,
                    engineer_user_id,
                    company_user_id,
                    job_post_id,
                    cv_document_id,
                    model_name,
                    prompt_version,
                    score,
                    recommendation,
                    strengths,
                    missing_skills,
                    training_suggestions,
                    summary_for_company,
                    raw_response,
                    is_latest
                )
                VALUES
                (
                    %s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,1
                )
                """,
                (
                    app_id_int,
                    original.get("engineer_user_id"),
                    payload["user_id"],
                    job_id,
                    original.get("cv_document_id"),
                    model_name,
                    "company_candidate_ranking_v1",
                    item.get("score"),
                    item.get("recommendation") or "RecomendadaConCapacitacion",
                    _json_text(item.get("strengths") or []),
                    _json_text(item.get("missing_skills") or []),
                    _json_text(item.get("training_suggestions") or []),
                    item.get("summary_for_company") or "",
                    raw_response if raw_response else _json_text(item),
                )
            )

        conn.commit()

        return {
            "job": job,
            "ranking": ranking,
            "model_name": model_name,
            "fallback": fallback,
            "message": (
                "Ranking generado con IA"
                if not fallback
                else "Ranking generado con fallback local porque OpenAI no respondió correctamente"
            )
        }

    finally:
        conn.close()


@app.put("/company/applications/{application_id}/status")
def company_update_application_status(
    application_id: int,
    request: Request,
    data: CompanyApplicationStatusRequest
):
    payload = require_company(request)
    new_status = _normalize_application_status(data.status)
    message = (data.message or "").strip() or None

    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        cursor.execute(
            """
            SELECT
                a.*,
                j.title AS job_title,
                candidate.display_name AS candidate_name,
                COALESCE(cp.company_name, company.display_name, company.username) AS company_name
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            INNER JOIN users candidate
                ON candidate.id = a.engineer_user_id
            LEFT JOIN users company
                ON company.id = a.company_user_id
            LEFT JOIN company_profiles cp
                ON cp.user_id = company.id
            WHERE a.id = %s
              AND a.company_user_id = %s
            LIMIT 1
            """,
            (application_id, payload["user_id"])
        )
        application = cursor.fetchone()

        if not application:
            raise HTTPException(
                status_code=404,
                detail="Postulación no encontrada para esta empresa"
            )

        old_status = application.get("status")

        cursor.execute(
            """
            UPDATE applications
            SET status = %s,
                company_message = %s,
                reviewed_at = CASE WHEN %s IN ('EnRevision','DatosSolicitados','ReunionSolicitada') THEN NOW() ELSE reviewed_at END,
                selected_at = CASE WHEN %s = 'Seleccionada' THEN NOW() ELSE selected_at END,
                rejected_at = CASE WHEN %s = 'Rechazada' THEN NOW() ELSE rejected_at END,
                rejection_reason = CASE WHEN %s = 'Rechazada' THEN %s ELSE rejection_reason END
            WHERE id = %s
              AND company_user_id = %s
            """,
            (
                new_status,
                message,
                new_status,
                new_status,
                new_status,
                new_status,
                message,
                application_id,
                payload["user_id"]
            )
        )

        cursor.execute(
            """
            INSERT INTO application_status_history
            (
                application_id,
                old_status,
                new_status,
                changed_by_user_id,
                note
            )
            VALUES
            (
                %s,%s,%s,%s,%s
            )
            """,
            (
                application_id,
                old_status,
                new_status,
                payload["user_id"],
                message
            )
        )

        notif_type, notif_title = _candidate_status_notification(new_status)
        company_name = application.get("company_name") or "La empresa"
        job_title = application.get("job_title") or "el puesto"
        if message:
            body = f"{company_name} actualizó tu postulación para {job_title}: {message}"
        else:
            body = f"{company_name} actualizó tu postulación para el puesto {job_title}."

        cursor.execute(
            """
            INSERT INTO notifications
            (
                user_id,
                type,
                title,
                body,
                related_job_post_id,
                related_application_id,
                metadata,
                priority
            )
            VALUES
            (
                %s,%s,%s,%s,%s,%s,%s,'HIGH'
            )
            """,
            (
                application["engineer_user_id"],
                notif_type,
                notif_title,
                body,
                application["job_post_id"],
                application_id,
                _json_text(
                    {
                        "old_status": old_status,
                        "new_status": new_status,
                        "company_user_id": payload["user_id"]
                    }
                )
            )
        )

        conn.commit()

        return {
            "message": "Estado de postulación actualizado",
            "application_id": application_id,
            "old_status": old_status,
            "new_status": new_status
        }

    finally:
        conn.close()


@app.post("/company/applications/{application_id}/request")
def company_create_candidate_request(
    application_id: int,
    request: Request,
    data: CompanyCandidateRequestPayload
):
    payload = require_company(request)
    request_type = _normalize_candidate_request_type(data.request_type)

    title = (data.title or "").strip()
    if not title:
        default_titles = {
            "UPLOAD_CV": "Solicitud de actualización de CV",
            "SCHEDULE_MEETING": "Solicitud de reunión",
            "REQUEST_DOCUMENTS": "Solicitud de documentación",
            "CUSTOM_MESSAGE": "Mensaje de la empresa"
        }
        title = default_titles[request_type]

    details = (data.details or "").strip() or None
    meeting_at = (data.meeting_at or "").strip() or None

    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        cursor.execute(
            """
            SELECT
                a.*,
                j.title AS job_title,
                candidate.display_name AS candidate_name,
                COALESCE(cp.company_name, company.display_name, company.username) AS company_name
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            INNER JOIN users candidate
                ON candidate.id = a.engineer_user_id
            LEFT JOIN users company
                ON company.id = a.company_user_id
            LEFT JOIN company_profiles cp
                ON cp.user_id = company.id
            WHERE a.id = %s
              AND a.company_user_id = %s
            LIMIT 1
            """,
            (application_id, payload["user_id"])
        )
        application = cursor.fetchone()

        if not application:
            raise HTTPException(
                status_code=404,
                detail="Postulación no encontrada para esta empresa"
            )

        cursor.execute(
            """
            INSERT INTO company_candidate_requests
            (
                company_user_id,
                engineer_user_id,
                job_post_id,
                application_id,
                request_type,
                title,
                details,
                meeting_at
            )
            VALUES
            (
                %s,%s,%s,%s,%s,%s,%s,%s
            )
            """,
            (
                payload["user_id"],
                application["engineer_user_id"],
                application["job_post_id"],
                application_id,
                request_type,
                title,
                details,
                meeting_at
            )
        )
        request_id = cursor.lastrowid

        if request_type == "UPLOAD_CV":
            next_status = "DatosSolicitados"
            notification_type = "COMPANY_REQUEST"
        elif request_type == "SCHEDULE_MEETING":
            next_status = "ReunionSolicitada"
            notification_type = "MEETING_PROPOSED"
        else:
            next_status = "DatosSolicitados"
            notification_type = "COMPANY_REQUEST"

        cursor.execute(
            """
            UPDATE applications
            SET status = %s,
                company_message = %s,
                reviewed_at = NOW()
            WHERE id = %s
            """,
            (
                next_status,
                details,
                application_id
            )
        )

        cursor.execute(
            """
            INSERT INTO application_status_history
            (
                application_id,
                old_status,
                new_status,
                changed_by_user_id,
                note
            )
            VALUES
            (
                %s,%s,%s,%s,%s
            )
            """,
            (
                application_id,
                application["status"],
                next_status,
                payload["user_id"],
                title
            )
        )

        cursor.execute(
            """
            INSERT INTO notifications
            (
                user_id,
                type,
                title,
                body,
                related_job_post_id,
                related_application_id,
                related_request_id,
                metadata,
                priority
            )
            VALUES
            (
                %s,%s,%s,%s,%s,%s,%s,%s,'HIGH'
            )
            """,
            (
                application["engineer_user_id"],
                notification_type,
                title,
                details or f"{application.get('company_name') or 'La empresa'} envió una solicitud relacionada al puesto {application.get('job_title')}.",
                application["job_post_id"],
                application_id,
                request_id,
                _json_text(
                    {
                        "request_type": request_type,
                        "meeting_at": meeting_at,
                        "company_user_id": payload["user_id"]
                    }
                )
            )
        )

        conn.commit()

        return {
            "message": "Solicitud enviada al candidato",
            "request_id": request_id,
            "application_id": application_id,
            "next_status": next_status
        }

    finally:
        conn.close()


@app.get("/admin/dashboard")
def admin_dashboard(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        cursor.execute(
            """
            SELECT COUNT(*) AS total
            FROM users
            WHERE role = 'Usuario'
            AND deleted_at IS NULL
            """
        )
        users_count = cursor.fetchone()["total"]

        cursor.execute(
            """
            SELECT COUNT(*) AS total
            FROM users
            WHERE role = 'Empresa'
            AND deleted_at IS NULL
            """
        )
        companies_count = cursor.fetchone()["total"]

        cursor.execute(
            """
            SELECT COUNT(*) AS total
            FROM users
            WHERE is_online = 1
            AND deleted_at IS NULL
            """
        )
        online_count = cursor.fetchone()["total"]

        cursor.execute(
            """
            SELECT COUNT(*) AS total
            FROM applications
            WHERE status = 'Seleccionada'
            AND selected_at IS NOT NULL
            AND selected_at >= DATE_SUB(UTC_TIMESTAMP(), INTERVAL 30 DAY)
            """
        )
        accepted_count = cursor.fetchone()["total"]

        return {
            "registered_users": users_count,
            "registered_companies": companies_count,
            "online_users": online_count,
            "accepted_last_30_days": accepted_count,
        }
    finally:
        conn.close()

@app.get("/admin/users")
def admin_users(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                u.id,
                u.username,
                u.role,
                u.email,
                u.display_name,
                u.is_online,
                u.last_login,
                cv.file_name AS active_cv
            FROM users u
            LEFT JOIN cv_documents cv
                ON cv.engineer_user_id = u.id
                AND cv.is_active = 1
            WHERE u.role = 'Usuario'
            AND u.deleted_at IS NULL
            ORDER BY u.created_at DESC, cv.uploaded_at DESC
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()

@app.get("/admin/companies")
def admin_companies(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                u.id,
                COALESCE(cp.company_name, u.display_name) AS company_name,
                u.email,
                COUNT(j.id) AS active_jobs
            FROM users u
            LEFT JOIN company_profiles cp
                ON cp.user_id = u.id
            LEFT JOIN job_posts j
                ON j.company_user_id = u.id
                AND j.status = 'Abierta'
            WHERE u.role = 'Empresa'
            AND u.deleted_at IS NULL
            GROUP BY u.id, cp.company_name, u.display_name, u.email
            ORDER BY u.created_at DESC
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()

@app.get("/admin/accepted")
def admin_accepted(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                a.id,
                COALESCE(cp.company_name, company.display_name) AS company_name,
                j.title AS job_title,
                engineer.display_name AS engineer_name,
                a.selected_at
            FROM applications a
            INNER JOIN job_posts j
                ON j.id = a.job_post_id
            INNER JOIN users engineer
                ON engineer.id = a.engineer_user_id
            LEFT JOIN users company
                ON company.id = a.company_user_id
            LEFT JOIN company_profiles cp
                ON cp.user_id = company.id
            WHERE a.status = 'Seleccionada'
            ORDER BY a.selected_at DESC, a.id DESC
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()

@app.get("/admin/ai/settings")
def admin_ai_settings(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT setting_key, setting_value, setting_group, description, updated_at
            FROM admin_ai_settings
            ORDER BY setting_group ASC, setting_key ASC
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()

@app.put("/admin/ai/settings")
def update_admin_ai_settings(
    request: Request,
    data: AdminAiSettingUpdateRequest
):
    payload = require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT setting_key
            FROM admin_ai_settings
            WHERE setting_key = %s
            LIMIT 1
            """,
            (data.setting_key,)
        )
        existing = cursor.fetchone()

        if existing:
            cursor = conn.cursor()
            cursor.execute(
                """
                UPDATE admin_ai_settings
                SET setting_value = %s,
                    updated_by = %s
                WHERE setting_key = %s
                """,
                (data.setting_value, payload["user_id"], data.setting_key)
            )
        else:
            cursor = conn.cursor()
            cursor.execute(
                """
                INSERT INTO admin_ai_settings
                (
                    setting_key,
                    setting_value,
                    setting_group,
                    description,
                    updated_by
                )
                VALUES
                (
                    %s,%s,'GENERAL','Creada desde panel admin',%s
                )
                """,
                (data.setting_key, data.setting_value, payload["user_id"])
            )

        conn.commit()
        return {
            "message": "Configuracion actualizada"
        }
    finally:
        conn.close()

@app.get("/admin/system/summary")
def admin_system_summary(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)

        def scalar(query, params=()):
            cursor.execute(query, params)
            row = cursor.fetchone() or {"total": 0}
            return row.get("total", 0)

        cursor.execute(
            """
            SELECT role, COUNT(*) AS total
            FROM users
            WHERE deleted_at IS NULL
            GROUP BY role
            ORDER BY role
            """
        )
        users_by_role = cursor.fetchall()

        cursor.execute(
            """
            SELECT status, COUNT(*) AS total
            FROM job_posts
            GROUP BY status
            ORDER BY status
            """
        )
        jobs_by_status = cursor.fetchall()

        cursor.execute(
            """
            SELECT status, COUNT(*) AS total
            FROM applications
            GROUP BY status
            ORDER BY status
            """
        )
        applications_by_status = cursor.fetchall()

        cursor.execute(
            """
            SELECT status, COUNT(*) AS total
            FROM cv_documents
            GROUP BY status
            ORDER BY status
            """
        )
        cvs_by_status = cursor.fetchall()

        metrics = {
            "total_users": scalar("SELECT COUNT(*) AS total FROM users WHERE deleted_at IS NULL"),
            "active_users": scalar("SELECT COUNT(*) AS total FROM users WHERE is_active = 1 AND deleted_at IS NULL"),
            "online_users": scalar("SELECT COUNT(*) AS total FROM users WHERE is_online = 1 AND deleted_at IS NULL"),
            "total_companies": scalar("SELECT COUNT(*) AS total FROM users WHERE role = 'Empresa' AND deleted_at IS NULL"),
            "total_engineers": scalar("SELECT COUNT(*) AS total FROM users WHERE role = 'Usuario' AND deleted_at IS NULL"),
            "total_jobs": scalar("SELECT COUNT(*) AS total FROM job_posts"),
            "open_jobs": scalar("SELECT COUNT(*) AS total FROM job_posts WHERE status = 'Abierta'"),
            "total_applications": scalar("SELECT COUNT(*) AS total FROM applications"),
            "selected_applications": scalar("SELECT COUNT(*) AS total FROM applications WHERE status = 'Seleccionada'"),
            "rejected_applications": scalar("SELECT COUNT(*) AS total FROM applications WHERE status = 'Rechazada'"),
            "total_cvs": scalar("SELECT COUNT(*) AS total FROM cv_documents"),
            "analyzed_cvs": scalar("SELECT COUNT(*) AS total FROM cv_documents WHERE status = 'ANALYZED'"),
            "failed_cvs": scalar("SELECT COUNT(*) AS total FROM cv_documents WHERE status = 'FAILED'"),
            "total_ai_summaries": scalar("SELECT COUNT(*) AS total FROM cv_ai_summaries"),
            "total_ai_evaluations": scalar("SELECT COUNT(*) AS total FROM ai_evaluations"),
            "total_notifications": scalar("SELECT COUNT(*) AS total FROM notifications"),
            "unread_notifications": scalar("SELECT COUNT(*) AS total FROM notifications WHERE is_read = 0"),
            "total_chat_conversations": scalar("SELECT COUNT(*) AS total FROM ai_conversations"),
            "total_chat_messages": scalar("SELECT COUNT(*) AS total FROM ai_messages"),
            "total_company_requests": scalar("SELECT COUNT(*) AS total FROM company_candidate_requests")
        }

        return {
            "metrics": metrics,
            "users_by_role": users_by_role,
            "jobs_by_status": jobs_by_status,
            "applications_by_status": applications_by_status,
            "cvs_by_status": cvs_by_status
        }
    finally:
        conn.close()


@app.get("/admin/users/full")
def admin_users_full(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                u.id,
                u.username,
                u.role,
                u.email,
                u.display_name,
                u.is_active,
                u.is_online,
                u.last_login,
                u.created_at,
                ep.first_name,
                ep.last_name,
                ep.age,
                ep.phone,
                ep.country,
                ep.main_programming_language,
                cp.company_name,
                cp.contact_phone,
                (SELECT COUNT(*) FROM cv_documents cv WHERE cv.engineer_user_id = u.id) AS cv_count,
                (SELECT COUNT(*) FROM applications a WHERE a.engineer_user_id = u.id) AS applications_count,
                (SELECT COUNT(*) FROM job_posts j WHERE j.company_user_id = u.id) AS jobs_count,
                (SELECT COUNT(*) FROM notifications n WHERE n.user_id = u.id AND n.is_read = 0) AS unread_notifications
            FROM users u
            LEFT JOIN engineer_profiles ep ON ep.user_id = u.id
            LEFT JOIN company_profiles cp ON cp.user_id = u.id
            WHERE u.deleted_at IS NULL
            ORDER BY u.created_at DESC, u.id DESC
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/admin/users/{user_id}/detail")
def admin_user_detail(user_id: int, request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT id, role, username, email, display_name, is_active, is_online,
                   last_login, last_seen_at, created_at, updated_at
            FROM users
            WHERE id = %s AND deleted_at IS NULL
            LIMIT 1
            """,
            (user_id,)
        )
        user = cursor.fetchone()
        if not user:
            raise HTTPException(status_code=404, detail="Usuario no encontrado")

        cursor.execute("SELECT * FROM engineer_profiles WHERE user_id = %s LIMIT 1", (user_id,))
        engineer_profile = cursor.fetchone()

        cursor.execute("SELECT * FROM company_profiles WHERE user_id = %s LIMIT 1", (user_id,))
        company_profile = cursor.fetchone()

        cursor.execute(
            """
            SELECT cv.id, cv.file_name, cv.original_file_name, cv.status, cv.is_active,
                   cv.uploaded_at, cv.analyzed_at, cv.error_message,
                   ai.id AS analysis_id, ai.summary, ai.overall_score, ai.strengths,
                   ai.weak_points, ai.detected_skills, ai.suggested_roles,
                   ai.training_suggestions, ai.model_name, ai.created_at AS analysis_created_at
            FROM cv_documents cv
            LEFT JOIN cv_ai_summaries ai
              ON ai.cv_document_id = cv.id
             AND ai.is_latest = 1
            WHERE cv.engineer_user_id = %s
            ORDER BY cv.uploaded_at DESC, cv.id DESC
            LIMIT 20
            """,
            (user_id,)
        )
        cvs = cursor.fetchall()

        cursor.execute(
            """
            SELECT a.id, a.status, a.applied_at, a.updated_at, a.selected_at, a.rejected_at,
                   j.id AS job_id, j.title AS job_title,
                   company.id AS company_user_id,
                   COALESCE(cp.company_name, company.display_name) AS company_name,
                   ae.score AS latest_score,
                   ae.recommendation AS latest_recommendation
            FROM applications a
            INNER JOIN job_posts j ON j.id = a.job_post_id
            LEFT JOIN users company ON company.id = a.company_user_id
            LEFT JOIN company_profiles cp ON cp.user_id = company.id
            LEFT JOIN ai_evaluations ae
              ON ae.application_id = a.id
             AND ae.is_latest = 1
            WHERE a.engineer_user_id = %s
            ORDER BY a.applied_at DESC, a.id DESC
            LIMIT 30
            """,
            (user_id,)
        )
        applications_as_candidate = cursor.fetchall()

        cursor.execute(
            """
            SELECT j.id, j.title, j.status, j.location_mode, j.city, j.country,
                   j.created_at, j.published_at,
                   (SELECT COUNT(*) FROM applications a WHERE a.job_post_id = j.id) AS applications_count
            FROM job_posts j
            WHERE j.company_user_id = %s
            ORDER BY j.created_at DESC, j.id DESC
            LIMIT 30
            """,
            (user_id,)
        )
        jobs_as_company = cursor.fetchall()

        cursor.execute(
            """
            SELECT id, type, title, body, priority, is_read, created_at, read_at
            FROM notifications
            WHERE user_id = %s
            ORDER BY created_at DESC, id DESC
            LIMIT 20
            """,
            (user_id,)
        )
        notifications = cursor.fetchall()

        cursor.execute(
            """
            SELECT id, scope, title, created_at, updated_at,
                   (SELECT COUNT(*) FROM ai_messages m WHERE m.conversation_id = ai_conversations.id) AS messages_count
            FROM ai_conversations
            WHERE owner_user_id = %s
            ORDER BY updated_at DESC, id DESC
            LIMIT 20
            """,
            (user_id,)
        )
        conversations = cursor.fetchall()

        return {
            "user": user,
            "engineer_profile": engineer_profile,
            "company_profile": company_profile,
            "cvs": cvs,
            "applications_as_candidate": applications_as_candidate,
            "jobs_as_company": jobs_as_company,
            "notifications": notifications,
            "conversations": conversations
        }
    finally:
        conn.close()


@app.get("/admin/companies/{company_user_id}/detail")
def admin_company_detail(company_user_id: int, request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT u.id, u.username, u.email, u.display_name, u.is_active, u.is_online,
                   u.last_login, u.created_at,
                   cp.company_name, cp.contact_phone
            FROM users u
            LEFT JOIN company_profiles cp ON cp.user_id = u.id
            WHERE u.id = %s AND u.role = 'Empresa' AND u.deleted_at IS NULL
            LIMIT 1
            """,
            (company_user_id,)
        )
        company = cursor.fetchone()
        if not company:
            raise HTTPException(status_code=404, detail="Empresa no encontrada")

        cursor.execute(
            """
            SELECT j.id, j.title, j.description, j.status, j.location_mode,
                   j.city, j.country, j.created_at, j.published_at,
                   (SELECT COUNT(*) FROM applications a WHERE a.job_post_id = j.id) AS applications_count
            FROM job_posts j
            WHERE j.company_user_id = %s
            ORDER BY j.created_at DESC, j.id DESC
            LIMIT 50
            """,
            (company_user_id,)
        )
        jobs = cursor.fetchall()

        cursor.execute(
            """
            SELECT a.id, a.status, a.applied_at, a.updated_at,
                   j.id AS job_id, j.title AS job_title,
                   engineer.id AS engineer_user_id,
                   engineer.display_name AS engineer_name,
                   engineer.email AS engineer_email,
                   cv.id AS cv_document_id,
                   cv.file_name AS cv_file_name,
                   cv.status AS cv_status,
                   ae.score AS latest_score,
                   ae.recommendation AS latest_recommendation
            FROM applications a
            INNER JOIN job_posts j ON j.id = a.job_post_id
            INNER JOIN users engineer ON engineer.id = a.engineer_user_id
            LEFT JOIN cv_documents cv ON cv.id = a.cv_document_id
            LEFT JOIN ai_evaluations ae ON ae.application_id = a.id AND ae.is_latest = 1
            WHERE a.company_user_id = %s
            ORDER BY a.applied_at DESC, a.id DESC
            LIMIT 80
            """,
            (company_user_id,)
        )
        applications = cursor.fetchall()

        return {
            "company": company,
            "jobs": jobs,
            "applications": applications
        }
    finally:
        conn.close()


@app.get("/admin/cvs")
def admin_cvs(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                cv.id,
                cv.engineer_user_id,
                u.username,
                u.display_name AS engineer_name,
                u.email AS engineer_email,
                cv.file_name,
                cv.original_file_name,
                cv.status,
                cv.is_active,
                cv.uploaded_at,
                cv.analyzed_at,
                cv.error_message,
                ai.id AS analysis_id,
                ai.summary,
                ai.overall_score,
                ai.detected_skills,
                ai.suggested_roles,
                ai.model_name,
                ai.created_at AS analysis_created_at
            FROM cv_documents cv
            INNER JOIN users u ON u.id = cv.engineer_user_id
            LEFT JOIN cv_ai_summaries ai
              ON ai.cv_document_id = cv.id
             AND ai.is_latest = 1
            ORDER BY cv.uploaded_at DESC, cv.id DESC
            LIMIT 200
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/admin/applications/all")
def admin_applications_all(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                a.id,
                a.status,
                a.applied_at,
                a.updated_at,
                a.reviewed_at,
                a.selected_at,
                a.rejected_at,
                j.id AS job_id,
                j.title AS job_title,
                company.id AS company_user_id,
                COALESCE(cp.company_name, company.display_name) AS company_name,
                engineer.id AS engineer_user_id,
                engineer.display_name AS engineer_name,
                engineer.email AS engineer_email,
                cv.id AS cv_document_id,
                cv.file_name AS cv_file_name,
                cv.status AS cv_status,
                ae.score AS latest_score,
                ae.recommendation AS latest_recommendation,
                ae.summary_for_company AS latest_summary
            FROM applications a
            INNER JOIN job_posts j ON j.id = a.job_post_id
            INNER JOIN users engineer ON engineer.id = a.engineer_user_id
            LEFT JOIN users company ON company.id = a.company_user_id
            LEFT JOIN company_profiles cp ON cp.user_id = company.id
            LEFT JOIN cv_documents cv ON cv.id = a.cv_document_id
            LEFT JOIN ai_evaluations ae ON ae.application_id = a.id AND ae.is_latest = 1
            ORDER BY a.applied_at DESC, a.id DESC
            LIMIT 300
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/admin/chat/conversations")
def admin_chat_conversations(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                c.id,
                c.owner_user_id,
                u.username,
                u.display_name,
                c.owner_role,
                c.scope,
                c.title,
                c.created_at,
                c.updated_at,
                (SELECT COUNT(*) FROM ai_messages m WHERE m.conversation_id = c.id) AS messages_count,
                (SELECT MAX(m.created_at) FROM ai_messages m WHERE m.conversation_id = c.id) AS last_message_at
            FROM ai_conversations c
            INNER JOIN users u ON u.id = c.owner_user_id
            ORDER BY c.updated_at DESC, c.id DESC
            LIMIT 200
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/admin/notifications/all")
def admin_notifications_all(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                n.id,
                n.user_id,
                u.username,
                u.display_name,
                n.type,
                n.title,
                n.body,
                n.priority,
                n.is_read,
                n.created_at,
                n.read_at,
                n.related_job_post_id,
                n.related_application_id
            FROM notifications n
            INNER JOIN users u ON u.id = n.user_id
            ORDER BY n.created_at DESC, n.id DESC
            LIMIT 200
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()


@app.get("/admin/audit/logs")
def admin_audit_logs(request: Request):
    require_admin(request)
    conn = get_db()
    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                l.id,
                l.admin_user_id,
                u.username AS admin_username,
                u.display_name AS admin_name,
                l.action,
                l.target_type,
                l.target_id,
                l.details,
                l.created_at
            FROM admin_audit_logs l
            INNER JOIN users u ON u.id = l.admin_user_id
            ORDER BY l.created_at DESC, l.id DESC
            LIMIT 200
            """
        )
        return cursor.fetchall()
    finally:
        conn.close()

@app.get("/users/profile")
def user_profile(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT *
            FROM users
            WHERE id = %s
            LIMIT 1
            """,
            (payload["user_id"],)
        )
        profile = cursor.fetchone()
        return profile
    finally:
        conn.close()

@app.get("/notifications/my")
def my_notifications(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    conn = get_db()
    try:
        cursor = conn.cursor(
            dictionary=True
        )
        cursor.execute(
            """
            SELECT
                id,
                user_id,
                type,
                title,
                body,
                body AS message,
                related_job_post_id,
                related_application_id,
                related_request_id,
                metadata,
                priority,
                is_read,
                created_at,
                read_at
            FROM notifications
            WHERE user_id = %s
            ORDER BY is_read ASC, created_at DESC, id DESC
            """,
            (payload["user_id"],)
        )
        notifications = cursor.fetchall()
        return notifications
    finally:
        conn.close()

@app.put("/notifications/read/{notification_id}")
def read_notification(
    notification_id: int,
    request: Request
):
    token = get_token(request)
    payload = decode_token(token)
    conn = get_db()
    try:
        cursor = conn.cursor()
        cursor.execute(
            """
            UPDATE notifications
            SET is_read = 1,
                read_at = NOW()
            WHERE id = %s
            AND user_id = %s
            """,
            (
                notification_id,
                payload["user_id"]
            )
        )
        conn.commit()
        return {
            "message": "Notificacion leida"
        }
    finally:
        conn.close()


@app.post("/chat/message", response_model=ChatMessageResponse)
def chat_message(
    request: Request,
    data: ChatMessageRequest
):
    return _process_chat_message(
        request=request,
        conversation_id=data.conversation_id,
        message=data.message,
        scope=data.scope,
        context=data.context
    )


@app.post("/chat/message-with-file", response_model=ChatMessageResponse)
async def chat_message_with_file(
    request: Request,
    message: str = Form(...),
    scope: str = Form("APP_HELP"),
    conversation_id: int | None = Form(None),
    context: str | None = Form(None),
    archivo: UploadFile | None = File(None)
):
    attached_pdf_name = None
    attached_pdf_text = None

    if archivo is not None and archivo.filename:
        attached_pdf_name, _attached_path, attached_pdf_text = _save_chat_pdf_and_extract(archivo)

    return _process_chat_message(
        request=request,
        conversation_id=conversation_id,
        message=message,
        scope=scope,
        context=_parse_chat_context(context),
        attached_pdf_name=attached_pdf_name,
        attached_pdf_text=attached_pdf_text
    )


@app.get("/chat/conversations")
def chat_conversations(request: Request):
    token = get_token(request)
    payload = decode_token(token)
    user_id = int(payload["user_id"])

    conn = get_db()

    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT
                id,
                owner_user_id,
                owner_role,
                scope,
                title,
                context,
                created_at,
                updated_at
            FROM ai_conversations
            WHERE owner_user_id = %s
            ORDER BY updated_at DESC, id DESC
            """,
            (user_id,)
        )

        return cursor.fetchall()

    finally:
        conn.close()


@app.get("/chat/history/{conversation_id}")
def chat_history(
    conversation_id: int,
    request: Request
):
    token = get_token(request)
    payload = decode_token(token)
    user_id = int(payload["user_id"])

    conn = get_db()

    try:
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            """
            SELECT id
            FROM ai_conversations
            WHERE id = %s
              AND owner_user_id = %s
            LIMIT 1
            """,
            (
                conversation_id,
                user_id
            )
        )

        conversation = cursor.fetchone()

        if not conversation:
            raise HTTPException(
                status_code=404,
                detail="Conversación no encontrada"
            )

        cursor.execute(
            """
            SELECT
                id,
                conversation_id,
                sender,
                message,
                model_name,
                token_usage,
                metadata,
                created_at
            FROM ai_messages
            WHERE conversation_id = %s
            ORDER BY created_at ASC, id ASC
            """,
            (conversation_id,)
        )

        return cursor.fetchall()

    finally:
        conn.close()


class CVRequest(BaseModel):
    cv_texto: str
    puesto: str


def require_ia_available():
    if not IA_AVAILABLE:
        raise HTTPException(
            status_code=503,
            detail=f"Modulo IA no disponible: {IA_IMPORT_ERROR}"
        )


@app.post("/analizar")
def analizar(data: CVRequest):
    require_ia_available()

    resultado = analizar_cv_con_ia(
        data.cv_texto,
        data.puesto
    )

    return resultado


@app.post("/analizar-cv")
async def analizar_cv_endpoint(
    archivo: UploadFile = File(...),
    puesto: str = Form(...)
):
    require_ia_available()

    extensiones_validas = [
        ".pdf",
        ".png",
        ".jpg",
        ".jpeg"
    ]

    filename = archivo.filename or "cv"
    extension = os.path.splitext(filename)[1].lower()

    if extension not in extensiones_validas:
        raise HTTPException(
            status_code=400,
            detail="Formato no permitido"
        )

    safe_filename = os.path.basename(filename)
    unique_name = f"{uuid.uuid4()}_{safe_filename}"
    ruta_archivo = os.path.join(
        UPLOAD_DIR,
        unique_name
    )

    with open(ruta_archivo, "wb") as buffer:
        shutil.copyfileobj(
            archivo.file,
            buffer
        )

    texto_cv = extraer_texto(
        ruta_archivo
    )

    if len(texto_cv.strip()) < 30:
        raise HTTPException(
            status_code=400,
            detail="No se pudo extraer suficiente texto del archivo"
        )

    resultado = analizar_cv_con_ia(
        texto_cv,
        puesto
    )

    resultado["preview_texto"] = texto_cv[:1000]

    return resultado

