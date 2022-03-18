#include <IL/il.h>
#include <IL/ilu.h>
#include <iostream>

#include "utils.hpp"

void ilLogErrorStack() {
    ILenum errCode;
    while (errCode = ilGetError())
        std::cerr << iluErrorString(errCode) << '\n';
}
