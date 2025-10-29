#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t a = 1664525;
static uint64_t c = 1013904223;
static uint64_t m = (uint64_t)1 << 32;

static uint64_t state = 0;

void lcg_seed(uint64_t seed) {
    state = seed;
}

uint32_t lcg_next32() {
    state = (a * state + c) % m;
    return (uint32_t)state;
}

void lcg_stream_xor(uint8_t *dst, const uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint32_t rnd = lcg_next32();
        uint8_t key_byte = (uint8_t)(rnd & 0xFF);
        dst[i] = src[i] ^ key_byte;
    }
}

int main(void) {
    const char *plaintext = "Hello, this is a test message!";
    size_t len = strlen(plaintext);
    uint8_t *ciphertext = malloc(len);
    uint8_t *recovered   = malloc(len);

    lcg_seed(12345ULL);

    lcg_stream_xor(ciphertext, (const uint8_t*)plaintext, len);

    lcg_seed(12345ULL);
    lcg_stream_xor(recovered, ciphertext, len);

    printf("Plain:  %s\n", plaintext);
    printf("Cipher: ");
    for (size_t i=0; i<len; i++) printf("%02X ", ciphertext[i]);
    printf("\n");
    printf("Recover:%s\n", recovered);

    free(ciphertext);
    free(recovered);
    return 0;
}