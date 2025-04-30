#include "Scene.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(ResourceManager* resourceManager)
    : m_ResourceManager(resourceManager) {}

void Scene::LoadScene(const std::string& scenePath)
{
    // Load all models and textures for the scene
    std::filesystem::path baseDir = std::filesystem::path(scenePath).parent_path();

    std::vector<std::string> modelPaths = {/* list of model paths */ };


    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(scenePath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace);


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
    }

    LoadModel(scenePath, baseDir, scene);
}

MeshHandle Scene::LoadModel(const std::string& modelPath, const std::filesystem::path& baseDir,const aiScene* scene)
{
    std::vector<MeshHandle> meshHandles;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        MeshHandle meshHandle = LoadMeshData(i,mesh, scene, baseDir);

        m_ResourceManager->AddModel(meshHandle);
        if (i == 16)
        {
			std::cout << meshHandle.textureIndex << std::endl;
			std::cout << meshHandle.indexCount << std::endl;
        }
        meshHandles.push_back(meshHandle);
    }

    return meshHandles.empty() ? MeshHandle{} : meshHandles[0];
}

MeshHandle Scene::LoadMeshData(unsigned int meshIndex, aiMesh* mesh, const aiScene* scene, const std::filesystem::path& baseDir)
{
    MeshHandle meshHandle{};
    meshHandle.indexOffset = static_cast<uint32_t>(m_ResourceManager->GetIndices().size());
    const aiNode* meshNode = nullptr;


    // Convert Assimp matrix to glm matrix
    //glm::mat4 modelMatrix = GetNodeWorldTransform(meshNode);

    meshHandle.modelMatrix = glm::mat4(1.0f);
    const float* mat = glm::value_ptr(meshHandle.modelMatrix);
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    // Process faces and vertices
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        aiFace face = mesh->mFaces[f];
        for (unsigned int idx = 0; idx < face.mNumIndices; idx++) {
            unsigned int vertexIndex = face.mIndices[idx];

            Vertex vertex{};
            vertex.pos = {
                mesh->mVertices[vertexIndex].x,
                mesh->mVertices[vertexIndex].y,
                mesh->mVertices[vertexIndex].z
            };

            vertex.texCoord = mesh->mTextureCoords[0]
                ? glm::vec2(mesh->mTextureCoords[0][vertexIndex].x,
                    mesh->mTextureCoords[0][vertexIndex].y)
                : glm::vec2(0.0f, 0.0f);

            vertex.color = mesh->HasVertexColors(0)
                ? glm::vec3(mesh->mColors[0][vertexIndex].r,
                    mesh->mColors[0][vertexIndex].g,
                    mesh->mColors[0][vertexIndex].b)
                : glm::vec3(1.0f);

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_ResourceManager->GetVertices().size());
                m_ResourceManager->GetVertices().push_back(vertex);
            }

            // Push back the index for this vertex into the global indices vector
            m_ResourceManager->GetIndices().push_back(uniqueVertices[vertex]);
        }
    }

    meshHandle.indexCount = static_cast<uint32_t>(m_ResourceManager->GetIndices().size()) - meshHandle.indexOffset;

    // Load texture
    if (mesh->mMaterialIndex > 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texPath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::filesystem::path texturePath = baseDir / texPath.C_Str();
            texturePath = texturePath.lexically_normal();
            // Add texture if it doesn't exist already
            meshHandle.textureIndex = m_ResourceManager->AddTexture(texturePath.string());
        }
        else {
            meshHandle.textureIndex = 0;
			//std::cout << "No texture found for mesh: " << mesh->mName.C_Str() << std::endl;
        }
    }
    else {
        meshHandle.textureIndex = 0;
    }
	std::cout << "----------Mesh loaded: " << mesh->mName.C_Str()<<" --------------" << std::endl;
    return meshHandle;
}