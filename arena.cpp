#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include <vector>
#include <iostream>

#include "RobotBase.h"

enum displayType {
    display_char=1,
    display_color=2,
    display_texture=4
};

struct config {
    int arenaWidth;
    int arenaHeight;
    int moundCount;
    int pitCount;
    int flamethrowerCount;
    bool live;
    uint64_t displayTypes;
};

void loadConfig(struct config* c, const char* filename) {
}

typedef struct {
    char idChar;
    uint32_t color;
    uint32_t* texture;
} displayOptions_t;

const char compileString[] = "g++ -fPIC -shared -o %s.so %s.cpp RobotBase.o -I -std=c++20";

bool separateExt(const char* in, char* base, char* ext) {
    int indexOfDot = strlen(in) - 1;
    while (in[indexOfDot] != '.' && indexOfDot >= 0) indexOfDot--;
    if (indexOfDot == 0) return false;
    strncpy(base, in, indexOfDot);
    base[indexOfDot] = '\0';
    strcpy(ext, in + indexOfDot + 1);
    return true;
}

void loadRobots(std::vector<RobotBase*>& robots) {
    DIR* thisDir = opendir(".");
    if (thisDir == NULL) {
        fprintf(stderr, "ERROR on line %d: %s\n", __LINE__, strerror(errno));
    }
    errno = 0;

    char ext[8];
    char base[256-8];
    char sharedFilename[256];
    char compileStringInstance[600];

    for (struct dirent* entry; (entry = readdir(thisDir)) != NULL; errno = 0) {
        if (!separateExt(entry->d_name, base, ext)) continue;
        if (strcmp(ext, "cpp") != 0) continue;
        if (strcmp(base, "arena") == 0) continue;
        if (strcmp(base, "RobotBase") == 0) continue;
        printf("base: %s, ext: %s\n", base, ext);
        sprintf(compileStringInstance, compileString, base, base);
        puts(compileStringInstance);
        int status = system(compileStringInstance);
        if (status != 0) {
            fprintf(stderr, "Failed to compile %s\n", base);
        }
        sprintf(sharedFilename, "./%s.so", base);
        void* handle = dlopen(sharedFilename, RTLD_LAZY);
        if (handle == NULL) {
            fprintf(stderr, "Failed to load %s: %s\n", sharedFilename, dlerror());
            dlclose(handle);
            continue;
        }
        RobotFactory createRobot = (RobotFactory)dlsym(handle, "create_robot");
        if (createRobot == NULL) {
            fprintf(stderr, "Failed to find `create_robot` in %s: %s\n", sharedFilename, dlerror());
        }
        RobotBase* robot = createRobot();
        if (robot == NULL) {
            fprintf(stderr, "createRobot from file %s failed to produce a valid robot.\n", sharedFilename);
        }
        robots.push_back(robot);
        dlclose(handle);
    }
    closedir(thisDir);
}

char generateDisplay(int iteration) {
    static char characters[] = "$%^&*!?@!=+-_~/.,><";
    return characters[iteration % (sizeof characters) - 1];
}

int main() {
    struct config conf;
    loadConfig(&conf, "config");
    std::vector<char> arena;
    std::vector<RobotBase*> robots;
    loadRobots(robots);
    std::vector<char> robotDisplays;
    for (int i = 0; i < robots.size(); i++) {
        robotDisplays.push_back(generateDisplay(i));
    }
}
