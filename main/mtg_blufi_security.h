#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void mtg_blufi_negotiate(uint8_t *data, int len, uint8_t **output_data,
                         int *output_len, bool *need_free);
int mtg_blufi_encrypt(uint8_t iv8, uint8_t *data, int len);
int mtg_blufi_decrypt(uint8_t iv8, uint8_t *data, int len);
uint16_t mtg_blufi_checksum(uint8_t iv8, uint8_t *data, int len);
int mtg_blufi_security_init(void);
void mtg_blufi_security_deinit(void);

#ifdef __cplusplus
}
#endif
