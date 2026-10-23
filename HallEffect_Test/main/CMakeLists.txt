idf_component_register(
    SRCS "HallEffect_Test.c"
    INCLUDE_DIRS "."
    REQUIRES esp_adc esp_driver_gpio
)