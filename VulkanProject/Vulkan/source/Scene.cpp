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
        aiProcess_CalcTangentSpace
        /* | aiProcess_RemoveRedundantMaterials */ );


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
    }

    LoadObjModel(scenePath, baseDir, scene);
}

MeshHandle Scene::LoadObjModel(const std::string& modelPath, const std::filesystem::path& baseDir,const aiScene* scene)
{
    std::vector<MeshHandle> meshHandles;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        MeshHandle meshHandle = LoadMeshData(i,mesh, scene, baseDir);

        m_ResourceManager->AddModel(meshHandle);

        meshHandles.push_back(meshHandle);
    }

    return meshHandles.empty() ? MeshHandle{} : meshHandles[0];
}

void Scene::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        
    }
}

void Scene::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    //std::unordered_map<Vertex, uint32_t> uniqueVertices;
    //
    //for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    //    Vertex vertex;
    //
    //    vertex.pos = glm::vec3(
    //        mesh->mVertices[i].x,
    //        mesh->mVertices[i].y,
    //        mesh->mVertices[i].z
    //    );
    //    vertex.color = mesh->HasNormals() ? glm::vec3(
    //        mesh->mNormals[i].x,
    //        mesh->mNormals[i].y,
    //        mesh->mNormals[i].z
    //    ) : glm::vec3(0.f);
    //
    //    if (mesh->mTextureCoords[0]) {
    //        vertex.texCoord = glm::vec2(
    //            mesh->mTextureCoords[0][i].x,
    //            mesh->mTextureCoords[0][i].y
    //        );
    //    }
    //    else {
    //        vertex.texCoord = glm::vec2(0.0f);
    //    }
    //
    //    if (uniqueVertices.count(vertex) == 0) {
    //        uniqueVertices[vertex] = static_cast<uint32_t>(m_ResourceManager->GetVertices().size());
    //        m_ResourceManager->GetVertices().push_back(vertex);
    //    }
    //}
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