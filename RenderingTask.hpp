#ifndef RENDERING_TASK_HPP
#define RENDERING_TASK_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Light.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "ogl_interface/AGL3Window.hpp"
#include "ogl_interface/Camera.hpp"

struct Ray {
    glm::vec3 o; // origin
    glm::vec3 d; // direction
};

class RenderingTask {
public:
    std::string rtcPath;
    // given in rtc file
    std::string origObjPath;
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
    bool renderPreview = false;

    RenderingTask(std::string path);
    void render() const;
    void preview();
    friend std::ostream &operator<<(std::ostream &os, const RenderingTask *rt);
    void updateRTCFile();

private:
    class RTWindow;
    friend RTWindow;

    std::vector<Light> lights;
    std::vector<Material> mats;
    std::vector<Mesh> meshes;

    Ray getPrimaryRay(unsigned int px, unsigned int py) const;
    glm::vec3 traceRay(const Ray &r, unsigned int maxDepth) const;
    bool findNearestIntersection(const Ray &r, float &t, glm::vec3 &n, const Material **mat) const;
    bool isObstructed(const Ray &r) const;
    void renderBatch(std::vector<unsigned char> &imgData, unsigned int from, unsigned int count,
                     unsigned int &progress) const;
    void recomputeCameraParams();
};

class RenderingTask::RTWindow : public AGLWindow {
public:
    RTWindow(RenderingTask *rt);

    void MainLoop();
    void KeyCB(int key, int scancode, int action, int mods) override;

private:
    RenderingTask *rt;
    void updateRTCamera(const Camera &camera);
};

#endif // RENDERING_TASK_HPP
