#pragma once

#include "basic_types.hpp"
#include "geometry.hpp"
#include "mesh.hpp"
#include "utility/numeric.hpp"
#include "utility/permuted_span.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>


struct LineMeshIntersection {
    float t;                    // Line equation parameter.
    float pointCoord2;          // Barycentric coordinate relative to vertex 2.
    float pointCoord3;          // Barycentric coordinate relative to vertex 3.
    PackedFVec3 point;          // Intersection point.
    MeshTriIndex meshTriIndex;  // Index of intersected tri.
};


class BSPTree {
public:
    BSPTree(Span<PackedFVec3 const> vertexPositions, Span<VertexRange const> vertexRanges,
            Span<MeshTri const> tris, PermutedSpan<TriRange const, MeshIndex> triRanges,
            Span<PreprocessedTri const> preprocessedTris, Span<TriRange const> preprocessedTriRanges,
            BoundingBox const& box) :
        _root{}, _inodes{}, _leaves{}
    {
        auto const approxLeaves = (preprocessedTris.size() + Leaf::MAX_TRIS - 1) / Leaf::MAX_TRIS;
        _leaves.reserve(approxLeaves);
        auto const approxInodes = std::max<std::size_t>(approxLeaves, 1) - 1;
        _inodes.reserve(approxInodes);

        _root = _createNode(vertexPositions, vertexRanges, tris, triRanges, preprocessedTris, preprocessedTriRanges,
            box, _inodes, _leaves, 0);
    }

    template<SurfaceConsideration Surfaces>
    std::optional<LineMeshIntersection> lineTriNearestIntersection(Line const& line, float tMin) const {
        struct Traverser {
            Line const& line;
            Span<INode const> inodes;
            Span<Leaf const> leaves;
            float tMin;

            std::optional<LineMeshIntersection> visitLeaf(BoundingBox const& box, Leaf const& leaf) const {
                assert(leaf.triCount > 0);

                std::array<LineTrisIntersection, Leaf::MAX_TRI_BLOCKS> intersections;
                auto const blockCount = (leaf.triCount + 7u) / 8u;
                for (unsigned i = 0; ;) {
                    intersections[i] = lineTrisIntersection<Surfaces>(line, leaf.tris[i]);
                    ++i;
                    if (i >= blockCount) {
                        break;
                    }
                }

                LineMeshIntersection nearestIntersection{INFINITY};
                bool hasIntersection = false;
                for (unsigned triIndex = 0; ;) {
                    auto const blockIndex = triIndex / 8;
                    auto const i = triIndex % 8;
                    auto const& blockIntersections = intersections[blockIndex];
                    if (blockIntersections.exists[i]) {
                        auto const t = blockIntersections.t[i];
                        if (t < nearestIntersection.t && t >= tMin) {
                            auto const point = line(t);
                            auto const inBox = point.x >= box.min.x && point.x <= box.max.x
                                            && point.y >= box.min.y && point.y <= box.max.y
                                            && point.z >= box.min.z && point.z <= box.max.z;
                            if (inBox) {
                                nearestIntersection = {
                                    t, blockIntersections.pointCoord2[i],
                                    blockIntersections.pointCoord3[i],
                                    point, leaf.triIndices[triIndex]};
                                hasIntersection = true;
                            }
                        }
                    }
                    ++triIndex;
                    if (triIndex >= leaf.triCount) {
                        break;
                    }
                }
                if (hasIntersection) {
                    return {nearestIntersection};
                }
                else {
                    return std::nullopt;
                }
            }

            std::optional<LineMeshIntersection> visitNode(Node const& node) const {
                if (lineIntersectsBox(line, node.box)) {
                    if (node.index > 0) {
                        return visitInode(inodes[node.index - 1]);
                    }
                    else if (node.index < 0) {
                        return visitLeaf(node.box, leaves[-(node.index + 1)]);
                    }
                    // Else empty leaf.
                }
                return std::nullopt;
            }

            std::optional<LineMeshIntersection> visitInode(INode const& inode) const {
                float planeToLineOrigin = 0.0f;
                assert(inode.divisionAxis < 3);
                switch (inode.divisionAxis) {
                case 0:
                    planeToLineOrigin = line.origin.x - inode.positiveChild.box.min.x;
                    break;
                case 1:
                    planeToLineOrigin = line.origin.y - inode.positiveChild.box.min.y;
                    break;
                case 2:
                    planeToLineOrigin = line.origin.z - inode.positiveChild.box.min.z;
                    break;
                }
                auto const positiveNear = planeToLineOrigin >= 0.0f;

                auto const& nearChild = positiveNear ? inode.positiveChild : inode.negativeChild;
                if (auto const intersection = visitNode(nearChild)) {
                    return intersection;
                }
                auto const& farChild = !positiveNear ? inode.positiveChild : inode.negativeChild;
                return visitNode(farChild);
            }
        };

        return Traverser{line, readOnlySpan(_inodes), readOnlySpan(_leaves), tMin}.visitNode(_root);
    }

private:
    struct Node {
        BoundingBox box;
        std::int32_t index;     // index < 0: leaf at (-index - 1)
                                // index = 0: empty leaf
                                // index > 0: inode at (index - 1)
    };

    struct INode {
        Node negativeChild;             // On side of -ve axis direction
        Node positiveChild;             // On side of +ve axis direction
        std::uint8_t divisionAxis;      // 0 (X), 1 (Y), 2 (Z)
    };

    struct Leaf {
        constexpr inline static std::uint8_t MAX_TRIS = 32;
        constexpr inline static std::uint8_t MAX_TRI_BLOCKS = (MAX_TRIS + 7) / 8;

        std::array<PreprocessedTriBlock, MAX_TRI_BLOCKS> tris;
        std::array<MeshTriIndex, MAX_TRIS> triIndices;
        std::uint8_t triCount;
    };

    Node _root;
    std::vector<INode> _inodes;
    std::vector<Leaf> _leaves;

    static Node _createNode(Span<PackedFVec3 const> vertexPositions, Span<VertexRange const> vertexRanges,
            Span<MeshTri const> tris, PermutedSpan<TriRange const, MaterialIndex> triRanges,
            Span<PreprocessedTri const> preprocessedTris, Span<TriRange const> preprocessedTriRanges,
            BoundingBox const& box, std::vector<INode>& inodes, std::vector<Leaf>& leaves,
            std::uint8_t divisionAxis = 0) {
        assert(vertexRanges.size() == triRanges.size());
        assert(divisionAxis < 3);

        {
            auto const instanceCount = intCast<MeshIndex>(vertexRanges.size());
            bool subdivide = false;
            std::array<PreprocessedTri, Leaf::MAX_TRIS> trisInBox{};
            std::array<MeshTriIndex, Leaf::MAX_TRIS> triIndicesInBox{};
            std::uint8_t inBoxCount = 0;
            for (MeshIndex instanceIndex = 0; instanceIndex < instanceCount && !subdivide; ++instanceIndex) {
                auto const instanceTris = tris[triRanges[instanceIndex]];
                auto const instanceVertexPositions = vertexPositions[vertexRanges[instanceIndex]];
                auto const instancePreprocessedTris = preprocessedTris[preprocessedTriRanges[instanceIndex]];
                auto const triCount = intCast<TriIndex>(instanceTris.size());
                for (TriIndex triIndex = 0; triIndex < triCount && !subdivide; ++triIndex) {
                    auto const& meshTri = instanceTris[triIndex];
                    Tri const tri{
                        instanceVertexPositions[meshTri.v1],
                        instanceVertexPositions[meshTri.v2],
                        instanceVertexPositions[meshTri.v3]
                    };
                    if (triIntersectsBox(tri, box)) {
                        if (inBoxCount < Leaf::MAX_TRIS) {
                            trisInBox[inBoxCount] = instancePreprocessedTris[triIndex];
                            triIndicesInBox[inBoxCount] = {instanceIndex, triIndex};
                            ++inBoxCount;
                        }
                        else {
                            subdivide = true;
                        }
                    }
                }
            }
            if (!subdivide) {
                if (inBoxCount == 0) {
                    return {box, 0};
                }
                else {
                    auto const blockCount = (inBoxCount + 7u) / 8u;
                    std::array<PreprocessedTriBlock, Leaf::MAX_TRI_BLOCKS> triBlocks{};
                    for (unsigned i = 0; i < blockCount; ++i) {
                        auto const offset = i * 8;
                        triBlocks[i] = {
                            {
                                trisInBox[offset].normal, trisInBox[offset + 1].normal,
                                trisInBox[offset + 2].normal, trisInBox[offset + 3].normal,
                                trisInBox[offset + 4].normal, trisInBox[offset + 5].normal,
                                trisInBox[offset + 6].normal, trisInBox[offset + 7].normal
                            },
                            {
                                trisInBox[offset].v1, trisInBox[offset + 1].v1,
                                trisInBox[offset + 2].v1, trisInBox[offset + 3].v1,
                                trisInBox[offset + 4].v1, trisInBox[offset + 5].v1,
                                trisInBox[offset + 6].v1, trisInBox[offset + 7].v1
                            },
                            {
                                trisInBox[offset].v1ToV2, trisInBox[offset + 1].v1ToV2,
                                trisInBox[offset + 2].v1ToV2, trisInBox[offset + 3].v1ToV2,
                                trisInBox[offset + 4].v1ToV2, trisInBox[offset + 5].v1ToV2,
                                trisInBox[offset + 6].v1ToV2, trisInBox[offset + 7].v1ToV2
                            },
                            {
                                trisInBox[offset].v1ToV3, trisInBox[offset + 1].v1ToV3,
                                trisInBox[offset + 2].v1ToV3, trisInBox[offset + 3].v1ToV3,
                                trisInBox[offset + 4].v1ToV3, trisInBox[offset + 5].v1ToV3,
                                trisInBox[offset + 6].v1ToV3, trisInBox[offset + 7].v1ToV3
                            }
                        };
                    }
                    leaves.push_back({triBlocks, triIndicesInBox, inBoxCount});
                    return {box, -intCast<std::int32_t>(leaves.size())};
                }
            }
        }

        auto negativeSubbox = box;
        auto positiveSubbox = box;
        switch (divisionAxis) {
        case 0: {
                auto const centre = (box.min.x + box.max.x) / 2.0f;
                negativeSubbox.max.x = centre;
                positiveSubbox.min.x = centre;
                break;
            }
        case 1: {
                auto const centre = (box.min.y + box.max.y) / 2.0f;
                negativeSubbox.max.y = centre;
                positiveSubbox.min.y = centre;
                break;
            }
        case 2: {
                auto const centre = (box.min.z + box.max.z) / 2.0f;
                negativeSubbox.max.z = centre;
                positiveSubbox.min.z = centre;
                break;
            }
        }
        std::uint8_t const nextDivisionAxis = (divisionAxis + 1) % 3;
        auto const index = inodes.size();
        // Insert inode before recursing so they're in traversal order.
        inodes.push_back({{}, {}, divisionAxis});
        inodes[index].negativeChild = _createNode(vertexPositions, vertexRanges, tris, triRanges, preprocessedTris,
            preprocessedTriRanges, negativeSubbox, inodes, leaves, nextDivisionAxis);
        inodes[index].positiveChild = _createNode(vertexPositions, vertexRanges, tris, triRanges, preprocessedTris,
            preprocessedTriRanges, positiveSubbox, inodes, leaves, nextDivisionAxis);
        return {box, intCast<std::int32_t>(index + 1)};
    }
};
