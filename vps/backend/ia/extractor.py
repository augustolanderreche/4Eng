import fitz
import pytesseract
import os
import cv2
import numpy as np

from PIL import Image
from pdf2image import convert_from_path


# =========================
# LIMPIEZA DE TEXTO
# =========================

def limpiar_texto(texto):

    texto = texto.replace("\n", " ")
    texto = texto.replace("\t", " ")

    while "  " in texto:
        texto = texto.replace("  ", " ")

    return texto.strip()


# =========================
# MEJORA DE IMAGEN OCR
# =========================

def mejorar_imagen_para_ocr(path_imagen):

    imagen = cv2.imread(path_imagen)

    gris = cv2.cvtColor(imagen, cv2.COLOR_BGR2GRAY)

    gris = cv2.GaussianBlur(gris, (5, 5), 0)

    _, threshold = cv2.threshold(
        gris,
        0,
        255,
        cv2.THRESH_BINARY + cv2.THRESH_OTSU
    )

    return threshold


# =========================
# OCR IMAGEN
# =========================

def extraer_texto_imagen(path_imagen):

    imagen_mejorada = mejorar_imagen_para_ocr(path_imagen)

    texto = pytesseract.image_to_string(
        imagen_mejorada,
        lang="spa+eng",
        config="--oem 3 --psm 6"
    )

    return limpiar_texto(texto)


# =========================
# PDF NORMAL
# =========================

def extraer_texto_pdf_normal(path_pdf):

    texto = ""

    pdf = fitz.open(path_pdf)

    for pagina in pdf:

        texto_pagina = pagina.get_text()

        if texto_pagina.strip():
            texto += texto_pagina + "\n"

    return limpiar_texto(texto)


# =========================
# PDF ESCANEADO
# =========================

def extraer_texto_pdf_ocr(path_pdf):

    paginas = convert_from_path(path_pdf)

    texto_total = ""

    for i, pagina in enumerate(paginas):

        temp_path = f"temp_page_{i}.jpg"

        pagina.save(temp_path, "JPEG")

        texto = extraer_texto_imagen(temp_path)

        texto_total += texto + "\n"

        os.remove(temp_path)

    return limpiar_texto(texto_total)


# =========================
# DETECTAR TIPO PDF
# =========================

def pdf_tiene_texto(path_pdf):

    pdf = fitz.open(path_pdf)

    for pagina in pdf:

        texto = pagina.get_text()

        if texto.strip():
            return True

    return False


# =========================
# EXTRACTOR GENERAL
# =========================

def extraer_texto(path_archivo):

    extension = os.path.splitext(path_archivo)[1].lower()

    # PDF
    if extension == ".pdf":

        if pdf_tiene_texto(path_archivo):

            print("PDF con texto detectado")

            return extraer_texto_pdf_normal(path_archivo)

        else:

            print("PDF escaneado detectado")

            return extraer_texto_pdf_ocr(path_archivo)

    # Imagen
    elif extension in [".png", ".jpg", ".jpeg"]:

        return extraer_texto_imagen(path_archivo)

    else:
        raise Exception("Formato no soportado")