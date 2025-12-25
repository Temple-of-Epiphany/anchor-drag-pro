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
                            "rtc_pcf85063a.c"
                            # Custom fonts - Orbitron (futuristic/technical)
                            "fonts/orbitron_variablefont_wght_16.c"
                            "fonts/orbitron_variablefont_wght_20.c"
                            "fonts/orbitron_variablefont_wght_24.c"
                            "fonts/orbitron_variablefont_wght_32.c"
                            "fonts/orbitron_variablefont_wght_48.c"
                            # Custom fonts - Poppins (clean, readable)
                            "fonts/poppins_regular_16.c"
                            "fonts/poppins_regular_20.c"
                            "fonts/poppins_regular_24.c"
                            "fonts/poppins_regular_32.c"
                            "fonts/poppins_regular_48.c"
                            # Custom fonts - GolosText (geometric, excellent readability)
                            "fonts/golostext_regular_16.c"
                            "fonts/golostext_regular_20.c"
                            "fonts/golostext_regular_24.c"
                            "fonts/golostext_regular_32.c"
                            "fonts/golostext_regular_48.c"
                            # Custom fonts - Apple Symbols (icons)
                            "fonts/apple_symbols_16.c"
                            "fonts/apple_symbols_24.c"
                            "fonts/apple_symbols_32.c"
                            # Custom fonts - SF Pro Rounded (Apple's rounded system font)
                            "fonts/sfnsrounded_16.c"
                            "fonts/sfnsrounded_20.c"
                            "fonts/sfnsrounded_24.c"
                            "fonts/sfnsrounded_32.c"
                            "fonts/sfnsrounded_48.c"
                       INCLUDE_DIRS ".")
