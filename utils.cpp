#include "utils.hpp"

void ilLogErrorStack() {
    ILenum errCode;
    while (errCode = ilGetError())
        std::cerr << iluErrorString(errCode) << '\n';
}
