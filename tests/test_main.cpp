#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <restapi/models.hpp>

using namespace restapi;
using json = nlohmann::json;

// ── ApiResponse tests ───────────────────────────────────────────────────

TEST_CASE("ApiResponse::ok crea respuesta exitosa") {
  auto resp = ApiResponse::ok("Todo bien");

  CHECK(resp.success == true);
  CHECK(resp.message == "Todo bien");
  CHECK(resp.data.has_value() == false);
  CHECK(resp.error.has_value() == false);
}

TEST_CASE("ApiResponse::ok con datos") {
  json data = {{"key", "value"}, {"num", 42}};
  auto resp = ApiResponse::ok("Con datos", data);

  CHECK(resp.success == true);
  CHECK(resp.data.has_value() == true);
  CHECK(resp.data.value()["key"] == "value");
  CHECK(resp.data.value()["num"] == 42);
}

TEST_CASE("ApiResponse::err crea respuesta de error") {
  auto resp = ApiResponse::err("Algo falló", "detalle técnico");

  CHECK(resp.success == false);
  CHECK(resp.message == "Algo falló");
  CHECK(resp.error.has_value() == true);
  CHECK(resp.error.value() == "detalle técnico");
}

TEST_CASE("ApiResponse::to_json produce JSON correcto") {
  auto resp = ApiResponse::ok("Prueba", json{{"x", 1}});
  json j = resp.to_json();

  CHECK(j["success"] == true);
  CHECK(j["message"] == "Prueba");
  CHECK(j["data"]["x"] == 1);
  CHECK(j.contains("timestamp"));
  CHECK(j["timestamp"].is_number());
}

// ── ValidationRequest tests ─────────────────────────────────────────────

TEST_CASE("ValidationRequest::from_json con campo requerido") {
  json j = {{"input", "hola"}};
  auto result = ValidationRequest::from_json(j);

  CHECK(result.has_value() == true);
  CHECK(result.value().input == "hola");
  CHECK(result.value().min_length.has_value() == false);
}

TEST_CASE("ValidationRequest::from_json sin input retorna error") {
  json j = {{"otro", "campo"}};
  auto result = ValidationRequest::from_json(j);

  CHECK(result.has_value() == false);
  CHECK(result.error().find("input") != std::string::npos);
}

TEST_CASE("ValidationRequest::from_json con opcionales") {
  json j = {{"input", "test"}, {"min_length", 3}, {"max_length", 10}};
  auto result = ValidationRequest::from_json(j);

  CHECK(result.has_value() == true);
  CHECK(result.value().min_length.value() == 3);
  CHECK(result.value().max_length.value() == 10);
}

TEST_CASE("ValidationRequest::from_json con pattern") {
  json j = {{"input", "abc123"}, {"pattern", "^[a-z0-9]+$"}};
  auto result = ValidationRequest::from_json(j);

  CHECK(result.has_value() == true);
  CHECK(result.value().pattern.value() == "^[a-z0-9]+$");
}

// ── ValidationResult tests ──────────────────────────────────────────────

TEST_CASE("ValidationResult por defecto es válido") {
  ValidationResult r;
  CHECK(r.valid == true);
  CHECK(r.errors.empty());
}

TEST_CASE("ValidationResult::to_json produce JSON correcto") {
  ValidationResult r;
  r.valid = false;
  r.errors = {"error 1", "error 2"};

  json j = r.to_json();
  CHECK(j["valid"] == false);
  CHECK(j["errors"].size() == 2);
  CHECK(j["errors"][0] == "error 1");
}

// ── ServerInfo tests ────────────────────────────────────────────────────

TEST_CASE("ServerInfo valores por defecto") {
  ServerInfo info;
  CHECK(info.name == "restapi-server");
  CHECK(info.version == "1.0.0");
  CHECK(info.language == "C++20");
  CHECK(info.framework == "cpp-httplib");
}

TEST_CASE("ServerInfo::to_json produce JSON correcto") {
  ServerInfo info;
  json j = info.to_json();

  CHECK(j["name"] == "restapi-server");
  CHECK(j["version"] == "1.0.0");
  CHECK(j["language"] == "C++20");
  CHECK(j["framework"] == "cpp-httplib");
  CHECK(j.contains("compiler"));
  CHECK(j.contains("os"));
  CHECK(j.contains("uptime"));
  CHECK(j["active_connections"] == 0);
}
