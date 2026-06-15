#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")/../infra"

docker compose exec mosquitto mosquitto_passwd -c -b /mosquitto/config/passwords iot_user change_me_password
docker compose restart mosquitto
