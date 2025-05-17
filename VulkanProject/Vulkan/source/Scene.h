#include <string>
#include <vector>
#include <filesystem>
#include "ResourceManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Scene {
public:
    Scene(ResourceManager* resourceManager);

    void LoadScene(const std::string& scenePath);

    MeshHandle LoadObjModel(const std::string& modelPath, const std::filesystem::path& baseDir,const aiScene* scene);

    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);

    const aiNode* FindMeshNode(const aiNode* node, unsigned int meshIndex);

    glm::mat4 GetWorldTransform(const aiNode* node);

    MeshHandle LoadMeshData(unsigned int meshIndex, aiMesh* mesh, const aiScene* scene, const std::filesystem::path& baseDir);

    void NormalizeSceneToUnitSize();

private:
    ResourceManager* m_ResourceManager;

};