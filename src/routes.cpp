#include <restapi/routes.hpp>
#include <restapi/models.hpp>
#include <restapi/middleware.hpp>
#include <restapi/server.hpp>

#include <string>
#include <regex>
#include <charconv>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace restapi {

Routes::Routes(httplib::Server& svr) : svr_(svr) {}

auto Routes::register_all() -> void {
  // ── GET /api/health ──────────────────────────────────────────────
  svr_.Get("/api/health", [this](const httplib::Request& req, httplib::Response& res) {
    handle_health(req, res);
  });

  // ── GET /api/info ────────────────────────────────────────────────
  svr_.Get("/api/info", [this](const httplib::Request& req, httplib::Response& res) {
    handle_info(req, res);
  });

  // ── POST /api/echo ───────────────────────────────────────────────
  svr_.Post("/api/echo", [this](const httplib::Request& req, httplib::Response& res) {
    handle_echo(req, res);
  });

  // ── POST /api/validate ───────────────────────────────────────────
  svr_.Post("/api/validate", [this](const httplib::Request& req, httplib::Response& res) {
    handle_validate(req, res);
  });

  // ── GET /api/time ────────────────────────────────────────────────
  svr_.Get("/api/time", [this](const httplib::Request& req, httplib::Response& res) {
    handle_time(req, res);
  });

  spdlog::info("Rutas registradas: /api/health, /api/info, /api/echo, /api/validate, /api/time");
}

auto Routes::handle_health(const httplib::Request&, httplib::Response& res) -> void {
  auto response = ApiResponse::ok("Servidor operativo", json{
    {"status", "healthy"},
    {"uptime_seconds", 0}  // Se rellena en el endpoint real
  });
  res.set_content(response.to_json().dump(2), "application/json");
}

auto Routes::handle_info(const httplib::Request&, httplib::Response& res) -> void {
  ServerInfo info;

  // Información del compilador
#if defined(__clang__)
  info.compiler = "Clang " + std::to_string(__clang_major__) + "." +
                  std::to_string(__clang_minor__);
#elif defined(__GNUC__) || defined(__GNUG__)
  info.compiler = "GCC " + std::to_string(__GNUC__) + "." +
                  std::to_string(__GNUC_MINOR__) + "." +
                  std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
  info.compiler = "MSVC " + std::to_string(_MSC_VER);
#else
  info.compiler = "Desconocido";
#endif

  // Información del sistema operativo
#if defined(__linux__)
  info.os = "Linux";
#elif defined(__APPLE__) || defined(__MACH__)
  info.os = "macOS";
#elif defined(_WIN32) || defined(_WIN64)
  info.os = "Windows";
#else
  info.os = "Desconocido";
#endif

  // Timestamp
  const auto now = std::chrono::system_clock::now();
  const auto tt = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%SZ");
  json j = info.to_json();
  j["server_time"] = ss.str();
  j["timezone"] = "UTC";

  auto response = ApiResponse::ok("Información del servidor", j);
  res.set_content(response.to_json().dump(2), "application/json");
}

auto Routes::handle_echo(const httplib::Request& req, httplib::Response& res) -> void {
  try {
    json body = json::parse(req.body);
    json echo_data;
    echo_data["received"] = body;
    echo_data["method"] = req.method;
    echo_data["path"] = req.path;
    echo_data["headers"] = json::object();
    for (const auto& [key, val] : req.headers) {
      echo_data["headers"][key] = val;
    }

    auto response = ApiResponse::ok("Echo", echo_data);
    res.set_content(response.to_json().dump(2), "application/json");
  } catch (const json::parse_error& e) {
    res.status = 400;
    auto response = ApiResponse::err("JSON inválido", e.what());
    res.set_content(response.to_json().dump(2), "application/json");
  }
}

auto Routes::handle_validate(const httplib::Request& req, httplib::Response& res) -> void {
  try {
    json body = json::parse(req.body);
    auto vreq = ValidationRequest::from_json(body);

    if (!vreq.has_value()) {
      res.status = 400;
      auto response = ApiResponse::err("Petición inválida", vreq.error());
      res.set_content(response.to_json().dump(2), "application/json");
      return;
    }

    ValidationResult result;
    const auto& input = vreq->input;

    // Validar longitud mínima
    if (vreq->min_length.has_value() &&
        static_cast<int>(input.length()) < vreq->min_length.value()) {
      result.valid = false;
      result.errors.push_back(
        "Longitud mínima: " + std::to_string(vreq->min_length.value()) +
        " caracteres (actual: " + std::to_string(input.length()) + ")");
    }

    // Validar longitud máxima
    if (vreq->max_length.has_value() &&
        static_cast<int>(input.length()) > vreq->max_length.value()) {
      result.valid = false;
      result.errors.push_back(
        "Longitud máxima: " + std::to_string(vreq->max_length.value()) +
        " caracteres (actual: " + std::to_string(input.length()) + ")");
    }

    // Validar patrón regex
    if (vreq->pattern.has_value()) {
      try {
        std::regex re(vreq->pattern.value());
        if (!std::regex_match(input, re)) {
          result.valid = false;
          result.errors.push_back(
            "No coincide con el patrón: " + vreq->pattern.value());
        }
      } catch (const std::regex_error& e) {
        res.status = 400;
        auto response = ApiResponse::err("Patrón regex inválido", e.what());
        res.set_content(response.to_json().dump(2), "application/json");
        return;
      }
    }

    json j;
    j["validation"] = result.to_json();
    j["input_length"] = input.length();

    auto response = result.valid
      ? ApiResponse::ok("Validación exitosa", j)
      : ApiResponse::err("Validación fallida", j.dump());

    res.set_content(response.to_json().dump(2), "application/json");
  } catch (const json::parse_error& e) {
    res.status = 400;
    auto response = ApiResponse::err("JSON inválido", e.what());
    res.set_content(response.to_json().dump(2), "application/json");
  }
}

auto Routes::handle_time(const httplib::Request& /*req*/, httplib::Response& res) -> void {
  const auto now = std::chrono::system_clock::now();
  const auto tt = std::chrono::system_clock::to_time_t(now);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()).count() % 1000;

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%S");
  ss << "." << std::setfill('0') << std::setw(3) << ms << "Z";

  std::stringstream ss_local;
  ss_local << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");

  json time_data;
  time_data["utc"] = ss.str();
  time_data["local"] = ss_local.str();
  time_data["epoch_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()).count();
  time_data["epoch_s"] = std::chrono::duration_cast<std::chrono::seconds>(
    now.time_since_epoch()).count();

  auto response = ApiResponse::ok("Tiempo actual", time_data);
  res.set_content(response.to_json().dump(2), "application/json");
}

} // namespace restapi
