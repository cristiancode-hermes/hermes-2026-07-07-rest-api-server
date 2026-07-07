#pragma once

#include <string_view>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace restapi {

using json = nlohmann::json;

/// Concepto que define un manejador de ruta
template <typename T>
concept RouteHandler = requires(T& t, const httplib::Request& req, httplib::Response& res) {
  { t.handle(req, res) } -> std::same_as<void>;
};

/// Registro de rutas del servidor
class Routes {
public:
  explicit Routes(httplib::Server& svr);
  ~Routes() = default;

  // No copiable, sí movible
  Routes(const Routes&) = delete;
  Routes& operator=(const Routes&) = delete;
  Routes(Routes&&) noexcept = default;
  Routes& operator=(Routes&&) noexcept = default;

  /// Registra todas las rutas de la API
  auto register_all() -> void;

private:
  httplib::Server& svr_;

  // ── Handlers ──────────────────────────────────────────────────────
  auto handle_health(const httplib::Request& req, httplib::Response& res) -> void;
  auto handle_info(const httplib::Request& req, httplib::Response& res) -> void;
  auto handle_echo(const httplib::Request& req, httplib::Response& res) -> void;
  auto handle_validate(const httplib::Request& req, httplib::Response& res) -> void;
  auto handle_time(const httplib::Request& req, httplib::Response& res) -> void;
};

} // namespace restapi
