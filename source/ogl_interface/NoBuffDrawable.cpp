#include "NoBuffDrawable.hpp"
#include "../utils.hpp"

NoBuffDrawable::NoBuffDrawable(std::string vertPath, std::string fragPath) {
    compileProgramFromFile(program, vertPath.c_str(), fragPath.c_str());
}

NoBuffDrawable::~NoBuffDrawable() {
    glDeleteProgram(program);
}
