/* Outils communs aux tests sur l'hôte. Voir helpers.h. */
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RETIMER_REFERENCES
#define RETIMER_REFERENCES "references"
#endif
#ifndef RETIMER_OUTPUTS
#define RETIMER_OUTPUTS "screens"
#endif

bool helpers_write_pbm(const char *path, const uint8_t *bytes, int width,
                       int height)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        return false;
    }
    size_t size = (size_t)((width + 7) / 8) * (size_t)height;
    fprintf(f, "P4\n%d %d\n", width, height);
    bool ok = fwrite(bytes, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

/* Lit un entier de l'en-tête PBM, en sautant blancs et commentaires. */
static bool read_int(FILE *f, int *value)
{
    int c = fgetc(f);
    while (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '#') {
        if (c == '#') {
            while (c != '\n' && c != EOF) {
                c = fgetc(f);
            }
        }
        c = fgetc(f);
    }
    if (c < '0' || c > '9') {
        return false;
    }
    int v = 0;
    while (c >= '0' && c <= '9') {
        v = v * 10 + (c - '0');
        c = fgetc(f);
    }
    *value = v; /* c est le blanc unique qui précède les données */
    return true;
}

bool helpers_read_pbm(const char *path, uint8_t *bytes, int width,
                      int height)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return false;
    }
    char magic[2];
    int l = 0;
    int h = 0;
    bool ok = fread(magic, 1, 2, f) == 2 && magic[0] == 'P' &&
              magic[1] == '4' && read_int(f, &l) && read_int(f, &h) &&
              l == width && h == height;
    if (ok) {
        size_t size = (size_t)((width + 7) / 8) * (size_t)height;
        ok = fread(bytes, 1, size, f) == size;
    }
    fclose(f);
    return ok;
}

long helpers_snapshot(const char *name, const uint8_t *bytes, int width,
                      int height)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.pbm", RETIMER_OUTPUTS, name);
    helpers_write_pbm(path, bytes, width, height);

    snprintf(path, sizeof path, "%s/%s.pbm", RETIMER_REFERENCES, name);
    const char *update_env = getenv("RETIMER_UPDATE_REFERENCES");
    if (update_env != NULL && strcmp(update_env, "1") == 0) {
        helpers_write_pbm(path, bytes, width, height);
        return 0;
    }

    size_t size = (size_t)((width + 7) / 8) * (size_t)height;
    uint8_t *reference = malloc(size);
    if (reference == NULL) {
        return -1;
    }
    long differences = -1;
    if (helpers_read_pbm(path, reference, width, height)) {
        differences = 0;
        for (size_t i = 0; i < size; i++) {
            uint8_t x = (uint8_t)(reference[i] ^ bytes[i]);
            while (x != 0) {
                differences += x & 1u;
                x >>= 1;
            }
        }
    }
    free(reference);
    return differences;
}
