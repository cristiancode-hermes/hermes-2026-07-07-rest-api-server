cmake_minimum_required(VERSION 3.28)
project(restapi-server VERSION 1.0.0 LANGUAGES CXX)

# ── C++ Standard ─────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ──────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# ── FetchContent: Dependencies ───────────────────────────────────────────
include(FetchContent)

# cpp-httplib (header-only)
FetchContent_Declare(
  cpp-httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.2
)
set(HTTPLIB_USE_NONBLOCKING_IF_AVAILABLE ON CACHE BOOL "")

# nlohmann/json (header-only)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.1
)

FetchContent_MakeAvailable(cpp-httplib nlohmann_json spdlog)

# ── Sources ──────────────────────────────────────────────────────────────
set(SOURCES
  src/main.cpp
  src/server.cpp
  src/routes.cpp
)

set(PUBLIC_HEADERS
  include/restapi/server.hpp
  include/restapi/routes.hpp
  include/restapi/models.hpp
  include/restapi/middleware.hpp
)

# ── Executable ───────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
  ${SOURCES}
  ${PUBLIC_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  httplib::httplib
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)

# ── Compiler warnings ───────────────────────────────────────────────────
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall -Wextra -Wpedantic
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
    -Woverloaded-virtual -Wformat=2
  >
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer
  )
  target_link_options(${PROJECT_NAME} PRIVATE
    -fsanitize=address,undefined
  )
endif()

# ── Tests ────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
  enable_testing()

  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
  )
  FetchContent_MakeAvailable(doctest)

  add_executable(${PROJECT_NAME}_tests
    tests/test_main.cpp
    tests/test_routes.cpp
  )

  target_include_directories(${PROJECT_NAME}_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    doctest::doctest
    httplib::httplib
    nlohmann_json::nlohmann_json
    spdlog::spdlog
  )

  target_compile_options(${PROJECT_NAME}_tests PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
      -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
    >
  )

  include(CTest)
  add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif()

# ── Install ──────────────────────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
