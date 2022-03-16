#ifndef RENDERING_TASK_HPP
#define RENDERING_TASK_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Light.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

class RenderingTask {
public:
    std::string outputPath = "out.png";
    unsigned int recLvl;
    glm::vec2 res;
    glm::vec3 viewPoint;
    glm::vec3 lookAt;
    glm::vec3 up = {0, 1, 0};
    float yView = 1;

    RenderingTask(std::string path);

private:
    std::vector<Light> lights;
    std::vector<Material> mats;
    std::vector<Mesh> meshes;
};

#endif // RENDERING_TASK_HPP
