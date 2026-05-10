# Phase 0 minimal main component.
# Add SRCS / INCLUDE_DIRS / REQUIRES as features land in subsequent phases.

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        # core IDF
        esp_system
        esp_common
        log
        freertos
        spi_flash
)
