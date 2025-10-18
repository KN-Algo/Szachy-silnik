FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# --- System deps ---
RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config curl wget ca-certificates \
    libssl-dev \
    nlohmann-json3-dev \
    python3 python3-pip python3-venv \
    && rm -rf /var/lib/apt/lists/*

# --- Zbuduj Paho MQTT C (v1.3.12) ze źródeł ---
# Włącza SSL i instaluje do /usr/local (doda .pc dla pkg-config)
RUN git clone --depth 1 --branch v1.3.12 https://github.com/eclipse/paho.mqtt.c.git /tmp/paho.c \
    && cmake -S /tmp/paho.c -B /tmp/paho.c/build \
    -DPAHO_WITH_SSL=ON \
    -DPAHO_BUILD_STATIC=ON \
    -DPAHO_HIGH_PERFORMANCE=ON \
    && cmake --build /tmp/paho.c/build --target install -j"$(nproc)" \
    && ldconfig \
    && rm -rf /tmp/paho.c

# --- Zbuduj Paho MQTT C++ (v1.3.2) ze źródeł ---
RUN git clone --depth 1 --branch v1.3.2 https://github.com/eclipse/paho.mqtt.cpp.git /tmp/paho.cpp \
    && cmake -S /tmp/paho.cpp -B /tmp/paho.cpp/build \
    -DPAHO_WITH_SSL=ON \
    -DPAHO_BUILD_STATIC=ON \
    -DPAHO_BUILD_DOCUMENTATION=OFF \
    -DPAHO_BUILD_SAMPLES=OFF \
    && cmake --build /tmp/paho.cpp/build --target install -j"$(nproc)" \
    && ldconfig \
    && rm -rf /tmp/paho.cpp

# --- Katalog roboczy i źródła projektu ---
WORKDIR /app
COPY . /app

# --- Build C++ projektu (out-of-source) ---
RUN mkdir -p build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j"$(nproc)"

# --- Python deps (Flask/Gunicorn itp.) ---
# Jeśli masz requirements.txt, użyj go zamiast tej linii.
RUN pip3 install --no-cache-dir flask gunicorn paho-mqtt

# --- Entrypoint orchestruący 3 procesy ---
# (Ten plik wklejałeś wcześniej; jeśli zmieniałeś nazwę lub ścieżkę — dostosuj)
COPY entrypoint.sh /app/entrypoint.sh
# Konwertuj końcówki linii z Windows (\r\n) na Unix (\n) i ustaw uprawnienia
RUN sed -i 's/\r$//' /app/entrypoint.sh && chmod +x /app/entrypoint.sh

# --- Port HTTP dla Flask ---
EXPOSE 5000

# --- Domyślne ENV; możesz nadpisać w docker-compose ---
ENV FLASK_HOST=0.0.0.0 \
    FLASK_PORT=5000 \
    FLASK_MODE=flask \
    FLASK_APP_MODULE=/app/health_check.py \
    ENGINE_MQTT_BIN=/app/build/engine_mqtt \
    CHESS_CLI_BIN=/app/build/chess_cli

ENTRYPOINT ["/app/entrypoint.sh"]
