#!/usr/bin/env bash
set -Eeuo pipefail

FLASK_HOST="${FLASK_HOST:-0.0.0.0}"
FLASK_PORT="${FLASK_PORT:-5000}"
FLASK_APP_MODULE="${FLASK_APP_MODULE:-/app/health_check.py}"
FLASK_MODE="${FLASK_MODE:-flask}"

# Ścieżki do binarek/procesów
ENGINE_MQTT_BIN="${ENGINE_MQTT_BIN:-/app/build/engine_mqtt}"
CHESS_CLI_BIN="${CHESS_CLI_BIN:-/app/build/chess_cli}"

pids=()

start_flask() {
  if [ "$FLASK_MODE" = "gunicorn" ]; then
    echo "[ENTRYPOINT] WARN: FLASK_MODE=gunicorn wymaga modułu w formie pkg:app, aktualnie: ${FLASK_APP_MODULE}"
    gunicorn --bind "${FLASK_HOST}:${FLASK_PORT}" "${FLASK_APP_MODULE}" &
  else
    # Prosty tryb: 'flask run' na wskazanym pliku
    export FLASK_APP="${FLASK_APP_MODULE}"
    python3 -m flask run --host "${FLASK_HOST}" --port "${FLASK_PORT}" &
  fi
  pids+=($!)
  echo "[ENTRYPOINT] Flask started on ${FLASK_HOST}:${FLASK_PORT}, pid ${pids[-1]}"
}

start_engine_mqtt() {
  if [ -x "$ENGINE_MQTT_BIN" ]; then
    "$ENGINE_MQTT_BIN" &
    pids+=($!)
    echo "[ENTRYPOINT] engine_mqtt started, pid ${pids[-1]}"
  elif [ -f "$ENGINE_MQTT_BIN" ]; then
    chmod +x "$ENGINE_MQTT_BIN" || true
    "$ENGINE_MQTT_BIN" &
    pids+=($!)
    echo "[ENTRYPOINT] engine_mqtt started (after chmod), pid ${pids[-1]}"
  elif [ -f "/app/engine_mqtt.py" ]; then
    python3 /app/engine_mqtt.py &
    pids+=($!)
    echo "[ENTRYPOINT] engine_mqtt (python) started, pid ${pids[-1]}"
  else
    echo "[ENTRYPOINT] INFO: engine_mqtt nie znaleziony (${ENGINE_MQTT_BIN}) – pomijam."
  fi
}

start_chess_cli() {
  if [ -x "$CHESS_CLI_BIN" ]; then
    "$CHESS_CLI_BIN" &
    pids+=($!)
    echo "[ENTRYPOINT] chess_cli started, pid ${pids[-1]}"
  elif [ -f "$CHESS_CLI_BIN" ]; then
    chmod +x "$CHESS_CLI_BIN" || true
    "$CHESS_CLI_BIN" &
    pids+=($!)
    echo "[ENTRYPOINT] chess_cli started (after chmod), pid ${pids[-1]}"
  else
    echo "[ENTRYPOINT] WARN: chess_cli nie znaleziony (${CHESS_CLI_BIN})."
  fi
}

_term() {
  echo "[ENTRYPOINT] Caught termination signal. Stopping children: ${pids[*]}"
  for pid in "${pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      kill -TERM "$pid" 2>/dev/null || true
    fi
  done
  wait
  echo "[ENTRYPOINT] All stopped. Bye."
  exit 0
}

trap _term SIGTERM SIGINT

# Start usług
start_flask
start_engine_mqtt
start_chess_cli

while true; do
  if ! kill -0 "${pids[0]}" 2>/dev/null; then _term; fi
  sleep 2
done
