# Dockerfile — multi-stage build con GCC 14
# Uso: docker build -t restapi-server . && docker run -p 8080:8080 restapi-server

# ── Stage 1: Builder ─────────────────────────────────────────────────────
FROM gcc:14-bookworm AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    ca-certificates \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copiar solo los archivos de configuración primero para cachear capas
COPY CMakeLists.txt CMakePresets.json vcpkg.json ./
COPY src/ ./src/
COPY include/ ./include/
COPY tests/ ./tests/

RUN cmake --preset release && \
    cmake --build --preset release

# ── Stage 2: Runtime ────────────────────────────────────────────────────
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/release/restapi-server /usr/local/bin/restapi-server

EXPOSE 8080

ENV SERVER_PORT=8080
ENV SERVER_HOST=0.0.0.0
ENV LOG_LEVEL=info

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/api/health || exit 1

ENTRYPOINT ["restapi-server"]
