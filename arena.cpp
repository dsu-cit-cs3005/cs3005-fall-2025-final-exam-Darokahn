#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <dlfcn.h>
#include <math.h>
#include <time.h>

#include <vector>
#include <map>
#include <iostream>

#include "RobotBase.h"
#include "RadarObj.h"
#include "config.h"

enum displayType {
    display_char=1,
    display_color=2,
    display_texture=4
};

typedef struct {
    char idChar;
    uint32_t color;
    uint32_t* texture;
} displayOptions_t;

bool separateExt(const char* in, char* base, char* ext) {
    int indexOfDot = strlen(in) - 1;
    while (in[indexOfDot] != '.' && indexOfDot >= 0) indexOfDot--;
    if (indexOfDot <= 0) return false;
    strncpy(base, in, indexOfDot);
    base[indexOfDot] = '\0';
    strcpy(ext, in + indexOfDot + 1);
    return true;
}

std::vector<std::string> getCppFiles(const char* dirname, std::vector<std::string> excludes) {
    DIR* thisDir = opendir(dirname);
    if (thisDir == NULL) {
        fprintf(stderr, "ERROR on line %d: %s\n", __LINE__, strerror(errno));
    }
    errno = 0;

    char ext[8];
    char base[256-8];

    std::vector<std::string> filenames;

    for (struct dirent* entry; (entry = readdir(thisDir)) != NULL; errno = 0) {
        if (!separateExt(entry->d_name, base, ext)) continue;
        if (strcmp(ext, "cpp") != 0) continue;
        bool skipEntry = false;
        for (size_t i = 0; i < excludes.size(); i++) {
            if (strcmp(base, excludes[i].c_str()) == 0) {
                skipEntry = true;
                break;
            }
        }
        if (skipEntry) continue;
        filenames.push_back(base);
    }
    closedir(thisDir);
    return filenames;
}

char generateDisplayChar(int iteration) {
    static char characters[] = "$%^&*!?@!=+-_~/><";
    return characters[iteration % ((sizeof characters) - 1)];
}

const char compileString[] = "g++ -fPIC -shared -o %s.so %s.cpp -I. -std=c++20 -g";

void loadRobots(std::vector<void*>& dlsymHandles, std::vector<RobotBase*>& robots, std::vector<std::string> filenames) {
    char sharedFilename[256];
    char compileStringInstance[600];
    for (size_t i = 0; i < filenames.size(); i++) {
        std::string s = filenames[i];
        sprintf(compileStringInstance, compileString, s.c_str(), s.c_str());
        puts(compileStringInstance);
        int status = system(compileStringInstance);
        if (status != 0) {
            fprintf(stderr, "Failed to compile %s\n", s.c_str());
            continue;
        }
        sprintf(sharedFilename, "./%s.so", s.c_str());
        void* handle = dlopen(sharedFilename, RTLD_LAZY);
        if (handle == NULL) {
            fprintf(stderr, "Failed to load %s: %s\n", sharedFilename, dlerror());
            dlclose(handle);
            continue;
        }
        dlsymHandles.push_back(handle);
        RobotFactory createRobot = (RobotFactory)dlsym(handle, "create_robot");
        if (createRobot == NULL) {
            fprintf(stderr, "Failed to find `create_robot` in %s: %s\n", sharedFilename, dlerror());
            continue;
        }
        RobotBase* robot = createRobot();
        if (robot == NULL) {
            fprintf(stderr, "createRobot from file %s failed to produce a valid robot.\n", sharedFilename);
            continue;
        }
        robot->m_name = s;
        robots.push_back(robot);
    }
}

struct rayBox_t {
    int x1;
    int y1;
    int x2;
    int y2;
    double xOffset;
    double yOffset;
    double breadth;
    bool insideDirection = 1;

    rayBox_t(int x1, int y1, int x2, int y2, int breadth, bool baseTerminated, bool endTerminated) {
        (void) baseTerminated;
        (void) endTerminated;
        this->breadth = breadth;
        this->x1 = x1;
        this->y1 = y1;
        double rise = y2 - y1;
        double run = x2 - x1;
        double angle = atan2(rise, run);
        xOffset = sin(angle) * breadth;
        yOffset = cos(angle) * breadth;
    }

    bool contains(int x, int y) {
        x -= x1;
        y -= y1;
        if (yOffset == 0) return (x >= -breadth/2 && x <= breadth/2);
        double yValueCorridor1 = (xOffset/yOffset)*(x + xOffset) + yOffset;
        double yValueCorridor2 = (xOffset/yOffset)*(x - xOffset) - yOffset;
        double yValueWall1 = -(yOffset/xOffset) * x;
        double yValueWall2 = -(yOffset/xOffset) * x - x2 - y2;
        if (y > MAX(yValueCorridor1, yValueCorridor2)) return false;
        if (y < MIN(yValueCorridor1, yValueCorridor2)) return false;
        if (y > MAX(yValueWall1, yValueWall2)) return false;
        if (y < MIN(yValueWall1, yValueWall2)) return false;
        return true;
    }
};

struct arena_t {
    std::vector<char> board;
    std::vector<RobotBase*> robots;
    std::vector<RadarObj> obstacles;
    std::vector<void*> dlsymHandles;
    struct config conf;
    int roundCount;

    void clear() {
        for (size_t i = 0; i < board.size(); i++) {
            board[i] = '.';
        }
    }

    void placeObstacles() {
        int x;
        int y;
        for (int i = 0; i < conf.moundCount; i++) {
            do {
                x = rand() % conf.width;
                y = rand() % conf.height;
            } while (this->at(x, y) != '.');
            this->at(x, y) = 'M';
            obstacles.push_back(RadarObj(x, y, 'M'));
        }
        for (int i = 0; i < conf.pitCount; i++) {
            do {
                x = rand() % conf.width;
                y = rand() % conf.height;
            } while (this->at(x, y) != '.');
            this->at(x, y) = 'P';
            obstacles.push_back(RadarObj(x, y, 'P'));
        }
        for (int i = 0; i < conf.flamethrowerCount; i++) {
            do {
                x = rand() % conf.width;
                y = rand() % conf.height;
            } while (this->at(x, y) != '.');
            this->at(x, y) = 'F';
            obstacles.push_back(RadarObj(x, y, 'F'));
        }
    }

    void initRobots() {
        for (size_t i = 0; i < robots.size(); i++) {
            int x;
            int y;
            do {
                x = rand() % conf.width;
                y = rand() % conf.height;
            } while (this->at(x, y) != '.');
            robots[i]->move_to(y, x);
            robots[i]->m_character = generateDisplayChar(i);
        }
    }

    arena_t(struct config conf): conf(conf) {
        std::srand(time(NULL));
        std::vector<std::string> filenames = getCppFiles(".", {"arena", "RobotBase"});
        loadRobots(dlsymHandles, robots, filenames);
        board.resize(conf.width * conf.height);
        clear();
        placeObstacles();
        initRobots();
        roundCount = 0;
    }

    char& at(int x, int y) {
        return board[y * conf.width + x];
    }

    // depends on `rayBox_t::contains`, which currently doesn't quite work.
    std::vector<RadarObj> scanRay(int x, int y, int dx, int dy, int breadth) {
        (void) x;
        (void) y;
        (void) dx;
        (void) dy;
        (void) breadth;
        std::vector<RadarObj> objects;
        return objects;
    }
    
    // also depends on `rayBox_t::contains`.
    void handleShot(RobotBase* robot, int dx, int dy) {
        (void) dx;
        (void) dy;
        int x, y;
        robot->get_current_location(y, x);
        switch (robot->get_weapon()) {
            case flamethrower:
                break;
            case railgun:
                break;
            case grenade:
                break;
            case hammer:
                break;
        }
    }

    // also depends on `raybox_t::contains`
    void handleMove(RobotBase* robot, int direction, int distance) {
        (void) robot;
        (void) direction;
        (void) distance;
    }

    void shadeBox(rayBox_t box) {
        for (int y = 0; y < conf.height; y++) {
            for (int x = 0; x < conf.width; x++) {
                if (!box.contains(x, y)) continue; 
                this->at(x, y) = 'c';
            }
        }
    }

    void showBoard() {
        for (int y = 0; y < conf.height; y++) {
            for (int x = 0; x < conf.width; x++) {
                printf("%c   ", this->at(x, y));
            }
            printf("\n\n");
        }
    }

    bool processRound() {
        system("clear");

        showBoard();
        printf("ROUND %d\n", roundCount);

        for (size_t i = 0; i < robots.size(); i++) {
            RobotBase* thisRobot = robots[i];
            printf("%c: %s", thisRobot->m_character, thisRobot->print_stats().c_str());
            if (thisRobot->get_health() < 0) {
                printf(" - ELIMINATED\n");
                continue;
            }
            printf("\n");
            int direction;
            thisRobot->get_radar_direction(direction);
            direction = 0;
            int x;
            int y;
            int dx;
            int dy;
            thisRobot->get_current_location(y, x);
            std::pair<int, int> directionPair = directions[direction];
            std::vector<RadarObj> objects = scanRay(x, y, directionPair.first, directionPair.second, 3);
            thisRobot->process_radar_results(objects);
            bool shot = thisRobot->get_shot_location(dy, dx);
            if (shot) handleShot(thisRobot, dx, dy);
            else {
                int distance;
                thisRobot->get_move_direction(direction, distance);
                handleMove(thisRobot, direction, distance);
            }

            if (conf.live) {
                printf("%s\n", "No log for now");
            }

            roundCount++;
        }
        if (conf.live) {
            sleep(1);
        }
        return true;
    }

    ~arena_t() {
        for (size_t i = 0; i < robots.size(); i++) {
            delete robots[i];
        }
        for (size_t i = 0; i < dlsymHandles.size(); i++) {
            dlclose(dlsymHandles[i]);
        }
    }
};

int main() {
    struct config conf;
    loadConfig(&conf, "config");
    printf(stringOf_config, layoutOf_config(conf));
    arena_t arena(conf);
    bool running = true;
    do {
        running = arena.processRound();
    } while (running);
}
