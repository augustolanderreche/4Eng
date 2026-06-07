#!/bin/sh
set -e

CERT_DIR=/etc/nginx/certs
CRT=${CERT_DIR}/selfsigned.crt
KEY=${CERT_DIR}/selfsigned.key
DOMAIN=${NGINX_SERVER_NAME:-localhost}

if [ ! -f "$CRT" ] || [ ! -f "$KEY" ]; then
  mkdir -p "$CERT_DIR"

  openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout "$KEY" \
    -out "$CRT" \
    -subj "/C=AR/ST=BA/L=CABA/O=VPS-POO/OU=DEV/CN=${DOMAIN}"
fi

exec nginx -g "daemon off;"
