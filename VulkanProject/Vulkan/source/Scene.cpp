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

    m_ResourceManager->AddTexture("textures/error.png", VK_FORMAT_R8G8B8A8_SRGB);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(scenePath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace

        /* | aiProcess_RemoveRedundantMaterials */ );


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
    }
    std::cout << scenePath << std::endl;
    LoadObjModel(scenePath, baseDir, scene);
}

MeshHandle Scene::LoadObjModel(const std::string& modelPath, const std::filesystem::path& baseDir,const aiScene* scene)
{
    std::vector<MeshHandle> meshHandles;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        MeshHandle meshHandle = LoadMeshData(i,mesh, scene, baseDir);

        m_ResourceManager->AddModel(meshHandle,i);

        meshHandles.push_back(meshHandle);
    }

    return meshHandles.empty() ? MeshHandle{} : meshHandles[0];
}

const aiNode* Scene::FindMeshNode(const aiNode* node, unsigned int meshIndex)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        if (node->mMeshes[i] == meshIndex) return node;
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        const aiNode* found = FindMeshNode(node->mChildren[i], meshIndex);
        if (found) return found;
    }
    return nullptr;
}

glm::mat4 Scene::GetWorldTransform(const aiNode* node)
{
    glm::mat4 transform(1.f);
    while (node)
    {
        aiMatrix4x4 m = node->mTransformation;
        glm::mat4 mat = glm::transpose(glm::make_mat4(&m.a1));
        transform = mat * transform;
        node = node->mParent;
    }
    return transform;
}

MeshHandle Scene::LoadMeshData(unsigned int meshIndex, aiMesh* mesh, const aiScene* scene, const std::filesystem::path& baseDir)
{
    MeshHandle meshHandle{};
    meshHandle.indexOffset = static_cast<uint32_t>(m_ResourceManager->GetIndices().size());
    const aiNode* meshNode = FindMeshNode(scene->mRootNode,meshIndex);
    if(meshNode)
    meshHandle.modelMatrix = GetWorldTransform(meshNode);

    meshHandle.modelMatrix = glm::mat4(1.0f);
    const float* mat = glm::value_ptr(meshHandle.modelMatrix);
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    // Process faces and vertices
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        aiFace face = mesh->mFaces[f];
        for (unsigned int idx = 0; idx < face.mNumIndices; idx++) {
            unsigned int vertexIndex = face.mIndices[idx];

            Vertex vertex{};
            aiVector3D position = scene->mRootNode->mTransformation * mesh->mVertices[vertexIndex];

            vertex.pos = { position.x,
                           position.y,
                           position.z };

            vertex.texCoord = mesh->mTextureCoords[0]
                ? glm::vec2(mesh->mTextureCoords[0][vertexIndex].x,
                    mesh->mTextureCoords[0][vertexIndex].y)
                : glm::vec2(0.0f, 0.0f);

            vertex.normal = mesh->HasNormals()
                ? glm::vec3(mesh->mNormals[vertexIndex].x,
                    mesh->mNormals[vertexIndex].y,
                    mesh->mNormals[vertexIndex].z)
                : glm::vec3(1.0f);

            vertex.tangent = mesh->HasTangentsAndBitangents()
                ? glm::vec3(mesh->mTangents[vertexIndex].x,
                    mesh->mTangents[vertexIndex].y,
                    mesh->mTangents[vertexIndex].z)
                : glm::vec3(0.0f);

            vertex.bitTangent = mesh->HasTangentsAndBitangents()
                ? glm::vec3(mesh->mBitangents[vertexIndex].x,
                    mesh->mBitangents[vertexIndex].y,
                    mesh->mBitangents[vertexIndex].z)
                : glm::vec3(0.0f);

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
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texPath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::filesystem::path texturePath = baseDir / texPath.C_Str();
            texturePath = texturePath.lexically_normal();
            meshHandle.material.baseColorTextureIndex = m_ResourceManager->AddTexture(texturePath.string(),VK_FORMAT_R8G8B8A8_SRGB);

            meshHandle.material.hasAlphaMask = 0;
            meshHandle.material.alphaCutoff = 0.5f;

            aiString alphaModeStr;
            if (material->Get("$mat.gltf.alphaMode",0,0, alphaModeStr)==AI_SUCCESS);// dont pass all textures to the depth buffer cause thats not necessary
            {
                std::string alphaMode = alphaModeStr.C_Str();

                if (alphaMode == "MASK") {
                    meshHandle.material.hasAlphaMask = 1;

                    float alphaCutoff = 0.5f;
                    if (material->Get("$mat.gltf.alphaCutoff", 0, 0, alphaCutoff) == AI_SUCCESS) {
                        meshHandle.material.alphaCutoff = alphaCutoff;
                    }
                }
            }

            if (meshHandle.material.hasAlphaMask) {
                meshHandle.material.alphaTextureIndex = m_ResourceManager->AddAlphaTexture(texturePath.string(), VK_FORMAT_R8G8B8A8_SRGB);
            }
        }
        else {
            meshHandle.material.baseColorTextureIndex = 0;
        }
        if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
            std::filesystem::path texturePath = baseDir / texPath.C_Str();
            texturePath = texturePath.lexically_normal();
            meshHandle.material.normalTextureIndex = m_ResourceManager->AddTexture(texturePath.string(),VK_FORMAT_R8G8B8A8_UNORM);
        }
        else {
            meshHandle.material.normalTextureIndex = 0;
        }
        if (material->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS) {
            std::filesystem::path texturePath = baseDir / texPath.C_Str();
            texturePath = texturePath.lexically_normal();
            meshHandle.material.metallicRoughnessTextureIndex = m_ResourceManager->AddTexture(texturePath.string(), VK_FORMAT_R8G8B8A8_SRGB);
        }
        else {
            meshHandle.material.metallicRoughnessTextureIndex = 0;
        }
    }
	std::cout << "----------Mesh loaded: " << mesh->mName.C_Str()<<" --------------" << std::endl;
    return meshHandle;
}
