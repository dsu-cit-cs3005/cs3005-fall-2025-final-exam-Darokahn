#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <vector>

#include "RobotBase.h"

struct config {};

void loadConfig(struct config* c, const char* filename) {
}

const char compileString[] = "g++ -shared -fPIC -o %s.so %s.cpp RobotBase.o -I -std=c++20";

bool separateExt(const char* in, char* base, char* ext) {
    int indexOfDot = strlen(in) - 1;
    while (in[indexOfDot] != '.' && indexOfDot >= 0) indexOfDot--;
    if (indexOfDot == 0) return false;
    strncpy(base, in, indexOfDot);
    base[indexOfDot] = '\0';
    strcpy(ext, in + indexOfDot + 1);
    return true;
}

int main() {
    struct config c;
    loadConfig(&c, "config");
    DIR* thisDir = opendir(".");
    if (thisDir == NULL) {
        fprintf(stderr, "ERROR on line %d: %s\n", __LINE__, strerror(errno));
    }
    errno = 0;
    std::vector<RobotBase> robots;
    for (struct dirent* entry; (entry = readdir(thisDir)) != NULL; errno = 0) {
        char base[256];
        char ext[8];
        if (!separateExt(entry->d_name, base, ext)) continue;
        if (strcmp(ext, "cpp") != 0) continue;
        if (strcmp(base, "arena") == 0) continue;
        printf("base: %s, ext: %s\n", base, ext);
        char compileStringInstance[600];
        sprintf(compileStringInstance, compileString, base, base);
        puts(compileStringInstance);
        int status = system(compileStringInstance);
    }
}
