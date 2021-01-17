#include "basic_types.hpp"
#include "bsp.hpp"
#include "geometry.hpp"
#include "image.hpp"
#include "mesh.hpp"
#include "render.hpp"
#include "scene.hpp"
#include "utility/numeric.hpp"
#include "utility/permuted_span.hpp"
#include "utility/span.hpp"
#include "utility/time.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>


static std::vector<MeshTri> quadMeshTris(unsigned quadCount) {
    std::vector<MeshTri> tris;
    for (unsigned i = 0; i < quadCount; ++i) {
        auto const fourI = 4 * i;
        auto const v1 = intCast<VertexIndex>(fourI);
        auto const v2 = intCast<VertexIndex>(fourI + 2);
        auto const v3 = intCast<VertexIndex>(fourI + 1);
        auto const v4 = intCast<VertexIndex>(fourI + 3);
        tris.push_back({v1, v2, v3});
        tris.push_back({v3, v2, v4});
    }
    return tris;
}


std::tuple<std::vector<PackedFVec3>, std::vector<PackedFVec3>, std::vector<MeshTri>> plane() {
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


std::tuple<std::vector<PackedFVec3>, std::vector<PackedFVec3>, std::vector<MeshTri>> cube() {
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
    constexpr unsigned IMAGE_WIDTH = 1920;
    constexpr unsigned IMAGE_HEIGHT = 1080;

    std::vector<PackedFVec3> renderBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};
    std::vector<PackedFVec3> filteredBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};
    std::vector<PackedU8Vec3> imageBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};

    Scene scene{
        {   // Camera
            {9.0f, 8.0f, 16.0f}, PackedFVec3{0.3f, -2.6f, 0.0f}, glm::radians(45.0f)
        },
        {plane(), cube()},   // Meshes
        {   // Materials
            {{0.25f, 0.25f, 0.25f}, 0.9f, 0.0f, {0.0f, 0.0f, 0.0f}},    // Floor
            {{1.0f, 1.0f, 1.0f}, 0.04f, 1.0f, {0.0f, 0.0f, 0.0f}}       // Mirror
        },
        {   // Models
            {   // Mesh instance transforms
                {{2.0f, 0.0f, 2.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {16.0f, 1.0f, 16.0f}},                   // Floor
                {{0.0f, 5.0f, -6.0f}, PackedFVec3{glm::half_pi<float>(), 0.0f, 0.0f}, {20.0f, 1.0f, 10.0f}}, // Mirror 1
                {{-6.0f, 5.0f, 0.0f}, PackedFVec3{0.0f, 0.0f, -glm::half_pi<float>()}, {10.0f, 1.0f, 20.0f}} // Mirror 2
            },
            {   // Model mesh indices
                0, 0, 0
            },
            {   // Model material indices
                0, 1, 1
            }
        },
        {}, // Instantiated meshes
        {}  // Preprocessed tris
    };

    // Generate RGB cube models.
    {
        constexpr PackedFVec3 POSITION{0.0f, 2.5f, 0.0f};
        constexpr float SCALE = 4.0f;
        constexpr unsigned DIVISOR = 3;
        constexpr float SUBSCALE = 0.75f;
        for (unsigned x = 0; x < DIVISOR; ++x) {
            auto const xFract = x / static_cast<float>(DIVISOR - 1);
            for (unsigned y = 0; y < DIVISOR; ++y) {
                auto const yFract = y / static_cast<float>(DIVISOR - 1);
                for (unsigned z = 0; z < DIVISOR; ++z) {
                    auto const zFract = z / static_cast<float>(DIVISOR - 1);
                    auto const colour = srgbToLinear(PackedFVec3{xFract, yFract, zFract});
                    scene.materials.push_back({colour, 0.5f, 0.5f, colour});
                    auto const position = PackedFVec3{
                        (xFract - 0.5f) * (SCALE - SCALE / DIVISOR),
                        (yFract - 0.5f) * (SCALE - SCALE / DIVISOR),
                        (zFract - 0.5f) * (SCALE - SCALE / DIVISOR)
                    } + POSITION;
                    constexpr glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
                    constexpr PackedFVec3 scale{SUBSCALE * SCALE / DIVISOR};
                    scene.models.meshTransforms.push_back({position, orientation, scale});
                    scene.models.meshes.push_back(1);
                    scene.models.materials.push_back(intCast<MaterialIndex>(scene.materials.size() - 1));
                }
            }
        }
    }

    auto const preprocessBeginTime = std::chrono::high_resolution_clock::now();

    scene.preprocessedMaterials.resize(scene.materials.size());
    std::transform(scene.materials.cbegin(), scene.materials.cend(), scene.preprocessedMaterials.begin(), preprocessMaterial);

    auto const pixelToRayTransform = ::pixelToRayTransform(scene.camera.forward(), scene.camera.down(),
        scene.camera.right(), scene.camera.fov, IMAGE_WIDTH, IMAGE_HEIGHT);

    instantiateMeshes(readOnlySpan(scene.meshes.vertexPositions), readOnlySpan(scene.meshes.vertexNormals),
        readOnlySpan(scene.meshes.vertexRanges), readOnlySpan(scene.models.meshTransforms),
        readOnlySpan(scene.models.meshes), scene.instantiatedMeshes);

    preprocessTris(readOnlySpan(scene.instantiatedMeshes.vertexPositions),
        readOnlySpan(scene.instantiatedMeshes.vertexRanges), readOnlySpan(scene.meshes.tris),
        PermutedSpan{readOnlySpan(scene.meshes.triRanges), readOnlySpan(scene.models.meshes)},
        scene.preprocessedTris, scene.preprocessedTriRanges);

    auto meshBoundingBox = computeBoundingBox(readOnlySpan(scene.instantiatedMeshes.vertexPositions));
    // Expand bounding box slightly to account for FP error when handling surfaces right on edge of box.
    meshBoundingBox.min *= 1.001f;
    meshBoundingBox.max *= 1.001f;

    BSPTree const bspTree{
        readOnlySpan(scene.instantiatedMeshes.vertexPositions), readOnlySpan(scene.instantiatedMeshes.vertexRanges),
        readOnlySpan(scene.meshes.tris),
        PermutedSpan{readOnlySpan(scene.meshes.triRanges), readOnlySpan(scene.models.meshes)},
        readOnlySpan(scene.preprocessedTris), readOnlySpan(scene.preprocessedTriRanges),
        meshBoundingBox
    };

    auto const renderBeginTime = std::chrono::high_resolution_clock::now();
    RenderData const renderData{
        IMAGE_WIDTH, IMAGE_HEIGHT,
        scene.camera.position, pixelToRayTransform,
        {
            bspTree,
            readOnlySpan(scene.instantiatedMeshes.vertexNormals), readOnlySpan(scene.instantiatedMeshes.vertexRanges),
            readOnlySpan(scene.meshes.tris),
            PermutedSpan{readOnlySpan(scene.meshes.triRanges), readOnlySpan(scene.models.meshes)},
            PermutedSpan{readOnlySpan(scene.preprocessedMaterials), readOnlySpan(scene.models.materials)}
        }
    };
    render(renderData, Span{renderBuffer});

    auto const postprocessBeginTime = std::chrono::high_resolution_clock::now();
    std::transform(renderBuffer.cbegin(), renderBuffer.cend(), renderBuffer.begin(), reinhardToneMap);
    std::transform(renderBuffer.cbegin(), renderBuffer.cend(), renderBuffer.begin(),
        static_cast<PackedFVec3(*)(PackedFVec3 const&)>(linearToSRGB));
    std::transform(renderBuffer.cbegin(), renderBuffer.cend(), renderBuffer.begin(), nanToRed);
    std::transform(renderBuffer.cbegin(), renderBuffer.cend(), renderBuffer.begin(), infToGreen);
    std::copy(renderBuffer.cbegin(), renderBuffer.cend(), filteredBuffer.begin());
    medianFilter<1>(readOnlySpan(renderBuffer), IMAGE_WIDTH, Span{filteredBuffer});
    std::transform(filteredBuffer.cbegin(), filteredBuffer.cend(), imageBuffer.begin(), floatTo8BitUInt);

    auto const endTime = std::chrono::high_resolution_clock::now();

    {
        auto const time = std::chrono::duration_cast<FPSeconds>(renderBeginTime - preprocessBeginTime);
        auto const timePerModel = time / scene.models.meshes.size();
        std::cout << "Preprocess done in " << formatDuration(time)
            << " (" << formatDuration(timePerModel) << " per model)" << '\n';
    }

    {
        auto const time = std::chrono::duration_cast<FPSeconds>(postprocessBeginTime - renderBeginTime);
        auto const timePerPixel = time / (IMAGE_WIDTH * IMAGE_HEIGHT);
        auto const timePerSample = timePerPixel / PIXEL_SAMPLE_RATE;
        std::cout << "Render done in " << formatDuration(time)
            << " (" << formatDuration(timePerPixel) << " per pixel, "
            << formatDuration(timePerSample) << " per sample)" << '\n';
    }

    {
        auto const time = std::chrono::duration_cast<FPSeconds>(endTime - postprocessBeginTime);
        auto const timePerPixel = time / (IMAGE_WIDTH * IMAGE_HEIGHT);
        std::cout << "Postprocess done in " << formatDuration(time)
            << " (" << formatDuration(timePerPixel) << " per pixel)" << '\n';
    }

    {
        auto const time = std::chrono::duration_cast<FPSeconds>(endTime - preprocessBeginTime);
        std::cout << "Pipeline done in " << formatDuration(time) << '\n';
    }

    {
        std::ofstream output{"output.ppm", std::ofstream::binary | std::ofstream::out};
        output << "P6\n";
        output << IMAGE_WIDTH << ' ' << IMAGE_HEIGHT << '\n';
        output << "255\n";
        using PixelType = decltype(imageBuffer)::value_type;
        static_assert(sizeof(PixelType) == 3 && alignof(PixelType) == 1);
        output.write(reinterpret_cast<char const*>(imageBuffer.data()), imageBuffer.size() * sizeof(PixelType));
    }
}
