#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <expected>

namespace restapi {

using json = nlohmann::json;

/// Modelo de respuesta estándar de la API
struct ApiResponse {
  bool success{true};
  std::string message;
  std::optional<json> data{std::nullopt};
  std::optional<std::string> error{std::nullopt};
  int64_t timestamp{
    std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count()
  };

  /// Convierte a JSON
  [[nodiscard]] auto to_json() const -> json {
    json j;
    j["success"] = success;
    j["message"] = message;
    j["timestamp"] = timestamp;
    if (data.has_value()) j["data"] = data.value();
    if (error.has_value()) j["error"] = error.value();
    return j;
  }

  /// Crea una respuesta de éxito
  static auto ok(std::string_view msg = "OK",
                 std::optional<json> data = std::nullopt) -> ApiResponse {
    ApiResponse r;
    r.success = true;
    r.message = std::string(msg);
    r.data = std::move(data);
    return r;
  }

  /// Crea una respuesta de error
  static auto err(std::string_view msg, std::string_view err_detail = "") -> ApiResponse {
    ApiResponse r;
    r.success = false;
    r.message = std::string(msg);
    r.error = std::string(err_detail);
    return r;
  }
};

/// Payload de validación
struct ValidationRequest {
  std::string input;
  std::optional<int> min_length{std::nullopt};
  std::optional<int> max_length{std::nullopt};
  std::optional<std::string> pattern{std::nullopt};

  static auto from_json(const json& j) -> std::expected<ValidationRequest, std::string> {
    ValidationRequest v;
    if (!j.contains("input") || !j["input"].is_string()) {
      return std::unexpected("Campo 'input' requerido (string)");
    }
    v.input = j["input"].get<std::string>();
    if (j.contains("min_length") && j["min_length"].is_number()) {
      v.min_length = j["min_length"].get<int>();
    }
    if (j.contains("max_length") && j["max_length"].is_number()) {
      v.max_length = j["max_length"].get<int>();
    }
    if (j.contains("pattern") && j["pattern"].is_string()) {
      v.pattern = j["pattern"].get<std::string>();
    }
    return v;
  }

  [[nodiscard]] auto to_json() const -> json {
    json j;
    j["input"] = input;
    if (min_length.has_value()) j["min_length"] = min_length.value();
    if (max_length.has_value()) j["max_length"] = max_length.value();
    if (pattern.has_value()) j["pattern"] = pattern.value();
    return j;
  }
};

/// Resultado de validación
struct ValidationResult {
  bool valid{true};
  std::vector<std::string> errors;

  [[nodiscard]] auto to_json() const -> json {
    json j;
    j["valid"] = valid;
    j["errors"] = errors;
    return j;
  }
};

/// Información del servidor
struct ServerInfo {
  std::string name{"restapi-server"};
  std::string version{"1.0.0"};
  std::string language{"C++20"};
  std::string framework{"cpp-httplib"};
  std::string compiler;
  std::string os;
  std::string uptime;
  int active_connections{0};

  [[nodiscard]] auto to_json() const -> json {
    json j;
    j["name"] = name;
    j["version"] = version;
    j["language"] = language;
    j["framework"] = framework;
    j["compiler"] = compiler;
    j["os"] = os;
    j["uptime"] = uptime;
    j["active_connections"] = active_connections;
    return j;
  }
};

} // namespace restapi
