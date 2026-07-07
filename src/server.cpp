#include <restapi/server.hpp>
#include <restapi/routes.hpp>
#include <restapi/middleware.hpp>
#include <restapi/models.hpp>

#include <cstdlib>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace restapi {

// ── Config ───────────────────────────────────────────────────────────────

auto Config::from_env() -> Config {
  Config cfg;

  if (const auto* p = std::getenv("SERVER_PORT")) {
    cfg.port = std::stoi(p);
  }
  if (const auto* p = std::getenv("SERVER_HOST")) {
    cfg.host = p;
  }
  if (const auto* p = std::getenv("LOG_LEVEL")) {
    cfg.log_level = p;
  }
  if (const auto* p = std::getenv("LOG_PATTERN")) {
    cfg.log_pattern = p;
  }
  if (const auto* p = std::getenv("CORS_ALLOWED_ORIGINS")) {
    cfg.cors_origins = p;
  }
  if (const auto* p = std::getenv("RATE_LIMIT_ENABLED")) {
    cfg.rate_limit_enabled = std::string(p) == "true";
  }
  if (const auto* p = std::getenv("RATE_LIMIT_REQUESTS")) {
    cfg.rate_limit_requests = std::stoi(p);
  }
  if (const auto* p = std::getenv("RATE_LIMIT_WINDOW_SEC")) {
    cfg.rate_limit_window_sec = std::stoi(p);
  }

  return cfg;
}

auto Config::from_file(std::string_view path) -> std::expected<Config, std::string> {
  if (!std::filesystem::exists(path)) {
    return std::unexpected("Archivo no encontrado: " + std::string(path));
  }

  std::ifstream file(path.data());
  if (!file.is_open()) {
    return std::unexpected("No se pudo abrir: " + std::string(path));
  }

  try {
    json j;
    file >> j;

    Config cfg;
    if (j.contains("host")) cfg.host = j["host"].get<std::string>();
    if (j.contains("port")) cfg.port = j["port"].get<int>();
    if (j.contains("log_level")) cfg.log_level = j["log_level"].get<std::string>();
    if (j.contains("cors_origins")) cfg.cors_origins = j["cors_origins"].get<std::string>();
    if (j.contains("rate_limit_enabled")) cfg.rate_limit_enabled = j["rate_limit_enabled"].get<bool>();
    if (j.contains("rate_limit_requests")) cfg.rate_limit_requests = j["rate_limit_requests"].get<int>();
    if (j.contains("rate_limit_window_sec")) cfg.rate_limit_window_sec = j["rate_limit_window_sec"].get<int>();

    return cfg;
  } catch (const json::parse_error& e) {
    return std::unexpected("Error parseando JSON: " + std::string(e.what()));
  } catch (const json::type_error& e) {
    return std::unexpected("Error de tipo en JSON: " + std::string(e.what()));
  }
}

// ── Server Implementation (PIMPL) ────────────────────────────────────────

struct Server::Impl {
  Config config;
  httplib::Server svr;
  std::unique_ptr<Routes> routes;
  std::unique_ptr<RateLimiter> rate_limiter;
  std::chrono::steady_clock::time_point start_time;

  explicit Impl(const Config& cfg)
    : config(cfg)
    , start_time(std::chrono::steady_clock::now())
  {
    rate_limiter = std::make_unique<RateLimiter>(
      cfg.rate_limit_requests, cfg.rate_limit_window_sec);
  }
};

Server::Server(const Config& cfg)
  : impl_(std::make_unique<Impl>(cfg)) {}

Server::~Server() = default;

auto Server::setup() -> void {
  auto& svr = impl_->svr;
  const auto& cfg = impl_->config;

  // ── Middleware global ─────────────────────────────────────────────
  // CORS
  auto cors = CorsMiddleware(cfg.cors_origins);
  svr.set_pre_routing_handler([cors = std::move(cors)](const httplib::Request& req, httplib::Response& res) {
    cors(req, res);
    if (req.method == "OPTIONS") {
      res.status = 204;
      return httplib::Server::HandlerResponse::Handled;
    }
    return httplib::Server::HandlerResponse::Unhandled;
  });

  // Rate limiting (si está habilitado)
  if (cfg.rate_limit_enabled) {
    svr.set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
      if (!impl_->rate_limiter->allow(req.remote_addr)) {
        res.status = 429;
        res.set_content(
          ApiResponse::err("Demasiadas peticiones",
                           "Rate limit excedido").to_json().dump(),
          "application/json");
        return httplib::Server::HandlerResponse::Handled;
      }
      return httplib::Server::HandlerResponse::Unhandled;
    });
  }

  // Logger post-petición
  svr.set_post_routing_handler([](const httplib::Request& req, httplib::Response& res) {
    spdlog::info("{} {} -> {} [{}]",
                 req.method, req.path, res.status, req.remote_addr);
  });

  // ── Rutas ─────────────────────────────────────────────────────────
  impl_->routes = std::make_unique<Routes>(svr);
  impl_->routes->register_all();

  // ── Manejador de errores ──────────────────────────────────────────
  svr.set_exception_handler([](const httplib::Request& req, httplib::Response& res,
                                std::exception_ptr ep) {
    try {
      std::rethrow_exception(ep);
    } catch (const std::exception& e) {
      spdlog::error("Excepción en {} {}: {}", req.method, req.path, e.what());
      res.status = 500;
      res.set_content(
        ApiResponse::err("Error interno del servidor", e.what()).to_json().dump(),
        "application/json");
    }
  });

  // ── Página 404 personalizada ──────────────────────────────────────
  svr.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
    if (res.status == 404) {
      res.set_content(
        ApiResponse::err("Ruta no encontrada",
                         req.path).to_json().dump(),
        "application/json");
    } else if (res.status == 405) {
      res.set_content(
        ApiResponse::err("Método no permitido",
                         req.method).to_json().dump(),
        "application/json");
    }
  });
}

auto Server::run() -> std::expected<void, ServerError> {
  const auto& cfg = impl_->config;

  spdlog::info("Iniciando servidor en {}:{}...", cfg.host, cfg.port);
  spdlog::info("Rate limiting: {}", cfg.rate_limit_enabled ? "ON" : "OFF");

  if (!impl_->svr.listen(cfg.host, cfg.port)) {
    spdlog::error("No se pudo iniciar el servidor en {}:{}", cfg.host, cfg.port);
    return std::unexpected(ServerError::BindError);
  }

  return {};
}

auto Server::stop() -> void {
  spdlog::info("Deteniendo servidor...");
  impl_->svr.stop();
}

auto Server::port() const noexcept -> int {
  return impl_->config.port;
}

} // namespace restapi
