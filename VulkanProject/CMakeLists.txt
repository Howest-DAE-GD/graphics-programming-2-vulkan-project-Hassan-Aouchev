cmake_minimum_required(VERSION 3.17)

project("VulkanProject")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source to this project's executable.
add_executable(${PROJECT_NAME} "VulkanProject.cpp" "source/Device.cpp" "source/Instance.cpp" "source/PipelineManager.cpp" "source/SwapChain.cpp" "source/ResourceManager.cpp" "source/CommandManager.cpp" "source/Renderer.cpp" "source/Scene.cpp")

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
)
FetchContent_GetProperties(glfw)
if(NOT glfw_POPULATED)
    FetchContent_MakeAvailable(glfw)
endif()

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master
)
FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
    FetchContent_MakeAvailable(glm)
endif()

# FetchContent for stb_image
FetchContent_Declare(
    stb_image
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)

# Make sure it's available
FetchContent_MakeAvailable(stb_image)

set(EXTERNAL_DIR "${CMAKE_BINARY_DIR}/external")
file(MAKE_DIRECTORY ${EXTERNAL_DIR})

# Download tinyobjloader directly
file(DOWNLOAD 
    https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/master/tiny_obj_loader.h
    ${EXTERNAL_DIR}/tiny_obj_loader.h
    SHOW_PROGRESS
)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master  # You can specify a particular commit or tag if desired
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} ${stb_image_SOURCE_DIR} ${EXTERNAL_DIR} ${assimp_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm assimp)

# Shader compilation
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/CustomShaders")
file(GLOB SHADER_SOURCE_FILES "${SHADER_SOURCE_DIR}/*.vert" "${SHADER_SOURCE_DIR}/*.frag")

# Texture compilation
set(TEXTURE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/textures")
set(TEXTURE_BINARY_DIR "${CMAKE_BINARY_DIR}/textures")
file(GLOB TEXTURE_SOURCE_FILES "${TEXTURE_SOURCE_DIR}/*.png" "${TEXTURE_SOURCE_DIR}/*.jpg")

set(MODEL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/model")
set(MODEL_BINARY_DIR "${CMAKE_BINARY_DIR}/model")
file(GLOB MODEL_SOURCE_FILES "${MODEL_SOURCE_DIR}/*.obj")

# Make sure output directory exists
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR} ${TEXTURE_BINARY_DIR} ${MODEL_BINARY_DIR})

# Find glslc compiler (Vulkan SDK)
find_program(GLSLC_EXECUTABLE glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES bin)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Make sure Vulkan SDK is installed and added to your PATH.")
endif()

# List for compiled shader binaries
set(SHADER_BINARY_FILES "")
foreach(SHADER ${SHADER_SOURCE_FILES})
    get_filename_component(FILENAME ${SHADER} NAME)
    set(SPV ${SHADER_BINARY_DIR}/${FILENAME}.spv)
    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
    )
    list(APPEND SHADER_BINARY_FILES ${SPV})
endforeach()
# Ensure shaders are compiled before the main target
add_custom_target(Shaders ALL DEPENDS ${SHADER_BINARY_FILES})
add_dependencies(${PROJECT_NAME} Shaders)


set(TEXTURE_BINARY_FILES "")
foreach(TEXTURE ${TEXTURE_SOURCE_FILES})
    get_filename_component(FILENAME ${TEXTURE} NAME)
    set(DESTINATION_FILE ${TEXTURE_BINARY_DIR}/${FILENAME})
    add_custom_command(
        OUTPUT ${DESTINATION_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TEXTURE} ${DESTINATION_FILE}
        DEPENDS ${TEXTURE}
    )
    list(APPEND TEXTURE_BINARY_FILES ${DESTINATION_FILE})
endforeach()
add_custom_target(Textures ALL DEPENDS ${TEXTURE_BINARY_FILES})
add_dependencies(${PROJECT_NAME} Textures)

set(MODEL_BINARY_FILES "")
foreach(MODEL ${MODEL_SOURCE_FILES})
    get_filename_component(FILENAME ${MODEL} NAME)
    set(DESTINATION_FILE ${MODEL_BINARY_DIR}/${FILENAME})
    add_custom_command(
        OUTPUT ${DESTINATION_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MODEL} ${DESTINATION_FILE}
        DEPENDS ${MODEL}
    )
    list(APPEND MODEL_BINARY_FILES ${DESTINATION_FILE})
endforeach()
add_custom_target(Model ALL DEPENDS ${MODEL_BINARY_FILES})
add_dependencies(${PROJECT_NAME} Model)

# Define the directory for scene assets
set(SCENE_ASSETS_DIR "${CMAKE_BINARY_DIR}/scene")
file(MAKE_DIRECTORY ${SCENE_ASSETS_DIR})

# Download the assets zip file
file(DOWNLOAD 
    "https://github.com/Hassan-Aouchev/CrytekScene/archive/main.zip"
    "${SCENE_ASSETS_DIR}/CrytekScene.zip"
    SHOW_PROGRESS
    STATUS DOWNLOAD_STATUS
)

list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
if(NOT STATUS_CODE EQUAL 0)
    message(FATAL_ERROR "Failed to download scene assets.")
endif()

# Create a temporary directory for extraction
set(TEMP_EXTRACT_DIR "${CMAKE_BINARY_DIR}/temp_extract")
file(MAKE_DIRECTORY ${TEMP_EXTRACT_DIR})

# Extract the archive to the temporary directory
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf "${SCENE_ASSETS_DIR}/CrytekScene.zip"
    WORKING_DIRECTORY ${TEMP_EXTRACT_DIR}
    RESULT_VARIABLE EXTRACT_RESULT
)

if(NOT EXTRACT_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to extract scene assets.")
endif()

# Copy contents from the CrytekScene-main folder to the scene directory
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${TEMP_EXTRACT_DIR}/CrytekScene-main/" "${SCENE_ASSETS_DIR}/"
    RESULT_VARIABLE COPY_RESULT
)

if(NOT COPY_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to copy scene assets to final location.")
endif()

# Clean up temporary files
execute_process(
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${TEMP_EXTRACT_DIR}"
)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E remove "${SCENE_ASSETS_DIR}/CrytekScene.zip"
)

# Add the scene directory to include paths
include_directories(${SCENE_ASSETS_DIR}/textures)