from openai import OpenAI
from dotenv import load_dotenv

import os
import json

load_dotenv()

client = OpenAI(
    api_key=os.getenv("OPENAI_API_KEY")
)

AI_MODEL = os.getenv("OPENAI_MODEL", "gpt-5.4-mini")


def analizar_cv(cv_texto, puesto):
    prompt = f"""
Analiza el siguiente currículum.

IMPORTANTE:
- Devuelve SOLO JSON válido.
- No expliques nada fuera del JSON.
- Si falta información, usa null.
- La compatibilidad debe ser un número de 0 a 100.

Debes devolver exactamente esta estructura:

{{
  "nombre": "",
  "email": "",
  "telefono": "",
  "seniority": "",
  "años_experiencia": 0,
  "tecnologias": [],
  "idiomas": [],
  "resumen": "",
  "fortalezas": [],
  "debilidades": [],
  "compatibilidad": 0,
  "explicacion_compatibilidad": "",
  "roles_sugeridos": [],
  "recomendaciones_capacitacion": []
}}

PUESTO OBJETIVO:
{puesto}

CURRÍCULUM:
{cv_texto}
"""

    response = client.chat.completions.create(
        model=AI_MODEL,
        messages=[
            {
                "role": "system",
                "content": (
                    "Eres un analizador profesional de recursos humanos "
                    "especializado en selección de perfiles IT."
                )
            },
            {
                "role": "user",
                "content": prompt
            }
        ]
    )

    contenido = response.choices[0].message.content

    try:
        return json.loads(contenido)

    except Exception as e:
        return {
            "error": "Error parseando JSON",
            "detalle": str(e),
            "respuesta_modelo": contenido
        }
