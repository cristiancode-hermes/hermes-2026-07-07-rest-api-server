# 🌐 REST API Server — C++23

Servidor REST moderno escrito en C++23 con cpp-httplib, nlohmann/json y spdlog.

## ✨ Features

- ✅ API RESTful con endpoints: health, info, echo, validate, time
- ✅ Middleware: CORS, Rate Limiting, Logging automático
- ✅ Manejo de errores con `std::expected`
- ✅ Configuración via variables de entorno o archivo JSON
- ✅ Compilación multi-stage con Docker
- ✅ Tests unitarios con doctest
- ✅ RAII, PIMPL, Concepts modernos C++20
- ✅ Sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer)
- ✅ Señales SIGINT/SIGTERM para parada graceful

## 🏗️ Stack

| Componente     | Librería                               |
|----------------|----------------------------------------|
| HTTP Server    | [cpp-httplib](https://github.com/yhirose/cpp-httplib) (header-only) |
| JSON           | [nlohmann/json](https://github.com/nlohmann/json) (header-only)   |
| Logging        | [spdlog](https://github.com/gabime/spdlog)                        |
| Testing        | [doctest](https://github.com/doctest/doctest) (header-only)       |
| Build          | CMake 3.28+ / Ninja                    |
| Compilador     | GCC 14 (C++20)                         |

## 📁 Estructura

```
2026-07-07-rest-api-server/
├── CMakeLists.txt          # Build system C++20
├── CMakePresets.json       # Presets debug/release/sanitize
├── Dockerfile              # Multi-stage (gcc:14 → debian:slim)
├── vcpkg.json              # Manifest vcpkg (opcional)
├── .env.example            # Variables de entorno
├── .gitignore
├── README.md
├── include/
│   └── restapi/
│       ├── server.hpp      # Server class (PIMPL)
│       ├── routes.hpp      # Route handlers
│       ├── models.hpp      # Data models
│       └── middleware.hpp  # CORS, RateLimiter, Logger
├── src/
│   ├── main.cpp            # Entry point con signal handling
│   ├── server.cpp          # Server implementation
│   └── routes.cpp          # Route handler implementations
├── tests/
│   ├── test_main.cpp       # Model tests
│   └── test_routes.cpp     # Middleware & integration tests
└── docs/
    └── architecture.md     # Documentación técnica
```

## 🚀 Compilación y ejecución

### Requisitos

- CMake ≥ 3.28
- Ninja
- GCC 14+ o Clang 16+
- Conexión a internet (FetchContent descarga dependencias)

### Build

```bash
# Debug con tests
cmake --preset debug
cmake --build --preset debug

# Release optimizado
cmake --preset release
cmake --build --preset release

# Sanitizers (ASan + UBSan)
cmake --preset sanitize
cmake --build --preset sanitize
```

### Ejecutar

```bash
# Puerto por defecto 8080
./build/debug/restapi-server

# Puerto personalizado
SERVER_PORT=9090 ./build/debug/restapi-server

# Config desde archivo
./build/debug/restapi-server -c config.json

# Elegir nivel de log
LOG_LEVEL=trace ./build/debug/restapi-server
```

### Tests

```bash
cmake --build --preset debug
cd build/debug && ctest --preset debug
# o directamente:
./build/debug/restapi-server_tests
```

### Docker

```bash
docker build -t restapi-server .
docker run -p 8080:8080 restapi-server
```

## 📡 Endpoints de la API

| Método | Ruta            | Descripción                        |
|--------|------------------|-------------------------------------|
| GET    | `/api/health`    | Health check del servidor           |
| GET    | `/api/info`      | Información del servidor y compilador |
| POST   | `/api/echo`      | Eco: devuelve el body recibido      |
| POST   | `/api/validate`  | Valida un string (longitud, regex)  |
| GET    | `/api/time`      | Tiempo actual UTC y local           |

### Ejemplos

```bash
# Health
curl http://localhost:8080/api/health

# Info
curl http://localhost:8080/api/info

# Echo
curl -X POST http://localhost:8080/api/echo \
  -H 'Content-Type: application/json' \
  -d '{"mensaje": "hola mundo"}'

# Validate
curl -X POST http://localhost:8080/api/validate \
  -H 'Content-Type: application/json' \
  -d '{"input": "test123", "min_length": 3, "max_length": 10, "pattern": "^[a-z0-9]+$"}'

# Time
curl http://localhost:8080/api/time
```

## ⚙️ Configuración

Via variables de entorno (`.env.example`) o archivo JSON con `-c`:

| Variable                     | Por defecto | Descripción                    |
|------------------------------|-------------|---------------------------------|
| `SERVER_PORT`                | `8080`      | Puerto del servidor             |
| `SERVER_HOST`                | `0.0.0.0`   | Interfaz de red                 |
| `LOG_LEVEL`                  | `info`      | Nivel de log                    |
| `LOG_PATTERN`                | ...         | Formato de log (formato spdlog) |
| `CORS_ALLOWED_ORIGINS`       | `*`         | Orígenes CORS permitidos        |
| `RATE_LIMIT_ENABLED`         | `true`      | Habilitar rate limiting         |
| `RATE_LIMIT_REQUESTS`        | `100`       | Máx peticiones por ventana      |
| `RATE_LIMIT_WINDOW_SEC`      | `60`        | Ventana de tiempo (segundos)    |

## 📄 Licencia

MIT
