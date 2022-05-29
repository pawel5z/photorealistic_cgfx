#include "Axes.hpp"

Axes::Axes(std::string vertPath, std::string fragPath) : NoBuffDrawable(vertPath, fragPath) {}

void Axes::draw(const Camera &camera) const
{
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, false, &camera.getPVMat()[0][0]);
    glDrawArrays(GL_LINES, 0, 6);
}
