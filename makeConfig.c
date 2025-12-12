#include "config.h"

int main() {
    struct config c =
        #include "config.struct"
    ;
    storeConfig(&c, "config");
}
