# AGENTS

This repository is an ESP32 Arduino firmware project using PlatformIO.
Use the guidance below when making changes or running commands.

## Build, Upload, Lint, Test

Project environment is defined in `platformio.ini`.
Default env: `esp32doit-devkit-v1`.

Common commands (run from repo root):
- Build firmware: `pio run -e esp32doit-devkit-v1`
- Upload firmware: `pio run -e esp32doit-devkit-v1 -t upload`
- Clean build: `pio run -e esp32doit-devkit-v1 -t clean`
- Monitor serial: `pio device monitor -e esp32doit-devkit-v1`

Testing with PlatformIO Test Runner:
- Run all tests: `pio test -e esp32doit-devkit-v1`
- Run a single test folder:
  `pio test -e esp32doit-devkit-v1 -f test/master_uart_simulation`
  (you can also pass just the suite name: `-f master_uart_simulation`)

Static analysis (if needed):
- `pio check -e esp32doit-devkit-v1`
  Note: there is no explicit configuration file in this repo.

Upload notes:
- `platformio.ini` uses `scripts/auto_uploader.py` to prefer serial
  and fall back to OTA when available.
- Default OTA host is `ph-remote.local` (configurable via
  `custom_ota_host` in `platformio.ini`).

## Repository Layout

- `src/` main firmware entry (`main.cpp`) and core logic.
- `include/` shared headers such as `globals.h`.
- `lib/` local libraries (project-specific) and vendored third-party libs.
- `test/` PlatformIO tests (each suite has a `main.cpp`).

## Code Style and Conventions

General
- Language: C++ (Arduino framework, ESP32).
- Do not use exceptions; prefer `bool` return + `lastError()` getters.
- Avoid heavy dynamic allocation in hot paths; prefer stack buffers or
  fixed-size arrays (`char[]`, `snprintf`).
- Use `F("...")` for constant strings stored in flash when appropriate.
- Prefer explicit `uint8_t`, `uint16_t`, `uint32_t`, `int32_t` types for
  hardware-facing code.

Formatting
- Keep existing formatting in each file; do not reformat unrelated lines.
- Indentation in `src/` is commonly 2 spaces (follow local file style).
- Braces are K&R style in most project files.
- Long calls are often wrapped with aligned parameters; keep readability.

Includes
- Project headers first with quotes: `#include "..."`.
- Third-party and Arduino headers after, with angle brackets.
- Keep include order stable within a file; avoid unused includes.

Naming
- Classes and structs: `PascalCase` (e.g., `PumpsManager`).
- Methods and functions: `lowerCamelCase` (e.g., `readSingleRaw`).
- Enums: `enum class` with `PascalCase` values.
- Constants/macros: `UPPER_SNAKE_CASE` (e.g., `TELNET_HOSTNAME`).
- Private members often end with `_` (e.g., `gain_`, `shadow_`).

Globals and Singletons
- Global objects are declared in `include/globals.h` and defined once in
  `src/globals.cpp`. Do not create additional global definitions elsewhere.
- Prefer dependency injection where practical (pass pointers or refs).

Error Handling and Logging
- Return `false` on failure and set `last_error_` or similar fields.
- Use `remoteManager.log(...)` for user-visible logging.
- Keep user-facing messages short; avoid large allocations.

Timing and IO
- Use `delay(...)` sparingly and document why when used.
- For sensor reads, validate ranges before using results (see `readPH()`).
- When handling buttons, clear latches after consuming events.

EEPROM and Persistent Data
- Follow `ConfigStore` patterns: read -> validate -> write -> `save()`.
- Ensure any calibration write updates both runtime values and EEPROM.

JSON Handling
- Use `ArduinoJson` with `JsonDocument` and handle `DeserializationError`.
- Validate keys before reading; clamp values to safe ranges.

Concurrency (ESP32)
- Tasks are created with `xTaskCreatePinnedToCore`.
- Keep task loops short and include `vTaskDelay` to avoid hogging CPU.

## Working with `lib/`

- `lib/` contains both local modules and vendored third-party libraries.
- Avoid editing vendored third-party code unless necessary for a fix.
- Prefer changes in `src/` or project-owned `lib/<name>/` modules.

## Testing Guidance

- Tests live in `test/<suite>/main.cpp`.
- Use the `-f` flag to run a single test suite.
- Some tests are hardware-dependent; document any requirements in
  `test/<suite>/README` if you add new tests.

## Cursor/Copilot Rules

- No Cursor rules found in `.cursor/rules/` or `.cursorrules`.
- No Copilot rules found in `.github/copilot-instructions.md`.

## Safety and Hygiene

- Do not commit or edit secrets; prefer `platformio.ini` for config.
- Be cautious with pin definitions and hardware constants in
  `include/globals.h`.
- Avoid changing OTA or serial settings without a clear rationale.
