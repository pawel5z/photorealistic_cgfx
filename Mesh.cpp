#include "Mesh.hpp"

Mesh::Mesh(aiMesh *mesh, const std::vector<Material> &mats) {
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D v = mesh->mVertices[i];
        aiVector3D n = mesh->mNormals[i];
        vertices.push_back({{v.x, v.y, v.z}, {n.x, n.y, n.z}});
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace f = mesh->mFaces[i];
        Triangle t;
        for (unsigned int j = 0; j < f.mNumIndices; j++)
            t.indices[j] = f.mIndices[j];
        triangles.push_back(t);
    }

    mat = &mats[mesh->mMaterialIndex];
}
