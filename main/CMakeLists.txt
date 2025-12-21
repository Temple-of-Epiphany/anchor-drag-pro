# Main component CMakeLists.txt
# Author: Colin Bitterfield
# Email: colin@bitterfield.com
# Date Created: 2025-12-19
# Date Updated: 2025-12-19

idf_component_register(SRCS "main.c"
                            "splash_screen.c"
                            "display_driver.c"
                            "lvgl_init.c"
                       INCLUDE_DIRS ".")
