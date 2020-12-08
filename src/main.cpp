#include "image.hpp"
#include "mesh.hpp"
#include "render.hpp"
#include "scene.hpp"
#include "utility/permuted_span.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
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


std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<MeshTri>> plane() {
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


int main() {
    constexpr unsigned IMAGE_WIDTH = 1000;
    constexpr unsigned IMAGE_HEIGHT = 650;

    std::vector<glm::vec3> renderBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};
    std::vector<Pixel> srgbBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};
    std::vector<Pixel> imageBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};

    Scene scene{
        {   // Camera
            {6.0f, 2.0f, 6.5f},
            {glm::vec3{0.18f, -2.456, 0.0f}},
            glm::radians(45.0f)
        },
        {   // Lights
            {   // Point
                {{-6.0f, 2.0f, 6.5f}, {0.25f, 0.25f, 0.25f}}
            },
            {   // Directional
                {glm::normalize(glm::vec3{0.5f, -4.0f, -1.0f}), {0.02f, 0.02f, 0.02f}}
            }
        },
        {plane(), cube()},   // Meshes
        {   // Materials
            {{0.25f, 0.25f, 0.25f}, 0.9f, 0.0f, {0.0f, 0.0f, 0.0f}},    // Grey
            {{1.0f, 0.0f, 1.0f}, 0.5f, 0.0f, {0.5f, 0.0f, 0.5f}},       // Yellow
            {{0.0f, 1.0f, 1.0f}, 0.5f, 0.0f, {0.0f, 0.5f, 0.5f}},       // Cyan
            {{1.0f, 1.0f, 0.0f}, 0.5f, 0.0f, {0.5f, 0.5f, 0.0f}},       // Magenta
            {{1.0f, 1.0f, 1.0f}, 0.005f, 1.0f, {0.0f, 0.0f, 0.0f}}      // Mirror
        },
        {   // Models
            {   // Mesh instance transforms
                {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {100.0f, 1.0f, 100.0f}},
                {{-3.0f, 0.5f, 0.0f}},
                {{0.0f, 0.5f, 0.0f}},
                {{3.0f, 0.5f, 0.0f}},
                {{0.0f, 2.5f, -5.0f}, {glm::vec3{glm::half_pi<float>(), 0.0f, 0.0f}}, {10.0f, 1.0f, 5.0f}},
            },
            {   // Model mesh indices
                0, 1, 1, 1, 0
            },
            {   // Model material indices
                0, 1, 2, 3, 4
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

    auto const pixelToRayTransform = ::pixelToRayTransform(scene.camera.forward(), scene.camera.down(),
        scene.camera.right(), scene.camera.fov, IMAGE_WIDTH, IMAGE_HEIGHT);

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
            readOnlySpan(scene.lights.point), readOnlySpan(scene.lights.directional)
        }
    };

    auto const renderBeginTime = std::chrono::high_resolution_clock::now();
    render(renderData, Span{renderBuffer});

    auto const postprocessBeginTime = std::chrono::high_resolution_clock::now();
    std::transform(renderBuffer.cbegin(), renderBuffer.cend(), srgbBuffer.begin(),
        static_cast<Pixel(*)(glm::vec3)>(linearTo8BitSRGB));
    medianFilter<1>(Span{srgbBuffer}, IMAGE_WIDTH, Span{imageBuffer});

    auto const endTime = std::chrono::high_resolution_clock::now();

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(renderBeginTime - preprocessBeginTime);
        auto const timePerModel = time.count() / static_cast<double>(scene.models.meshes.size());
        std::cout << "Preprocess done in " << time.count() << "us (" << timePerModel << "us per model)" << std::endl;
    }

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(postprocessBeginTime - renderBeginTime);
        auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
        auto const timePerSample = timePerPixel / PIXEL_SAMPLE_RATE;
        std::cout << "Render done in " << time.count() << "us (" << timePerPixel << "us per pixel, "
            << timePerSample << "us per sample)" << std::endl;
    }

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(endTime - postprocessBeginTime);
        auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
        std::cout << "Postprocess done in " << time.count() << "us (" << timePerPixel << "us per pixel)" << std::endl;
    }

    {
        auto const time = std::chrono::duration_cast<std::chrono::microseconds>(endTime - preprocessBeginTime);
        std::cout << "Pipeline done in " << time.count() << "us" << std::endl;
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
