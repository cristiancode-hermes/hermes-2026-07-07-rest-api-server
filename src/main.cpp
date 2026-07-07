#include <restapi/server.hpp>
#include <restapi/routes.hpp>
#include <restapi/middleware.hpp>
#include <restapi/models.hpp>

#include <csignal>
#include <atomic>
#include <cstdlib>
#include <iostream>

using namespace restapi;

namespace {

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
  spdlog::warn("Señal {} recibida, cerrando servidor...", signal);
  g_running.store(false);
}

void print_banner(const Config& cfg) {
  std::cout << R"(
   ____       _     ____  ____        _    ____
  / ___|  ___| |_  |  _ \/ ___|  ___| |_ / ___|  ___ _ ____   _____ _ __
  \___ \ / _ \ __| | |_) \___ \ / _ \ __|\___ \ / _ \ '__\ \ / / _ \ '__|
   ___) |  __/ |_  |  _ < ___) |  __/ |_  ___) |  __/ |   \ V /  __/ |
  |____/ \___|\__| |_| \_\____/ \___|\__|____/ \___|_|    \_/ \___|_|
)" << '\n';
  spdlog::info("╔══════════════════════════════════════════════╗");
  spdlog::info("║  REST API Server v1.0.0                      ║");
  spdlog::info("║  Host: {}:{}", cfg.host, cfg.port);
  spdlog::info("║  C++20 + cpp-httplib + nlohmann/json         ║");
  spdlog::info("║  Log level: {}", cfg.log_level);
  spdlog::info("╚══════════════════════════════════════════════╝");
}

} // namespace

auto main(int argc, char* argv[]) -> int {
  // ── Señales ────────────────────────────────────────────────────
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // ── Configuración ──────────────────────────────────────────────
  Config cfg = Config::from_env();

  // Permitir -c config.json desde CLI
  for (int i = 1; i < argc; ++i) {
    if (std::string_view arg{argv[i]}; arg == "-c" || arg == "--config") {
      if (i + 1 < argc) {
        auto result = Config::from_file(argv[++i]);
        if (result.has_value()) {
          cfg = std::move(result.value());
          spdlog::info("Configuración cargada desde: {}", argv[i]);
        } else {
          spdlog::error("Error cargando configuración: {}", result.error());
          return EXIT_FAILURE;
        }
      }
    }
  }

  // ── Logger ─────────────────────────────────────────────────────
  spdlog::set_level(spdlog::level::from_str(cfg.log_level));
  spdlog::set_pattern(cfg.log_pattern);

  // ── Banner ────────────────────────────────────────────────────
  print_banner(cfg);

  // ── Servidor ──────────────────────────────────────────────────
  Server server(cfg);

  try {
    server.setup();

    // Ejecutar en hilo separado para poder capturar SIGINT
    std::thread server_thread([&server, &cfg]() {
      auto result = server.run();
      if (!result.has_value()) {
        spdlog::error("Servidor terminó con error");
        g_running.store(false);
      }
    });

    // Bucle principal esperando señal de parada
    while (g_running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.stop();
    if (server_thread.joinable()) {
      server_thread.join();
    }

  } catch (const std::exception& e) {
    spdlog::critical("Error fatal: {}", e.what());
    return EXIT_FAILURE;
  }

  spdlog::info("Servidor finalizado correctamente.");
  return EXIT_SUCCESS;
}
