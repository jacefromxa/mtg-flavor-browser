idf_component_register(
    SRCS "main.c"
         "mtg_cards.c"
         "mtg_state.c"
         "mtg_mana.c"
         "mtg_ui.c"
         "mtg_fetch.c"
         "mtg_blufi.c"
         "mtg_blufi_security.c"
         "mtg_wifi_prov.c"
         "mtg_radio.c"
         "../assets/fonts/mtg_font_16.c"
         "../assets/fonts/mtg_font_serif_20.c"
         "../assets/fonts/mtg_font_inventory.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi esp_http_client esp_http_server mbedtls nvs_flash bootloader_support json
)
