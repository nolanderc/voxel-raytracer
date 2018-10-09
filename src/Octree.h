//
// Created by christofer on 2018-06-20.
//

#pragma once


#include <vector>

typedef unsigned char uchar;
typedef unsigned int uint;

struct Node {
    // The logarithmic size of this node
    uchar size;

    // The indices of this node's children.
    // If the index is 0 the child is empty
    uint children[8];


    explicit Node(uchar size, std::vector<uint> children = {0, 0, 0, 0, 0, 0, 0, 0}) :
            size(size), children{0} {
        for (int i = 0; i < 8; ++i) {
            this->children[i] = children[i];
        }
    }
};


class Octree {

    std::vector<Node> nodes;

public:

    explicit Octree(uchar size);

    void insert(int x, int y, int z, uint color);

    std::vector<Node> getNodes();

private:
    void resizeToFit(int x, int y, int z);
};



