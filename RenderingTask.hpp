#ifndef RENDERING_TASK_HPP
#define RENDERING_TASK_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Light.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

struct Ray {
    glm::vec3 o; // origin
    glm::vec3 d; // direction
};

class RenderingTask {
public:
    // given in rtc file
    std::string outputPath = "out.png";
    unsigned int recLvl;
    unsigned int width, height;
    glm::vec3 viewPoint;
    glm::vec3 lookAt;
    glm::vec3 up = {0, 1, 0};
    float yView = 1;
    // inherited from above file
    glm::vec3 front;
    glm::vec3 right;

    RenderingTask(std::string path);
    void render();

private:
    std::vector<Light> lights;
    std::vector<Material> mats;
    std::vector<Mesh> meshes;

    Ray getPrimaryRay(unsigned int px, unsigned int py);
    glm::vec3 traceRay(Ray r, unsigned int maxDepth);
};

#endif // RENDERING_TASK_HPP
