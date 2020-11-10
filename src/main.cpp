#include "camera.hpp"
#include "image.hpp"
#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "render.hpp"
#include "scene.hpp"
#include "span.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


static std::vector<MeshTri> quadMeshTris(unsigned quadCount) {
    std::vector<MeshTri> tris;
    for (unsigned i = 0; i < quadCount; ++i) {
        tris.push_back({4 * i, 4 * i + 2, 4 * i + 1});
        tris.push_back({4 * i + 1, 4 * i + 2, 4 * i + 3});
    }
    return tris;
}


std::pair<std::vector<Vertex>, std::vector<MeshTri>> cube() {
    return {
        {
            // Front
            {{-0.5f,  0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},       // Top left
            {{ 0.5f,  0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},       // Top right
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},       // Bottom left
            {{ 0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},       // Bottom right
            // Rear
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},     // Top right
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},     // Top left
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},     // Bottom right
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},     // Bottom left
            // Top
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},       // Rear left
            {{ 0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},       // Rear right
            {{-0.5f, 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},       // Front left
            {{ 0.5f, 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},       // Front right
            // Bottom
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},     // Front left
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},     // Front right
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},     // Rear left
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},     // Rear right
            // Left
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},     // Rear top
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},     // Front top
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},     // Rear bottom
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},     // Front bottom
            // Right
            {{0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},       // Front top
            {{0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},       // Rear top
            {{0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},       // Front bottom
            {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},       // Rear bottom
        },
        quadMeshTris(6)
    };
}

std::pair<std::vector<Vertex>, std::vector<MeshTri>> rect() {
    return {
        {
            {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},       // Top left
            {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},       // Top right
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},       // Bottom left
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},       // Bottom right
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
            {0.0f, 0.0f, 0.0f},
            glm::angleAxis(glm::pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f}),
            glm::radians(45.0f)
        },
        {   // Lights
            {   // Point
                {{3.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.05f},
                {{-3.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.05f},
                {{0.0f, -2.0f, 5.0f}, {0.0f, 0.0f, 1.0f}, 0.01f}
            },
            {}, // Directional
            {}  // Spot
        },
        {   // Meshes
            cube(), rect()
        },
        {   // Materials
            {{0.5f, 0.0f, 1.0f}, 1.0f, 0.25f, 0.2f, 8.0f},
            {{0.0f, 0.5f, 1.0f}, 1.0f, 0.25f, 0.2f, 8.0f},
            {{0.5f, 1.0f, 0.0f}, 1.0f, 0.25f, 0.2f, 8.0f},
            {{1.0f, 1.0f, 1.0f}, 0.0f, 1.0f, 1.0f, 128.0f}
        },
        {}, // Models
        {}  // Instantiated meshes
    };

    {
        std::default_random_engine randEng{13};
        std::uniform_real_distribution<float> dist1{-1.0f, 1.0f};
        std::uniform_real_distribution<float> dist2{0.25f, 1.0f};
        for (unsigned i = 0; i < 300; ++i) {
            auto const mesh = randEng() % scene.meshes.meshes.size();
            auto const material = randEng() % scene.materials.size();
            glm::vec3 const position{dist1(randEng) * 10.0f, dist1(randEng) * 3.0f, dist1(randEng) * 10.0f};
            glm::quat const orientation{glm::vec3{dist1(randEng), 0.0f, dist1(randEng)}};
            glm::vec3 const scale{dist2(randEng), dist2(randEng), dist2(randEng)};
            scene.models.meshTransforms.push_back({position, orientation, scale});
            scene.models.meshes.push_back(mesh);
            scene.models.materials.push_back(material);
        }
    }

    instantiateMeshes(Span{scene.meshes.vertices}, Span{scene.meshes.meshes}, Span{scene.models.meshTransforms},
        Span{scene.models.meshes}, scene.instantiatedMeshes);

    auto const pixelToRayTransform = ::pixelToRayTransform(scene.camera.forward(), scene.camera.up(),
        scene.camera.fov, IMAGE_WIDTH, IMAGE_HEIGHT);

    RenderData const renderData{
        IMAGE_WIDTH, IMAGE_HEIGHT,
        scene.camera.position, pixelToRayTransform,
        {
            {Span{scene.instantiatedMeshes.vertices}, Span{scene.meshes.tris}, Span{scene.instantiatedMeshes.meshes}},
            Span{scene.models.materials}
        },
        {Span{scene.lights.point}, Span{scene.lights.directional}, Span{scene.lights.spot}},
        Span{scene.materials}
    };
    auto const t1 = std::chrono::high_resolution_clock::now();
    render(renderData, Span{renderBuffer});
    auto const t2 = std::chrono::high_resolution_clock::now();

    auto const time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    auto const timePerPixel = time.count() / static_cast<double>(IMAGE_WIDTH * IMAGE_HEIGHT);
    std::cout << "Render done in " << time.count() << "us (" << timePerPixel << " us per pixel)" << std::endl;

    linearTo8BitSRGB(Span{renderBuffer}, Span{imageBuffer});

    std::ofstream output{"output.ppm", std::ofstream::binary | std::ofstream::out};
    output << "P6\n";
    output << IMAGE_WIDTH << ' ' << IMAGE_HEIGHT << '\n';
    output << "255\n";
    static_assert(sizeof(Pixel) == 3 && alignof(Pixel) == 1);
    output.write(reinterpret_cast<char const*>(imageBuffer.data()), imageBuffer.size() * sizeof(Pixel));
}
