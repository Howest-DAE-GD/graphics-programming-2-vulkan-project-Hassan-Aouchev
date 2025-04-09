cmake_minimum_required(VERSION 3.17)

project("VulkanProject")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source to this project's executable.
add_executable(${PROJECT_NAME} "VulkanProject.cpp" "VulkanProject.h")

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

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} ${stb_image_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm)

# Shader compilation
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/CustomShaders")
file(GLOB SHADER_SOURCE_FILES "${SHADER_SOURCE_DIR}/*.vert" "${SHADER_SOURCE_DIR}/*.frag")

# Texture compilation
set(TEXTURE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/textures")
set(TEXTURE_BINARY_DIR "${CMAKE_BINARY_DIR}/textures")
file(GLOB TEXTURE_SOURCE_FILES "${TEXTURE_SOURCE_DIR}/*.png" "${TEXTURE_SOURCE_DIR}/*.jpg")

# Make sure output directory exists
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR} ${TEXTURE_BINARY_DIR})

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

# Add include directories for shaders
target_include_directories(${PROJECT_NAME} PUBLIC ${SHADER_SOURCE_DIR})