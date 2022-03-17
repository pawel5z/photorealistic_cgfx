#include <iostream>

#include "RenderingTask.hpp"

int main(int argc, const char *argv[]) {
    if (argc <= 1) {
        std::cerr << "Usage: ./raytrace config_file\n";
        exit(EXIT_FAILURE);
    }

    RenderingTask rt(argv[1]);
    rt.render();
    return 0;
}
