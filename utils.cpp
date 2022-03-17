#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>

#include "utils.hpp"

void ilLogErrorStack() {
    ILenum errCode;
    while (errCode = ilGetError())
        std::cerr << iluErrorString(errCode) << '\n';
}
