#include <doctest/doctest.h>
#include <restapi/models.hpp>
#include <restapi/middleware.hpp>
#include <restapi/routes.hpp>

#include <string>
#include <thread>
#include <chrono>
#include <httplib.h>

using namespace restapi;

// ── RequestLogger tests ─────────────────────────────────────────────────

TEST_CASE("RequestLogger se puede instanciar") {
  RequestLogger logger;
  httplib::Request req;
  httplib::Response res;
  req.method = "GET";
  req.path = "/api/test";
  req.remote_addr = "127.0.0.1";
  res.status = 200;

  // Solo verificar que no lanza
  CHECK_NOTHROW(logger(req, res));
}

// ── CorsMiddleware tests ────────────────────────────────────────────────

TEST_CASE("CorsMiddleware establece cabeceras CORS") {
  CorsMiddleware cors("*");
  httplib::Request req;
  httplib::Response res;

  cors(req, res);

  CHECK(res.get_header_value("Access-Control-Allow-Origin") == "*");
  CHECK(res.get_header_value("Access-Control-Allow-Methods").find("GET") != std::string::npos);
  CHECK(res.get_header_value("Access-Control-Allow-Headers").find("Content-Type") != std::string::npos);
}

TEST_CASE("CorsMiddleware con origen específico") {
  CorsMiddleware cors("https://example.com");
  httplib::Request req;
  httplib::Response res;

  cors(req, res);

  CHECK(res.get_header_value("Access-Control-Allow-Origin") == "https://example.com");
}

// ── RateLimiter tests ───────────────────────────────────────────────────

TEST_CASE("RateLimiter permite peticiones dentro del límite") {
  RateLimiter limiter(5, 60);

  for (int i = 0; i < 5; ++i) {
    CHECK(limiter.allow("127.0.0.1") == true);
  }
}

TEST_CASE("RateLimiter bloquea después del límite") {
  RateLimiter limiter(3, 60);

  CHECK(limiter.allow("10.0.0.1") == true);
  CHECK(limiter.allow("10.0.0.1") == true);
  CHECK(limiter.allow("10.0.0.1") == true);
  CHECK(limiter.allow("10.0.0.1") == false);
}

TEST_CASE("RateLimiter clientes diferentes no se afectan") {
  RateLimiter limiter(2, 60);

  CHECK(limiter.allow("A") == true);
  CHECK(limiter.allow("A") == true);
  CHECK(limiter.allow("A") == false);  // A excedido

  CHECK(limiter.allow("B") == true);   // B no afectado
  CHECK(limiter.allow("B") == true);
  CHECK(limiter.allow("B") == false);
}

TEST_CASE("RateLimiter cleanup no lanza") {
  RateLimiter limiter(5, 1);
  limiter.allow("test");
  CHECK_NOTHROW(limiter.cleanup());
}

// ── Model integration tests ─────────────────────────────────────────────

TEST_CASE("ApiResponse serialización/deserialización") {
  auto resp = ApiResponse::ok("integrado", json{{"count", 5}});
  json j = resp.to_json();

  CHECK(j["success"] == true);
  CHECK(j["message"] == "integrado");
  CHECK(j["data"]["count"] == 5);
  CHECK(j.contains("timestamp"));
}

TEST_CASE("ValidationRequest round-trip JSON") {
  json original = {
    {"input", "hello world"},
    {"min_length", 5},
    {"max_length", 50},
    {"pattern", "^[a-z ]+$"}
  };

  auto parsed = ValidationRequest::from_json(original);
  CHECK(parsed.has_value() == true);

  json serialized = parsed->to_json();
  CHECK(serialized["input"] == "hello world");
  CHECK(serialized["min_length"] == 5);
  CHECK(serialized["max_length"] == 50);
  CHECK(serialized["pattern"] == "^[a-z ]+$");
}
