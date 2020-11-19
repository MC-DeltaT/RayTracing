#include "image.hpp"
#include "mesh.hpp"
#include "render.hpp"
#include "scene.hpp"
#include "utility.hpp"

#include <algorithm>
#include <chrono>
#include <execution>
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>


static std::vector<MeshTri> quadMeshTris(unsigned quadCount) {
    std::vector<MeshTri> tris;
    for (unsigned i = 0; i < quadCount; ++i) {
        tris.push_back({4 * i, 4 * i + 2, 4 * i + 1});
        tris.push_back({4 * i + 1, 4 * i + 2, 4 * i + 3});
    }
    return tris;
}


std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<MeshTri>> cube() {
    return {
        {
            // Front
            {-0.5f,  0.5f, 0.5f},       // Top left
            { 0.5f,  0.5f, 0.5f},       // Top right
            {-0.5f, -0.5f, 0.5f},       // Bottom left
            { 0.5f, -0.5f, 0.5f},       // Bottom right
            // Rear
            { 0.5f,  0.5f, -0.5f},      // Top right
            {-0.5f,  0.5f, -0.5f},      // Top left
            { 0.5f, -0.5f, -0.5f},      // Bottom right
            {-0.5f, -0.5f, -0.5f},      // Bottom left
            // Top
            {-0.5f, 0.5f, -0.5f},       // Rear left
            { 0.5f, 0.5f, -0.5f},       // Rear right
            {-0.5f, 0.5f,  0.5f},       // Front left
            { 0.5f, 0.5f,  0.5f},       // Front right
            // Bottom
            {-0.5f, -0.5f,  0.5f},      // Front left
            { 0.5f, -0.5f,  0.5f},      // Front right
            {-0.5f, -0.5f, -0.5f},      // Rear left
            { 0.5f, -0.5f, -0.5f},      // Rear right
            // Left
            {-0.5f,  0.5f, -0.5f},      // Rear top
            {-0.5f,  0.5f,  0.5f},      // Front top
            {-0.5f, -0.5f, -0.5f},      // Rear bottom
            {-0.5f, -0.5f,  0.5f},      // Front bottom
            // Right
            {0.5f,  0.5f,  0.5f},       // Front top
            {0.5f,  0.5f, -0.5f},       // Rear top
            {0.5f, -0.5f,  0.5f},       // Front bottom
            {0.5f, -0.5f, -0.5f}        // Rear bottom
        },
        {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f}
        },
        quadMeshTris(6)
    };
}

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<MeshTri>> rect() {
    return {
        {
            {-0.5f, 0.0f, -0.5f},   // Rear left
            { 0.5f, 0.0f, -0.5f},   // Rear right
            {-0.5f, 0.0f,  0.5f},   // Front left
            { 0.5f, 0.0f,  0.5f},   // Front right
        },
        {
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}
        },
        quadMeshTris(1)
    };
}


int main() {
    constexpr unsigned IMAGE_WIDTH = 1920;
    constexpr unsigned IMAGE_HEIGHT = 1080;

    std::vector<glm::vec3> renderBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};
    std::vector<Pixel> imageBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};

    Scene scene{
        {   // Camera
            {0.0f, 0.0f, 10.0f},
            glm::angleAxis(glm::pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f}),
            glm::radians(45.0f)
        },
        {   // Lights
            {   // Point
                {{-10.0f, 10.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.01f},
                {{10.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.01f},
                {{0.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 1.0f}, 0.01f}
            },
            {}, // Directional
            {}  // Spot
        },
        {cube(), rect()},   // Meshes
        {   // Materials
            {{0.5f, 0.0f, 1.0f}, 1.0f, 0.25f, 0.2f, 8.0f},
            {{0.0f, 0.5f, 1.0f}, 1.0f, 0.25f, 0.2f, 8.0f},
            {{0.5f, 1.0f, 0.0f}, 1.0f, 0.25f, 0.2f, 8.0f},
            {{1.0f, 1.0f, 1.0f}, 0.0f, 1.0f, 1.0f, 128.0f}
        },
        {   // Models
            {   // Mesh instance transforms
                {{-3.0f, 0.0f, 0.0f}},
                {{0.0f, 0.0f, 0.0f}, glm::angleAxis(glm::quarter_pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f})},
                {{3.0f, 0.0f, 0.0f}}
            },
            {   // Model mesh indices
                0, 0, 0
            },
            {   // Model material indices
                2, 1, 0
            }
        },
        {}, // Instantiated meshes
        {}  // Preprocessed tris
    };

    auto const preprocessBeginTime = std::chrono::high_resolution_clock::now();

    instantiateMeshes(readOnlySpan(scene.meshes.vertexPositions), readOnlySpan(scene.meshes.vertexNormals),
        readOnlySpan(scene.meshes.vertexRanges), readOnlySpan(scene.models.meshTransforms),
        readOnlySpan(scene.models.meshes), scene.instantiatedMeshes);

    preprocessTris(readOnlySpan(scene.instantiatedMeshes.vertexPositions),
        readOnlySpan(scene.instantiatedMeshes.vertexRanges), readOnlySpan(scene.meshes.tris),
        PermutedSpan{readOnlySpan(scene.meshes.triRanges), readOnlySpan(scene.models.meshes)},
        scene.preprocessedTris, scene.preprocessedTriRanges);

    auto const pixelToRayTransform = ::pixelToRayTransform(scene.camera.forward(), scene.camera.up(), scene.camera.fov,
        IMAGE_WIDTH, IMAGE_HEIGHT);

    RenderData const renderData{
        IMAGE_WIDTH, IMAGE_HEIGHT,
        scene.camera.position, pixelToRayTransform,
        {
            readOnlySpan(scene.instantiatedMeshes.vertexNormals),
            readOnlySpan(scene.meshes.tris), readOnlySpan(scene.preprocessedTris),
            readOnlySpan(scene.instantiatedMeshes.vertexRanges),
            PermutedSpan{readOnlySpan(scene.meshes.triRanges), readOnlySpan(scene.models.meshes)},
            readOnlySpan(scene.preprocessedTriRanges),
            PermutedSpan{readOnlySpan(scene.materials), readOnlySpan(scene.models.materials)},
            readOnlySpan(scene.lights.point), readOnlySpan(scene.lights.directional), readOnlySpan(scene.lights.spot)
        }
    };

    auto const renderBeginTime = std::chrono::high_resolution_clock::now();
    render(renderData, Span{renderBuffer});

    auto const postprocessBeginTime = std::chrono::high_resolution_clock::now();
    std::transform(std::execution::par_unseq, renderBuffer.cbegin(), renderBuffer.cend(), imageBuffer.begin(),
        [](auto& value) {
            return linearTo8BitSRGB(value);
        }
    );

    auto const endTime = std::chrono::high_resolution_clock::now();

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(renderBeginTime - preprocessBeginTime);
        auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
        std::cout << "Preprocess done in " << time.count() << "us" << std::endl;
    }

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(postprocessBeginTime - renderBeginTime);
        auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
        std::cout << "Render done in " << time.count() << "us (" << timePerPixel << " us per pixel)" << std::endl;
    }

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(endTime - postprocessBeginTime);
        auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
        std::cout << "Postprocess done in " << time.count() << "us (" << timePerPixel << " us per pixel)" << std::endl;
    }

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(endTime - preprocessBeginTime);
        auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
        std::cout << "Pipeline done in " << time.count() << "us (" << timePerPixel << " us per pixel)" << std::endl;
    }

    {
        std::ofstream output{"output.ppm", std::ofstream::binary | std::ofstream::out};
        output << "P6\n";
        output << IMAGE_WIDTH << ' ' << IMAGE_HEIGHT << '\n';
        output << "255\n";
        static_assert(sizeof(Pixel) == 3 && alignof(Pixel) == 1);
        output.write(reinterpret_cast<char const*>(imageBuffer.data()), imageBuffer.size() * sizeof(Pixel));
    }
}
