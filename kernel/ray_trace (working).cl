
typedef struct {
    // The logarithmic size of this node
    uchar size;

    // The indices of this node's children.
    // If the index is 0 the child is empty
    uint children[8];
} Node;

// Calculate a vector * matrix multiplication
float4 mul(float4 v, float16 m) {
    return (float4) (
        dot(v, m.s048c),
        dot(v, m.s159d),
        dot(v, m.s26ae),
        dot(v, m.s37bf)
    );
}


// Calulate the direction of a ray leaving a camera
float3 ray_direction(float screen_x, float screen_y, float16 inv_matrix) {
    float4 screen_near = (float4)(screen_x, screen_y, -1.0, 1.0);
    float4 screen_far = (float4)(screen_x, screen_y, 1.0, 1.0);

    float4 world_near = mul(screen_near, inv_matrix);
    float4 world_far = mul(screen_far, inv_matrix);

    float near_inv_w = 1.0 / world_near.w;
    float far_inv_w = 1.0 / world_far.w;

    float3 near = world_near.xyz * near_inv_w;
    float3 far = world_far.xyz * far_inv_w;

    return normalize(far - near);
}


bool circle_intersection(float3 center, float radius, float3 direction, float3* normal, float* pDistance) {
    float t_middle = dot(center, direction);
    float3 point = direction * t_middle;
    float dist = distance(point, center);

    if (dist < radius) {
        float t0 = (t_middle - sqrt(radius*radius - dist*dist));
        if (t0 < 0) return false;

        *pDistance = t0;
        float3 intersection = t0 * direction;
        *normal = normalize(intersection - center);
        return true;
    } else {
        return false;
    }
}

/*
bool cubeIntersection(float3 position, float size, float3 origin, float3 direction, float3* pNormal, float* pDistance) {
    float3 halfSizes = (float3)(size / 2.0f);

    // The position of the cube relative to the ray's origin
    float3 relativePosition = position - origin;

    // Calculate the near and far planes
    float3 step = sign(direction);
    float3 near = relativePosition - step * halfSizes;
    float3 far = relativePosition + step * halfSizes;

    // Calculate entry and exit times for ray
    float3 absDirection = 1.0f / direction;
    float3 entry = near * absDirection;
    float3 exit = far * absDirection;

    // Check for existing collision
    if (direction.x == 0.0) { if (relativePosition.x + halfSizes.x < 0.0 || relativePosition.x - halfSizes.x > 0.0) { return false; } entry.x = -INFINITY; exit.x = INFINITY; }
    if (direction.y == 0.0) { if (relativePosition.y + halfSizes.y < 0.0 || relativePosition.y - halfSizes.y > 0.0) { return false; } entry.y = -INFINITY; exit.y = INFINITY; }
    if (direction.z == 0.0) { if (relativePosition.z + halfSizes.z < 0.0 || relativePosition.z - halfSizes.z > 0.0) { return false; } entry.z = -INFINITY; exit.z = INFINITY; }

    // Calculate the final entry and exit times
    float lastEntry = max(entry.x, max(entry.y, entry.z));
    float firstExit = min(exit.x, min(exit.y, exit.z));

    if (lastEntry < firstExit && lastEntry > 0.0) {
        *pDistance = lastEntry;

        // Find the normal
        *pNormal = (float3)(0.0, 0.0, 0.0);

        if (entry.x > entry.y) {
            if (entry.x > entry.z) {
                pNormal->x = -step.x;
            } else {
                pNormal->z = -step.z;
            }
        } else {
            if (entry.y > entry.z) {
                pNormal->y = -step.y;
            } else {
                pNormal->z = -step.z;
            }
        }

        return true;
    } else {
        return false;
    }
}
 */

bool voxelIntersection(float3 position, float size, float3 origin, float3 direction, float3* tEntry, float3* tExit) {
    float3 halfSizes = (float3)(size / 2.0f);

    // The position of the cube relative to the ray's origin
    float3 relativePosition = position - origin;

    // Calculate the near and far planes
    float3 step = sign(direction);
    float3 near = relativePosition - step * halfSizes;
    float3 far = relativePosition + step * halfSizes;

    // Calculate entry and exit times for ray
    float3 absDirection = 1.0f / direction;
    float3 entry = near * absDirection;
    float3 exit = far * absDirection;

    // Check for existing collision
    if (direction.x == 0.0) { if (relativePosition.x + halfSizes.x < 0.0 || relativePosition.x - halfSizes.x > 0.0) { return false; } entry.x = -INFINITY; exit.x = INFINITY; }
    if (direction.y == 0.0) { if (relativePosition.y + halfSizes.y < 0.0 || relativePosition.y - halfSizes.y > 0.0) { return false; } entry.y = -INFINITY; exit.y = INFINITY; }
    if (direction.z == 0.0) { if (relativePosition.z + halfSizes.z < 0.0 || relativePosition.z - halfSizes.z > 0.0) { return false; } entry.z = -INFINITY; exit.z = INFINITY; }

    // Calculate the final entry and exit times
    float lastEntry = max(entry.x, max(entry.y, entry.z));
    float firstExit = min(exit.x, min(exit.y, exit.z));

    if (lastEntry < firstExit) {
        *tEntry = entry;
        *tExit = exit;

        return true;
    } else {
        return false;
    }
}


float3 reflect(float3 incident, float3 normal) {
    return incident - 2.0f * dot(normal, incident) * normal;
}


uint firstChild(float tEnter, float3 tMid) {
    uint index = 0b000;

    if (tEnter > tMid.x) index ^= 0b100;
    if (tEnter > tMid.y) index ^= 0b010;
    if (tEnter > tMid.z) index ^= 0b001;

    return index;
}


void getChildT(uint childIndex, float3 t0, float3 tMid, float3 t1, float3* t0Child, float3* t1Child) {
    if ((childIndex & 0b100) == 0) {
        t0Child->x = t0.x;
        t1Child->x = tMid.x;
    } else {
        t0Child->x = tMid.x;
        t1Child->x = t1.x;
    }

    if ((childIndex & 0b010) == 0) {
        t0Child->y = t0.y;
        t1Child->y = tMid.y;
    } else {
        t0Child->y = tMid.y;
        t1Child->y = t1.y;
    }

    if ((childIndex & 0b001) == 0) {
        t0Child->z = t0.z;
        t1Child->z = tMid.z;
    } else {
        t0Child->z = tMid.z;
        t1Child->z = t1.z;
    }
}


uint getNextChild(uint prevChildIndex, float3 t1, bool* exitNode) {
    uint index = prevChildIndex;

    if (t1.x < t1.y) {
        if (t1.x < t1.z) {
            if ((index & 0b100) != 0) { *exitNode = true; }
            index |= 0b100;
        } else {
            if ((index & 0b001) != 0) { *exitNode = true; }
            index |= 0b001;
        }
    } else {
        if (t1.y < t1.z) {
            if ((index & 0b010) != 0) { *exitNode = true; }
            index |= 0b010;
        } else {
            if ((index & 0b001) != 0) { *exitNode = true; }
            index |= 0b001;
        }
    }

    return index;
}


bool traceOctree(__global Node* voxels, float3 origin, float3 direction, int* iterations, float3* normal, float* distance) {
    uint dirMask = (direction.x < 0.0 ? 4 : 0) + (direction.y < 0.0 ? 2 : 0) + (direction.z < 0.0 ? 1 : 0);

    Node root = voxels[0];
    uint realSize = 1 << root.size;
    float3 t0, t1;
    float t;
    if (voxelIntersection((float3)(0.0), realSize, origin, direction, &t0, &t1)) {
        if (t1.x < 0.0f || t1.y < 0.0f || t1.z < 0.0f) return false;

        t = max(t0.x, max(t0.y, t0.z));
        if (t < 0.0) t = 0.0;

        float3 tMid = 0.5f * (t0 + t1);

        uint childIndex = firstChild(t, tMid);



        // Create a stack
        typedef struct {
            uint index, childIndex;
            float3 t0, tMid, t1;
        } Stack;

        Stack stack[6];
        uint stackLen = 0;

        Node node = root;
        uint index = 0;

        *iterations = 0;
        while (true) {
            *iterations += 1;
            uint childGlobalIndex = node.children[childIndex ^ dirMask];

            float3 t0Child, t1Child;
            getChildT(childIndex, t0, tMid, t1, &t0Child, &t1Child);

            bool exitNode = false;
            uint nextChild = getNextChild(childIndex, t1Child, &exitNode);

            if (childGlobalIndex != 0) {
                Node child = voxels[childGlobalIndex];

                // Leaf node
                if (child.size == 0) {
                    float tEntry = max(t0Child.x, max(t0Child.y, t0Child.z));
                    *distance = tEntry;
                    *normal = (float3)(0.0f);
                    if (tEntry == t0Child.x) { normal->x = -sign(direction.x); }
                    if (tEntry == t0Child.y) { normal->y = -sign(direction.y); }
                    if (tEntry == t0Child.z) { normal->z = -sign(direction.z); }

                    return true;
                };

                if (!exitNode) {
                    // Push the new node to the stack
                    Stack s;
                    s.index = index;
                    s.childIndex = nextChild;
                    s.t0 = t0;
                    s.tMid = tMid;
                    s.t1 = t1;

                    stack[stackLen] = s;
                    stackLen++;
                }

                node = child;
                index = childGlobalIndex;
                t0 = t0Child;
                t1 = t1Child;
                tMid = 0.5f * (t0Child + t1Child);

                childIndex = firstChild(t, tMid);

                continue;
            }

            if (exitNode) {
                if (stackLen == 0) return false;

                // Pop the stack
                stackLen--;
                Stack s = stack[stackLen];

                t = min(t1.x, min(t1.y, t1.z));

                index = s.index;
                childIndex = s.childIndex;
                t0 = s.t0;
                tMid = s.tMid;
                t1 = s.t1;


                node = voxels[index];

                continue;
            }

            childIndex = nextChild;
            t = min(t1Child.x, min(t1Child.y, t1Child.z));
        }

        return false;
    } else {
        return false;
    }
}


__kernel void ray_trace(__write_only image2d_t pixels, float16 invMatrix, float3 eye, float time, float3 lightPosition, __global Node* voxels) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const int width = get_image_width(pixels);
    const int height = get_image_height(pixels);


    float screen_x = (float)x / (float)width * 2.0 - 1.0;
    float screen_y = (float)y / (float)height * 2.0 - 1.0;

    float3 direction = ray_direction(screen_x, screen_y, invMatrix);


    float4 color = (float4)(0.0, 0.0, 0.0, 1.0);

    float3 center = (float3)(0.0, 0.0, 3.0);

    float3 normal;
    float distance;

    // Test for intersection with root
    int iterations = 0;
    color.xyz = fabs(direction);
    if (traceOctree(voxels, eye, direction, &iterations, &normal, &distance)) {
        color.xyz = fabs(normal);
    } else {
        color.xyz *= (float)iterations / 50.0f;
    }


    /*
    float3 normal = (float3)(1.0, 1.0, 1.0);
    float distance = INFINITY;

    float3 n;
    float d;

     for (int i = 0; i < cubeCount; ++i) {
        float4 cube = cubes[i];
        if (cubeIntersection(cube.xyz, cube.w, eye, direction, &n, &d)) {
            if (d < distance) {
                distance = d;
                normal = n;
            }
        }
    }

    if (distance != INFINITY) {
        float3 hit = eye + (distance - 1e-5f) * direction;
        float3 lightDirection = normalize(lightPosition - hit);

        float diff = 0.8 * dot(normal, lightDirection);

        float spec = pow(max(0.0f, dot(reflect(-lightDirection, normal), -direction)), 8);

        float shadow = 1.0;


        for (int i = 0; i < cubeCount; ++i) {
            float4 cube = cubes[i];
            if (cubeIntersection(cube.xyz, cube.w, hit, lightDirection, &n, &d)) {
                shadow = 0.0;
                break;
            }
        }


        color.xyz = max(0.0f, diff + 0.1f * spec) * shadow + 0.1f;
    }
    */

    write_imagef(
        pixels,
        (int2)(x, y),
        color
    );
}

