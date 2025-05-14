#pragma once

#include <string>

struct ApplicationConfig {
    int width = 800;
    int height = 600;
    std::string scenePath = "scene/gltf/Sponza.gltf";
    std::string applicationName = "Vulkan Application";
    bool enableValidationLayers = true;
};