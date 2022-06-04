#ifndef RENDERING_TASK_HPP
#define RENDERING_TASK_HPP

#include <chrono>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "HemisphereSampler.hpp"
#include "KDTree.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Ray.hpp"
#include "ogl_interface/AGL3Window.hpp"
#include "ogl_interface/Camera.hpp"

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

class alignas(hardware_destructive_interference_size) CacheAlignedCounter {
public:
    unsigned int counter;

    CacheAlignedCounter(unsigned int counter = 0);
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

    RenderingTask(std::string rtcPath, unsigned int nSamples,
                  unsigned int concThreads = std::thread::hardware_concurrency());
    void render() const;
    void preview();
    friend std::ostream &operator<<(std::ostream &os, const RenderingTask *rt);
    void updateRTCFile();
    void buildAccStructures();

private:
    class RTWindow : public AGLWindow {
    public:
        RTWindow(RenderingTask *rt);
        void MainLoop();
        void KeyCB(int key, int scancode, int action, int mods) override;

    private:
        RenderingTask *rt;
        void updateRTCamera(const Camera &camera);
    };
    friend RTWindow;

    std::vector<Material> mats;
    std::vector<Mesh> meshes;
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    /* trianglesToMatIndices[i] corresponds to triangles[i].
     * This cannot be kept in Triangle struct, because Triangle structs are passed to element
     * buffer in OpenGL. */
    std::vector<unsigned int> trianglesToMatIndices;
    std::unique_ptr<KDTree> kdTree;
    /* lightIndices stores indices to triangles vector that have non-zero emission.
     * lightIndices and lightPowersCdf are of equal sizes. */
    std::vector<unsigned int> lightIndices;
    std::vector<float> lightPowersCdf;
    float lightPowersCombined;
    unsigned int concThreads;
    unsigned int nSamples;

    Ray getPrimaryRay(unsigned int px, unsigned int py) const;
    /**
     * @param brdf Takes incoming vector, outgoing vector, surface normal vector and material as
     * parameters.
     */
    glm::vec3 traceRay(const Ray &r, unsigned int maxDepth, std::mt19937 &randEng,
                       const std::function<glm::vec3(const glm::vec3 &, const glm::vec3 &,
                                                     const glm::vec3 &, const Material &)> &brdf,
                       HemisphereSampler &sampler1, HemisphereSampler &sampler2) const;
    bool findNearestIntersection(const Ray &r, float &t, glm::vec3 &n, const Material **mat) const;
    bool isObstructed(const Ray &r, const glm::vec3 &point) const;
    void renderBatch(std::vector<std::vector<glm::vec3>> &pixels,
                     std::queue<unsigned int> &&flatCoordsQueue, CacheAlignedCounter &progress,
                     std::chrono::steady_clock::time_point &ts, std::mutex &tsLock) const;
    void recomputeCameraParams();
    unsigned int getLightIdxFromRndVal(const float rnd) const;
    unsigned int getLightIdxFromRndVal(const float rnd, const unsigned int begin,
                                       const unsigned int end) const;
};

#endif // RENDERING_TASK_HPP
