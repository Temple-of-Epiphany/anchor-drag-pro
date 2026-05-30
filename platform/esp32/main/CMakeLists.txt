# Phase B (#69) — adds LVGL infrastructure (display + LVGL init + minimal
# splash) on top of the Phase 3 base (i2c_bus, ch422g, sd_card, ota,
# anchor_config + vendored tomlc99).

idf_component_register(
    SRCS
        # Core firmware
        "main.c"
        "i2c_bus.c"
        "ch422g.c"
        "sd_card.c"
        "ota.c"
        "anchor_config.c"
        "vendor/tomlc99/toml.c"

        # Phase B graphics (#69) — display driver, touch stub, LVGL init,
        # token-driven splash screen, persistent chrome shells, modal stack
        "display_driver.c"
        "touch_driver.c"
        "lvgl_init.c"
        "screen_splash.c"
        "screen_monitor.c"
        "screen_connections.c"
        "screen_info.c"
        "screen_settings.c"
        "screen_diagnostics.c"
        "screen_nav.c"
        "ui_chrome.c"
        "ui_modals.c"
        "ui_confirm.c"
        "ui_preset_picker.c"
        "ui_alarm.c"
        "ui_ota.c"
        "ui_wifi_password.c"
        "screen_wifi.c"

        # Phase 5+ networking
        "wifi_manager.c"
        "nmea0183.c"
        "gps_source.c"
        "tcp_gateway.c"

        # Anchor logic — pure C geometry + state machine
        "anchor_geo.c"
        "anchor_state.c"

        # Brand assets (#79)
        "assets/generated/anchor_logo_140.c"
    INCLUDE_DIRS
        "."
        "vendor/tomlc99"
        "../../../core/include"          # cross-platform design tokens + strings
    REQUIRES
        # core IDF
        esp_system
        esp_common
        log
        freertos
        spi_flash
        esp_timer
        nvs_flash

        # peripherals
        driver

        # SD card stack
        fatfs
        sdmmc
        esp_driver_sdmmc

        # OTA
        app_update
        esp_partition
        mbedtls

        # Phase B graphics
        esp_lcd

        # Phase 5 networking
        esp_wifi
        esp_event
        esp_netif
        lwip
)

# Vendored tomlc99 uses isdigit(p[i]) where p[i] is a plain char. ESP-IDF
# treats -Wchar-subscripts as an error, which would block the build. The
# warning is correct (signed char as array subscript is undefined) but
# benign in tomlc99's usage. Suppress the warning only for that file
# rather than disabling it project-wide.
set_source_files_properties(
    "vendor/tomlc99/toml.c"
    PROPERTIES COMPILE_OPTIONS "-Wno-char-subscripts;-Wno-error=char-subscripts"
)
