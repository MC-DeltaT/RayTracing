#include "camera.hpp"
#include "image.hpp"
#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "render.hpp"
#include "span.hpp"

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
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


Mesh cube() {
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

Mesh rect() {
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


struct Scene {
    Camera camera;
    std::vector<PointLight> pointLights;
    std::vector<Material> materials;
    std::vector<std::vector<Vertex>> meshVertices;
    std::vector<std::vector<MeshTri>> meshTris;
    std::vector<MeshInstance> meshInstances;
};


int main() {
    constexpr unsigned IMAGE_WIDTH = 1920;
    constexpr unsigned IMAGE_HEIGHT = 1080;
    constexpr unsigned RAY_TRACE_DEPTH = 10;

    std::vector<glm::vec3> renderBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};
    std::vector<Pixel> imageBuffer{IMAGE_HEIGHT * IMAGE_WIDTH};

    auto cube = ::cube();
    auto rect = ::rect();
    Scene const scene{
        {
            {0.0f, 0.0f, 10.0f},
            glm::angleAxis(glm::pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f}),
            glm::radians(45.0f)
        },
        {
            {{3.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.05f},
            {{-3.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.05f},
            {{0.0f, -2.0f, 5.0f}, {0.0f, 0.0f, 1.0f}, 0.01f}
        },
        {
            {{0.5f, 0.0f, 1.0f}, 1.0f, 0.25f, 0.2f, 16.0f},
            {{0.0f, 0.5f, 1.0f}, 1.0f, 0.25f, 0.2f, 16.0f},
            {{1.0f, 1.0f, 1.0f}, 0.0f, 1.0f, 1.0f, 128.0f}
        },
        {std::move(cube.vertices), std::move(rect.vertices)},
        {std::move(cube.tris), std::move(rect.tris)},
        {
            {
                0, 0,
                {{1.0f, 0.0f, 5.0f}, glm::angleAxis(glm::pi<float>() / 2.0f, glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f}))}
            },
            {
                0, 1,
                {
                    {-1.0f, 0.0f, 5.0f},
                    glm::angleAxis(-glm::pi<float>() / 4.0f, glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f})),
                    {0.5f, 0.5f, 0.5f}
                }
            },
            {
                1, 2,
                {
                    {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {3.0f, 3.0f, 1.0f}
                }
            }
        }
    };

    std::vector<std::vector<Vertex>> meshVerticesWorld;
    meshVerticesWorld.reserve(scene.meshInstances.size());
    for (auto const& instance : scene.meshInstances) {
        auto const modelTransform = instance.transform.matrix();
        auto const normalTransform = ::normalTransform(modelTransform);
        meshVerticesWorld.push_back(scene.meshVertices[instance.mesh]);
        for (auto& vertex : meshVerticesWorld.back()) {
            vertex.position = modelTransform * glm::vec4{vertex.position, 1.0f};
            vertex.normal = glm::normalize(normalTransform * vertex.normal);
        }
    }

    auto const pixelToRayTransform = ::pixelToRayTransform(scene.camera.forward(), scene.camera.up(), scene.camera.fov,
        IMAGE_WIDTH, IMAGE_HEIGHT);

    RenderData const renderData{
        IMAGE_WIDTH, IMAGE_HEIGHT,
        pixelToRayTransform, scene.camera.position,
        {Span{scene.pointLights}, {}, {}},
        {Span{meshVerticesWorld}, Span{scene.meshTris}, Span{scene.meshInstances}},
        Span{scene.materials},
        RAY_TRACE_DEPTH
    };
    render(renderData, Span{renderBuffer});

    std::cout << "Render done" << std::endl;

    linearTo8BitSRGB(Span{renderBuffer}, Span{imageBuffer});

    std::ofstream output{"output.ppm", std::ofstream::binary | std::ofstream::out};
    output << "P6\n";
    output << IMAGE_WIDTH << ' ' << IMAGE_HEIGHT << '\n';
    output << "255\n";
    static_assert(sizeof(Pixel) == 3 && alignof(Pixel) == 1);
    output.write(reinterpret_cast<char const*>(imageBuffer.data()), imageBuffer.size() * sizeof(Pixel));
}
