idf_component_register(SRCS "power_mgr.c" "power_calc.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_hw_support board_cfg)
