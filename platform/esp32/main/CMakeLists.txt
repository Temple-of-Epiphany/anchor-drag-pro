# Phase 3 — adds TOML config parser (vendored tomlc99) alongside i2c_bus,
# ch422g, sd_card, ota.

idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "ch422g.c"
        "sd_card.c"
        "ota.c"
        "anchor_config.c"
        "vendor/tomlc99/toml.c"
    INCLUDE_DIRS
        "."
        "vendor/tomlc99"
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
