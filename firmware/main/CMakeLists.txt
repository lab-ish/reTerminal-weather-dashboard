idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES wifi_conn time_sync battery http_fetch
                                epd_ed2208 framebuffer battery_overlay power_mgr)
