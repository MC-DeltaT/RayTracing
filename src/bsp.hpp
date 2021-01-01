#pragma once

#include "geometry.hpp"
#include "mesh.hpp"
#include "utility/misc.hpp"
#include "utility/permuted_span.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>


struct LineMeshIntersection : LineTriIntersection {
    MeshTriIndex meshTriIndex;
};


class BSPTree {
public:
    BSPTree(Span<glm::vec3 const> vertexPositions, Span<IndexRange const> vertexRanges,
            Span<MeshTri const> tris, PermutedSpan<IndexRange const> triRanges,
            Span<PreprocessedTri const> preprocessedTris, Span<IndexRange const> preprocessedTriRanges,
            BoundingBox const& box) :
        _preprocessedTris{preprocessedTris}, _preprocessedTriRanges{preprocessedTriRanges}, _root{}, _inodes{}, _leaves{}
    {
        {
            auto const approxLeaves = static_cast<std::size_t>(
                std::ceil(tris.size() / static_cast<double>(Leaf::MAX_TRIS)));
            _leaves.reserve(approxLeaves);
            auto const approxInodes = std::max<std::size_t>(approxLeaves, 1) - 1;
            _inodes.reserve(approxInodes);
        }
        _root = _createNode(vertexPositions, vertexRanges, tris, triRanges, _inodes, _leaves, box);

        // Inodes get inserted in reverse order of traversal.
        // We reverse the order here so traversal is better for the cache.
        auto const inodeCount = _inodes.size();
        auto const reverseNodeIndex = [inodeCount](Node& node) {
            if (!node.isLeaf) {
                node.index = inodeCount - node.index - 1;
            }
        };
        reverseNodeIndex(_root);
        std::reverse(_inodes.begin(), _inodes.end());
        for (auto& node : _inodes) {
            reverseNodeIndex(node.negativeChild);
            reverseNodeIndex(node.positiveChild);
        };
    }

    template<SurfaceConsideration Surfaces>
    std::optional<LineMeshIntersection> lineTriNearestIntersection(Line const& line, float tMin) const {
        struct Traverser {
            Span<PreprocessedTri const> preprocessedTris;
            Span<IndexRange const> preprocessedTriRanges;
            std::vector<INode> const& inodes;
            std::vector<Leaf> const& leaves;
            Line const& line;
            float tMin;

            std::optional<LineMeshIntersection> operator()(BoundingBox const& box, Leaf const& leaf) const {
                LineMeshIntersection nearestIntersection{{INFINITY}};
                bool hasIntersection = false;
                for (unsigned i = 0; i < leaf.triCount; ++i) {
                    auto const& meshTriIndex = leaf.tris[i];
                    auto const& triRange = preprocessedTriRanges[meshTriIndex.mesh];
                    auto const& tri = preprocessedTris[triRange][meshTriIndex.tri];
                    if (auto const intersection = lineTriIntersection<Surfaces>(line, tri, tMin, nearestIntersection.t)) {
                        auto const point = line(intersection->t);
                        if (inBox(point, box)) {
                            nearestIntersection = {*intersection, meshTriIndex};
                            hasIntersection = true;
                        }
                    }
                }
                if (hasIntersection) {
                    return {nearestIntersection};
                }
                else {
                    return std::nullopt;
                }
            }

            std::optional<LineMeshIntersection> operator()(Node const& node) const {
                if (lineIntersectsBox(line, node.box)) {
                    if (node.isLeaf) {
                        if (node.index > 0) {
                            return (*this)(node.box, leaves[node.index - 1]);
                        }
                    }
                    else {
                        return (*this)(inodes[node.index]);
                    }
                }
                return std::nullopt;
            }

            std::optional<LineMeshIntersection> operator()(INode const& inode) const {
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
                if (auto const intersection = (*this)(nearChild)) {
                    return intersection;
                }
                auto const& farChild = !positiveNear ? inode.positiveChild : inode.negativeChild;
                return (*this)(farChild);
            }
        };

        return Traverser{_preprocessedTris, _preprocessedTriRanges, _inodes, _leaves, line, tMin}(_root);
    }

private:
    struct Node {
        BoundingBox box;
        bool isLeaf;
        std::size_t index;
    };

    struct INode {
        unsigned char divisionAxis;     // 0 (X), 1 (Y), 2 (Z)
        Node negativeChild;             // On side of -ve axis direction
        Node positiveChild;             // On side of +ve axis direction
    };

    struct Leaf {
        constexpr inline static unsigned char MAX_TRIS = 32;

        std::array<MeshTriIndex, MAX_TRIS> tris;
        unsigned char triCount;
    };

    Span<PreprocessedTri const> _preprocessedTris;
    Span<IndexRange const> _preprocessedTriRanges;
    Node _root;
    std::vector<INode> _inodes;
    std::vector<Leaf> _leaves;

    static Node _createNode(Span<glm::vec3 const> vertexPositions, Span<IndexRange const> vertexRanges,
            Span<MeshTri const> tris, PermutedSpan<IndexRange const> triRanges, std::vector<INode>& inodes,
            std::vector<Leaf>& leaves, BoundingBox const& box, unsigned char divisionAxis = 0) {
        assert(vertexRanges.size() == triRanges.size());
        assert(divisionAxis < 3);

        {
            auto const instanceCount = vertexRanges.size();
            bool subdivide = false;
            std::array<MeshTriIndex, Leaf::MAX_TRIS> trisInBox{};
            unsigned char inBoxCount = 0;
            for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
                auto const instanceTris = tris[triRanges[instanceIndex]];
                auto const instanceVertexPositions = vertexPositions[vertexRanges[instanceIndex]];
                for (std::size_t triIndex = 0; triIndex < instanceTris.size(); ++triIndex) {
                    auto const& meshTri = instanceTris[triIndex];
                    Tri tri{
                        instanceVertexPositions[meshTri.i1],
                        instanceVertexPositions[meshTri.i2],
                        instanceVertexPositions[meshTri.i3]
                    };
                    if (triIntersectsBox(tri, box)) {
                        if (inBoxCount >= Leaf::MAX_TRIS) {
                            subdivide = true;
                            break;
                        }
                        trisInBox[inBoxCount] = {instanceIndex, triIndex};
                        ++inBoxCount;
                    }
                }
            }
            if (!subdivide) {
                if (inBoxCount == 0) {
                    return {box, true, 0};      // 0 node index = empty leaf
                }
                leaves.push_back({trisInBox, inBoxCount});
                return {box, true, leaves.size()};
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
        unsigned char nextDivisionAxis = (divisionAxis + 1) % 3;
        inodes.push_back({
            divisionAxis,
            _createNode(vertexPositions, vertexRanges, tris, triRanges, inodes, leaves, negativeSubbox, nextDivisionAxis),
            _createNode(vertexPositions, vertexRanges, tris, triRanges, inodes, leaves, positiveSubbox, nextDivisionAxis)
        });
        return {box, false, inodes.size() - 1};
    }
};
