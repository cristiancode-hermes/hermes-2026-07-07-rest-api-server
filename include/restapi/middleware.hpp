#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <regex>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <restapi/models.hpp>

namespace restapi {

using json = nlohmann::json;

/// Middleware de logging de peticiones
class RequestLogger {
public:
  explicit RequestLogger() = default;

  void operator()(const httplib::Request& req, httplib::Response& res) const {
    const auto duration = std::chrono::steady_clock::now() - start_;
    spdlog::info("[{}] {} {} -> {} ({}ms)",
                 req.remote_addr,
                 req.method,
                 req.path,
                 res.status,
                 std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
  }

private:
  std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};
};

/// Middleware CORS
class CorsMiddleware {
public:
  explicit CorsMiddleware(std::string_view allowed_origins = "*")
    : allowed_origins_(allowed_origins) {}

  void operator()(const httplib::Request& /*req*/, httplib::Response& res) const {
    res.set_header("Access-Control-Allow-Origin", allowed_origins_);
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.set_header("Access-Control-Max-Age", "86400");
  }

private:
  std::string allowed_origins_;
};

/// Middleware de rate limiting (token bucket simplificado)
class RateLimiter {
public:
  explicit RateLimiter(int max_requests = 100, int window_sec = 60)
    : max_requests_(max_requests)
    , window_sec_(window_sec) {}

  /// Devuelve true si la petición está permitida
  auto allow(std::string_view client_ip) -> bool {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    auto& entry = clients_[std::string(client_ip)];
    auto& [count, window_start] = entry;

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      now - window_start).count();

    if (elapsed >= window_sec_) {
      // Ventana expirada, reiniciar
      count = 1;
      window_start = now;
      return true;
    }

    if (count >= max_requests_) {
      spdlog::warn("Rate limit exceeded for {}", client_ip);
      return false;
    }

    ++count;
    return true;
  }

  /// Limpia entradas antiguas
  auto cleanup() -> void {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    std::erase_if(clients_, [&](const auto& item) {
      const auto& [_, entry] = item;
      const auto& [__, window_start] = entry;
      return std::chrono::duration_cast<std::chrono::seconds>(
        now - window_start).count() >= window_sec_ * 2;
    });
  }

private:
  int max_requests_;
  int window_sec_;
  std::mutex mutex_;
  std::unordered_map<std::string,
    std::pair<int, std::chrono::steady_clock::time_point>> clients_;
};

/// Middleware de validación de Content-Type
class ContentTypeValidator {
public:
  void operator()(const httplib::Request& req, httplib::Response& /*res*/,
                  bool has_body) const {
    (void)req;
    (void)has_body;
    // Content type validation is handled by the route handlers
  }
};

} // namespace restapi
