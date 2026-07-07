#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <optional>
#include <expected>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <httplib.h>

namespace restapi {

using json = nlohmann::json;

/// Enumeración de errores del servidor
enum class ServerError {
  None,
  ConfigError,
  BindError,
  RuntimeError
};

/// Configuración del servidor REST
struct Config {
  std::string host{"0.0.0.0"};
  int port{8080};
  std::string log_level{"info"};
  std::string log_pattern{"[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v"};
  std::string cors_origins{"*"};
  bool rate_limit_enabled{true};
  int rate_limit_requests{100};
  int rate_limit_window_sec{60};

  /// Carga configuración desde variables de entorno
  static auto from_env() -> Config;

  /// Carga configuración desde un archivo JSON
  static auto from_file(std::string_view path) -> std::expected<Config, std::string>;
};

/// Servidor REST con cpp-httplib
class Server {
public:
  explicit Server(const Config& cfg);
  ~Server();

  // No copiable, sí movible
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;
  Server(Server&&) noexcept = default;
  Server& operator=(Server&&) noexcept = default;

  /// Inicializa rutas y middleware
  auto setup() -> void;

  /// Arranca el servidor (bloqueante)
  [[nodiscard]] auto run() -> std::expected<void, ServerError>;

  /// Detiene el servidor
  auto stop() -> void;

  /// Devuelve el puerto configurado
  [[nodiscard]] auto port() const noexcept -> int;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace restapi
