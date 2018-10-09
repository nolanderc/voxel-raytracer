//
// Created by christofer on 2018-06-20.
//

#include "Octree.h"

Octree::Octree(uchar size) {
    nodes.emplace_back(size);
}

void Octree::insert(int x, int y, int z, uint color) {
    this->resizeToFit(x, y, z);

    // Create nodes until the right position is found
    uchar size = nodes[0].size;
    Node* currentNode = &nodes[0];

    while (size > 0) {
        uchar childIndex = 0b111;

        if (x < 0) childIndex ^= 0b100;
        if (y < 0) childIndex ^= 0b010;
        if (z < 0) childIndex ^= 0b001;

        int childGlobalIndex = currentNode->children[childIndex];

        if (childGlobalIndex == 0) {
            auto newIndex = static_cast<uint>(this->nodes.size());
            currentNode->children[childIndex] = newIndex;
            this->nodes.emplace_back(size - 1);
            childGlobalIndex = newIndex;
        }

        currentNode = &this->nodes[childGlobalIndex];

        size -= 1;

        uint realSize = 1u << (size - 1);
        x += (x < 0 ? realSize : -realSize);
        y += (y < 0 ? realSize : -realSize);
        z += (z < 0 ? realSize : -realSize);
    }

    currentNode->children[0] = color;
}


/// Change the size of the root to fit a new position
///
/// The change in size is accomplished by making the root increasingly larger and moving
/// children down a level:
///
///
///                           +-------+-------+-------+-------+
///                           |       |       |       |       |
///                           |   *   |   *   |   *   |   *   |
///                           |       |       |       |       |
///  +-------+-------+        +-------+-------+-------+-------+
///  |       |       |        |       |       |       |       |
///  |   1   |   3   |        |   *   |   1   |   3   |   *   |
///  |       |       |        |       |       |       |       |
///  +-------O-------+   =>   +-------+-------O-------+-------+
///  |       |       |        |       |       |       |       |
///  |   0   |   2   |        |   *   |   0   |   2   |   *   |
///  |       |       |        |       |       |       |       |
///  +-------+-------+        +-------+-------+-------+-------+
///                           |       |       |       |       |
///                           |   *   |   *   |   *   |   *   |
///                           |       |       |       |       |
///                           +-------+-------+-------+-------+
///
///
/// 1-4     == pre-existing bricks
/// O       == origin
/// *       == new brick
///
void Octree::resizeToFit(int x, int y, int z) {
    uchar size = nodes[0].size;
    auto halfSize = static_cast<int>(1u << (size - 1));

    while (x < -halfSize || halfSize <= x ||
                    y < -halfSize || halfSize <= y ||
                    z < -halfSize || halfSize <= z)
    {
        for (int i = 0; i < 8; ++i) {
            uint globalChildIndex = nodes[0].children[i];

            if (globalChildIndex != 0) {
                Node node = Node(size);

                node.children[7 - i] = globalChildIndex;
                nodes[0].children[i] = static_cast<uint>(nodes.size());

                nodes.push_back(node);
            }
        }

        nodes[0].size++;
        halfSize *= 2;
    }
}

std::vector<Node> Octree::getNodes() {
    return this->nodes;
}
