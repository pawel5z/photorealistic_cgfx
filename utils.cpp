#include <glm/glm.hpp>
#include <iostream>

#include "utils.hpp"

void ilLogErrorStack() {
    ILenum errCode;
    while (errCode = ilGetError())
        std::cerr << iluErrorString(errCode) << '\n';
}

// >>> adapted from https://ii.uni.wroc.pl/~anl/dyd/RGK/files/triangle_moller.c
bool intersect_triangle(const glm::vec3 &orig, const glm::vec3 &dir,
                        const glm::vec3 &vert0, const glm::vec3 &vert1, const glm::vec3 &vert2,
                        float *t, float *u, float *v) {
    glm::vec3 edge1, edge2, tvec, pvec, qvec;
    float det, inv_det;

    /* find vectors for two edges sharing vert0 */
    edge1 = vert1 - vert0;
    edge2 = vert2 - vert0;

    /* begin calculating determinant - also used to calculate U parameter */
    pvec = glm::cross(dir, edge2);

    /* if determinant is near zero, ray lies in plane of triangle */
    det = glm::dot(edge1, pvec);

#ifdef TEST_CULL /* define TEST_CULL if culling is desired */
    if (det < EPSILON)
        return false;

    /* calculate distance from vert0 to ray origin */
    tvec = orig - vert0;

    /* calculate U parameter and test bounds */
    *u = glm::dot(tvec, pvec);
    if (*u < 0.0 || *u > det)
        return false;

    /* prepare to test V parameter */
    glm::cross(tvec, edge1);

    /* calculate V parameter and test bounds */
    *v = glm::dot(dir, qvec);
    if (*v < 0.0 || *u + *v > det)
        return false;

    /* calculate t, scale parameters, ray intersects triangle */
    *t = glm::dot(edge2, qvec);
    inv_det = 1.0 / det;
    *t *= inv_det;
    *u *= inv_det;
    *v *= inv_det;
#else /* the non-culling branch */
    if (det > -EPSILON && det < EPSILON)
        return false;
    inv_det = 1.0 / det;

    /* calculate distance from vert0 to ray origin */
    tvec = orig - vert0;

    /* calculate U parameter and test bounds */
    *u = glm::dot(tvec, pvec) * inv_det;
    if (*u < 0.0 || *u > 1.0)
        return false;

    /* prepare to test V parameter */
    glm::cross(tvec, edge1);

    /* calculate V parameter and test bounds */
    *v = glm::dot(dir, qvec) * inv_det;
    if (*v < 0.0 || *u + *v > 1.0)
        return false;

    /* calculate t, ray intersects triangle */
    *t = glm::dot(edge2, qvec) * inv_det;
#endif
    return true;
}
// >>> adapted from https://ii.uni.wroc.pl/~anl/dyd/RGK/files/triangle_moller.c
