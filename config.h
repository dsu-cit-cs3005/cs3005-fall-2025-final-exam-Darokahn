#pragma once

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

struct config {
    uint64_t displayTypes;
    int width;
    int height;
    int moundCount;
    int pitCount;
    int flamethrowerCount;
    bool live;
};

static inline bool storeConfig(struct config* c, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "failed to store config: %s\n", strerror(errno));
        return false;
    }
    fwrite(c, sizeof(*c), 1, f);
    fclose(f);
    return true;
}

static inline bool loadConfig(struct config* c, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "failed to load config: %s\n", strerror(errno));
        return false;
    }
    fread(c, sizeof(*c), 1, f);
    fclose(f);
    return true;
}

#define stringOf_config "displayTypes: %lu\nwidth: %d\nheight: %d\nmoundCount: %d\npitCount: %d\nflamethrowerCount: %d\nlive: %d\n"
#define layoutOf_config(name) name.displayTypes, name.width, name.height, name.moundCount, name.pitCount, name.flamethrowerCount, name.live
