# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [Unreleased] — feature/pair-wizard

### Added
- **Pair Wizard** (`devices.js`): new step-by-step UI for pairing a screen.
  - Step 1: puts the ESP32 in scan mode (`GET /api/scan?reset=1`), polls
    every 1.5 s for a new io-homecontrol address, animates a scanning
    indicator, and times out after 60 s with a clear error message.
  - Step 2: shows the detected address, lets the user enter a name, and
    registers the device via `new1W <address> <name>` followed by `add`.
- **Position slider** (`devices.js`): drag-to-set slider on each device card;
  sends `pos1W` on release, updates the fill bar live while dragging.
- **Travel-time label** (`devices.js`): shows ⏱ travel time inline on each
  device card; updates immediately after an edit without requiring a reload.
- **`GET /api/scan`** (`web_server_handler.cpp`): new endpoint — returns
  `{ detected: bool, address: string }` relative to a stored baseline;
  supports `?reset=1` to restart detection.
- **`POST /api/scan/stop`** (`web_server_handler.cpp`): new endpoint — stops
  active scan and clears the baseline.
- **WebSocket auto-reconnect** (`app.js`): exponential back-off reconnect
  (1 s → 2 s → … → 30 s cap) so the UI recovers automatically after WiFi
  dropouts or ESP32 reboots without requiring a page reload.
- **`CHANGELOG.md`**: this file.

### Fixed
- **`new1W` missing address** (`devices.js`): the `new1W` firmware command
  was sent without the detected io-homecontrol address as its first token,
  causing the firmware to register the device against the wrong (or no)
  address. The detected address is now always passed as `new1W <address> <name>`.
- **`dotTimer` closure leak** (`devices.js`): the scanning-dots `setInterval`
  was declared as a `const` local to `showStep1()`, making it unreachable by
  `stopScan()`. The timer was hoisted to wizard-closure scope so every exit
  path (cancel button, timeout, address detected) clears it correctly.
- **`lastFromAddress` data race** (`web_server_handler.cpp`): the raw byte
  array `IOHC::lastFromAddress` was read by the HTTP handler on Core 0 while
  the radio ISR on Core 1 could be writing it concurrently. The read is now
  wrapped in `portENTER_CRITICAL` / `portEXIT_CRITICAL` with a dedicated
  `portMUX_TYPE` spinlock.
- **Hardcoded `"Confirm"` button label** (`popup.js`): the delete-confirm
  button text was hardcoded in English. It now uses
  `app.i18nText("button.confirm", "Confirm")` so it respects the active
  language.
- **`"solar energy"` debug fallback** (`popup.js`): the `booleanLabel`
  fallback string was accidentally left as `"solar energy"` (a debug
  placeholder). Changed to `app.i18nText("popup.boolean_label_default", "Active")`.
- **`loadJson` unhandled rejection** (`i18n.js`): `fetch()` could throw
  (network error, DNS failure during ESP32 boot) and crash the entire i18n
  initialisation, leaving all `[data-i18n]` labels empty. Wrapped in
  `try/catch`; returns `{}` on any error so the UI degrades gracefully.

---

## [Prior — master baseline] — 2026-06-22

### Added
- MQTT cover state synchronisation: position and state
  (OPENING / CLOSING / OPEN / CLOSED / STOP) published as the blind moves,
  giving real-time feedback in Home Assistant.
- Position persistence across reboots with atomic LittleFS save to prevent
  corruption on power loss.
- MQTT publishes current position on reconnect.
- Version information in MQTT and web server (`/api/info`).
- Latest firmware version lookup from GitHub releases.
- HTTPS certificate verification for remote requests.
- Display enable/disable toggle via web UI (`/api/display`).
- Syslog support (`/api/syslog`, `/api/syslog/test`).

### Fixed
- OLED and LilyGo T3-S3 bug fixes.
- `packet->repeat` used correctly in `startQueuedSend` log (was
  `iohc->repeat`).
- Removed premature `packet->repeat--` decrement in `startQueuedSend`.
- Use local `packet` variable instead of `radio->iohc` in `onTxTicker`.
- Removed delete loop on TX complete (callback queue owns the packets).
- SSD1306 display driver fix.

---

[Unreleased]: https://github.com/NH-Networks/iohomecontrol/compare/master...feature/pair-wizard
