# Main component CMakeLists.txt
# Author: Colin Bitterfield
# Email: colin@bitterfield.com
# Date Created: 2025-12-19
# Date Updated: 2025-12-22

idf_component_register(SRCS "main.c"
                            "splash_screen.c"
                            "splash_logo.c"
                            "tv_test_pattern.c"
                            # Excluded - LVGL 9.x specific (LV_COLOR_FORMAT_RGB565)
                            # "smpte_test_pattern.c"
                            # "smpte_test_screen.c"
                            "start_screen.c"
                            "simple_test_screen.c"
                            "display_driver.c"
                            "display_test.c"
                            "lvgl_init.c"
                            "touch_driver.c"
                            "ui_header.c"
                            "ui_footer.c"
                            "screens.c"
                            "datetime_settings.c"
                            "rtc_pcf85063a.c"
                            "power_management.c"
                            "esp_io_expander_ch422g.c"
                            "ch422g.c"
                            "sd_card.c"
                            # Custom fonts - Orbitron (futuristic/technical) - 16, 20, 24pt only
                            "fonts/orbitron_variablefont_wght_16.c"
                            "fonts/orbitron_variablefont_wght_20.c"
                            "fonts/orbitron_variablefont_wght_24.c"
                            # Removed: 32pt, 48pt (unused)
                            # Custom fonts - Poppins (clean, readable) - 16, 20, 24pt only
                            "fonts/poppins_regular_16.c"
                            "fonts/poppins_regular_20.c"
                            "fonts/poppins_regular_24.c"
                            # Removed: 32pt, 48pt (unused)
                            # Custom fonts - GolosText (geometric, excellent readability) - 16, 20, 24pt only
                            "fonts/golostext_regular_16.c"
                            "fonts/golostext_regular_20.c"
                            "fonts/golostext_regular_24.c"
                            # Removed: 32pt, 48pt (unused)
                            # Custom fonts - Apple Symbols (icons) - 32pt only (helm symbol ⎈ U+2388)
                            "fonts/apple_symbols_32.c"
                            # Removed: 16pt, 24pt (unused)
                            # SF Pro Rounded - Removed (unused)
                            # "fonts/sfnsrounded_16.c"
                            # "fonts/sfnsrounded_20.c"
                            # "fonts/sfnsrounded_24.c"
                            # "fonts/sfnsrounded_32.c"
                            # "fonts/sfnsrounded_48.c"
                       INCLUDE_DIRS "."
                       REQUIRES fatfs console sdmmc vfs spi_flash nvs_flash)
