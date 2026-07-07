# 🏗️ Arquitectura — REST API Server

## Visión general

Servidor REST moderno implementado en C++23 siguiendo principios de diseño RAII, PIMPL y cero-cost abstractions. La arquitectura está organizada en tres capas principales: **transporte HTTP**, **lógica de negocio (rutas)** y **modelos de datos**.

```
┌─────────────────────────────────────────────────┐
│                  main.cpp                        │
│  • Señales (SIGINT/SIGTERM)                     │
│  • Configuración (env / archivo)                │
│  • Inicialización del Server                    │
│  • Bucle principal + graceful shutdown          │
└──────────────┬──────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────┐
│              Server (server.hpp/.cpp)            │
│  • PIMPL (Impl) oculta los detalles             │
│  • Posee httplib::Server                        │
│  • Configura middleware global                  │
│  • Registra rutas via Routes                    │
│  • Manejo de excepciones                        │
│  • Logging de peticiones                        │
└──────────────┬──────────────────────────────────┘
               │
       ┌───────┴──────────┐
       ▼                  ▼
┌──────────────┐  ┌──────────────┐
│   Routes     │  │  Middleware   │
│  (rutas.cpp) │  │ (middleware.hpp)
│              │  │              │
│ • /api/health│  │ • CORS       │
│ • /api/info  │  │ • RateLimiter│
│ • /api/echo  │  │ • Logger     │
│ • /api/validate│  └──────────────┘
│ • /api/time  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Models     │
│ (models.hpp) │
│              │
│ • ApiResponse│
│ • ServerInfo │
│ • Validation │
└──────────────┘
```

## Patrones de diseño

### PIMPL (Pointer to Implementation)

La clase `Server` usa el patrón PIMPL para:
- Ocultar la dependencia de `httplib::Server` del header público
- Reducir tiempos de compilación
- Mantener ABI estable

```cpp
// server.hpp — solo el puntero
class Server {
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// server.cpp — implementación completa
struct Server::Impl {
  httplib::Server svr;
  // ...
};
```

### RAII

Todos los recursos se gestionan automáticamente:
- `std::unique_ptr<Impl>` para el PIMPL
- `std::unique_ptr<Routes>` y `std::unique_ptr<RateLimiter>` como miembros del Impl
- `std::mutex` en RateLimiter para protección de concurrencia
- `std::atomic<bool>` para señal de parada

### Manejo de errores con `std::expected`

Las funciones que pueden fallar retornan `std::expected<T, E>`:

```cpp
auto Config::from_file(std::string_view path)
    -> std::expected<Config, std::string>;

auto Server::run()
    -> std::expected<void, ServerError>;
```

### Concepts (C++20)

Se define el concept `RouteHandler` para garantizar interfaz de handlers:

```cpp
template <typename T>
concept RouteHandler = requires(T& t, const httplib::Request& req,
                                 httplib::Response& res) {
  { t.handle(req, res) } -> std::same_as<void>;
};
```

## Middleware chain

El orden de ejecución del middleware es:

1. **CORS** (pre-routing) — Añade cabeceras CORS; responde 204 a OPTIONS
2. **RateLimiter** (pre-routing) — Verifica límite de peticiones por IP; 429 si excede
3. **Route handler** — Ejecuta la lógica del endpoint
4. **Exception handler** — Captura excepciones y retorna 500
5. **Error handler** — Personaliza 404 y 405
6. **Post-routing logger** — Log de cada petición completada

## Seguridad

- **Cabeceras CORS** configurables
- **Rate limiting** por IP con token bucket
- **Validación de entrada** con longitud y regex
- **Sanitizers** en build debug (AddressSanitizer + UBSan)
- **Graceful shutdown** con captura de señales

## Rendimiento

- cpp-httplib es una librería header-only con soporte keep-alive
- spdlog es uno de los loggers C++ más rápidos
- Movilidad (move semantics) en todas las clases
- Unity build habilitado por defecto (CMakePresets.json)
- Compilación con LTO en release

## Dependencias (FetchContent)

Todas las dependencias se descargan automáticamente via CMake FetchContent:

| Dependencia   | Versión   | Header-only |
|---------------|-----------|-------------|
| cpp-httplib   | v0.18.2   | ✅           |
| nlohmann/json | v3.11.3   | ✅           |
| spdlog        | v1.15.1   | ❌ (librería)|
| doctest       | v2.4.11   | ✅           |
