#include "Application/VulkanApplication.h"
#include "Common/ApplicationConfig.h"
#include "Common/Constants.h"
#include <iostream>
#include <stdexcept>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main() {
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try {
        ApplicationConfig config;
        config.applicationName = Constants::DEFAULT_APP_NAME;
        config.scenePath = Constants::DEFAULT_SCENE_PATH;
        config.width = Constants::DEFAULT_WIDTH;
        config.height = Constants::DEFAULT_HEIGHT;

#ifdef NDEBUG
        config.enableValidationLayers = false;
#else
        config.enableValidationLayers = true;
#endif

        VulkanApplication app(config);
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}