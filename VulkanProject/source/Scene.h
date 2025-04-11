#include <string>
#include <vector>
#include <filesystem>
#include "ResourceManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


class PipelineManager;
class Scene {
public:
    Scene(ResourceManager* resourceManager, PipelineManager* pipelineManager);

    void LoadScene(const std::string& scenePath);

    MeshHandle LoadModel(const std::string& modelPath, const std::filesystem::path& baseDir,const aiScene* scene);

    MeshHandle LoadMeshData(unsigned int meshIndex, aiMesh* mesh, const aiScene* scene, const std::filesystem::path& baseDir);

    const aiNode* FindNodeForMesh(const aiNode* node, unsigned int meshIndex) const;

    void BuildMeshToNodeMap(const aiNode* node);

    glm::mat4 GetNodeWorldTransform(const aiNode* node) const;


private:
    ResourceManager* m_ResourceManager;
    PipelineManager* m_PipelineManager;
    std::unordered_map<unsigned int, const aiNode*> m_MeshToNodeMap;

};