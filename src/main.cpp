#include <vector>
#include <stdexcept>
#include <iterator>
#include <fstream>
#include <chrono>


#include "OpenCL.h"
#include "Octree.h"

#include "lodepng/lodepng.h"


#include "glm/glm.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/matrix_transform.hpp"


#include <GL/glew.h>

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <GL/glx.h>


#include <CL/cl_gl.h>


/// Find a platform and a device which supports a certain type of device
void findDevicePlatform(cl_device_type type, GLFWwindow *window, cl_platform_id *platform_id, cl_device_id *device_id) {
    // Enumerate all platforms
    cl_uint platformCount;
    clGetPlatformIDs(0, nullptr, &platformCount);

    Log().get(INFO) << "Found " << platformCount << " platform(s)";

    std::vector<cl_platform_id> platforms(platformCount);
    clGetPlatformIDs(platformCount, platforms.data(), nullptr);

    // Use the first platform that supports a GPU
    for (auto platform : platforms) {

        // Create context
        const cl_context_properties contextProperties[] = {
                CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(glfwGetGLXContext(window)),
                CL_GLX_DISPLAY_KHR, reinterpret_cast<cl_context_properties>(glfwGetX11Display()),
                CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform),
                0, 0
        };


        cl_device_id glDevices[32];
        size_t size;

        cl_int error = clGetGLContextInfoKHR(contextProperties, CL_DEVICES_FOR_GL_CONTEXT_KHR,
                                             32 * sizeof(cl_device_id), glDevices, &size);
        checkCLError(error);

        size_t count = size / sizeof(cl_device_id);

        if (count == 0) continue;

        *device_id = glDevices[0];
        *platform_id = platform;

        /*

        // Find a compatible device
        cl_uint deviceCount;
        clGetDeviceIDs(platform, type, 0, nullptr, &deviceCount);

        {
            size_t vendorLength;
            clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 0, nullptr, &vendorLength);

            std::string vendorName;
            vendorName.resize(vendorLength);
            clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, vendorLength, &vendorName[0], nullptr);
            vendorName.pop_back();

            Log().get(INFO) << "Platform (" << vendorName << ") has " << deviceCount << " device(s)";
        }

        // Try the next platform if no GPU was found
        if (deviceCount == 0) continue;






        std::vector<cl_device_id> devices(deviceCount);
        clGetDeviceIDs(platform, type, deviceCount, devices.data(), nullptr);

        // Use the first device available
        *device_id = devices[0];
        *platform_id = platform;

        {
            size_t deviceNameLength;
            clGetDeviceInfo(*device_id, CL_DEVICE_NAME, 0, nullptr, &deviceNameLength);

            std::string deviceName;
            deviceName.resize(deviceNameLength);
            clGetDeviceInfo(*device_id, CL_DEVICE_NAME, deviceNameLength, &deviceName[0], nullptr);
            deviceName.pop_back();

            Log().get(INFO) << "Using device '" << deviceName << "'";



            size_t extensionLength;
            clGetDeviceInfo(*device_id, CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionLength);

            std::string extensions;
            extensions.resize(extensionLength);
            clGetDeviceInfo(*device_id, CL_DEVICE_EXTENSIONS, extensionLength, &extensions[0], nullptr);

            Log().get(INFO) << "Available extensions: " << extensions;
        }*/

        return;
    }
}


/// Load a file from disk and store it as a string
std::string getKernelSource(std::string path) {
    std::ifstream file(path);


    if (file.is_open()) {
        return std::string(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
        );
    } else {
        throw std::runtime_error("Failed to open file");
    }
}


/// Create a context, program and queue
void createProQue(cl_device_id device, cl_platform_id platform, GLFWwindow *window, cl_context *context,
                  cl_command_queue *queue, cl_program *program, const char *path) {
    // Create context
    const cl_context_properties contextProperties[] = {
            CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(glfwGetGLXContext(window)),
            CL_GLX_DISPLAY_KHR, reinterpret_cast<cl_context_properties>(glfwGetX11Display()),
            CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform),
            0, 0
    };


    Log().get(INFO) << "Creating context...";
    cl_int error;
    *context = clCreateContext(contextProperties, 1, &device, nullptr, nullptr, &error);
    checkCLError(error);
    Log().get(INFO) << "Context created!";


    // Create a command queue
    Log().get(INFO) << "Creating queue...";
    *queue = clCreateCommandQueueWithProperties(*context, device, nullptr, &error);
    Log().get(INFO) << "Queue created!";


    // Create a program
    Log().get(INFO) << "Creating program...";
    std::string source = getKernelSource("./kernel/ray_trace.cl");
    source.append("\0");
    const char *sources = &source[0];
    *program = clCreateProgramWithSource(*context, 1, &sources, nullptr, &error);
    checkCLError(error);
    Log().get(INFO) << "Program created!";


    // Build program
    Log().get(INFO) << "Building program...";
    error = clBuildProgram(*program, 1, &device, nullptr, nullptr, nullptr);

    if (error == CL_BUILD_PROGRAM_FAILURE) {
        size_t length;
        clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &length);

        auto *log = new char[length + 1];
        log[length] = '\0';
        clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, length, log, nullptr);

        Log().get(ERROR) << log;
    }

    checkCLError(error);
    Log().get(INFO) << "Program built!";
}


void keyCallback(GLFWwindow *window, int key, int scanCode, int mods, int state) {
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    }
}



GLFWwindow* createWindow(int width, int height, GLFWmonitor* monitor) {
    // Create a window
    GLFWwindow* window = nullptr;
    if (monitor) {
        auto mode = glfwGetVideoMode(monitor);

        window = glfwCreateWindow(mode->width, mode->height, "OpenCL", monitor, nullptr);
    } else {
        window = glfwCreateWindow(width, height, "OpenCL", nullptr, nullptr);
    }

    // Set callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(0);


    // Setup state
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    // Activate the context
    glfwMakeContextCurrent(window);

    // Load OpenGL functions
    if (glewInit()) throw std::runtime_error("Failed to init GLEW!");

    return window;
}


#include <sstream>

Octree loadVox(std::string path) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!" + path);
    }

    /*size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0);

    char* bytes = new char[size + 1];
    bytes[size] = '\0';

    file.read(bytes, size);*/

    std::stringstream ss;
    ss << file.rdbuf();

    // Parse
    Octree octree = Octree(4);

    char magic[4];
    ss.read(magic, 4);

    int version;
    ss.read(reinterpret_cast<char *>(&version), sizeof(int));

    assert(magic[0] == 'V' || magic[1] == 'O' || magic[2] == 'X' || magic[3] == ' ' || version == 150);


    struct Chunk {
        char name[4];
        int content, children;
    };

    auto loadChunk = [&](){
        Chunk c;
        ss.read(reinterpret_cast<char *>(&c), sizeof(Chunk));
        return c;
    };

    Chunk main = loadChunk();

    assert(main.name[0] == 'M' || main.name[1] == 'A' || main.name[2] == 'I' || main.name[3] == 'N');


    int modelCount = 1;

    Chunk pack = loadChunk();
    if (pack.name[0] == 'P' || pack.name[1] == 'A' || pack.name[2] == 'C' || pack.name[3] == 'K') {
        ss.read(reinterpret_cast<char *>(&modelCount), sizeof(int));
    } else {
        ss.seekg(-ss.gcount(), std::ios::cur);
    }

    struct XYZI {
        unsigned char x, y, z, i;
    };

    std::vector<std::vector<XYZI>> models;

    for (int i = 0; i < modelCount; ++i) {
        Chunk size = loadChunk();
        assert(size.name[0] == 'S' || size.name[1] == 'I' || size.name[2] == 'Z' || size.name[3] == 'E');

        int sx, sy, sz;
        ss.read(reinterpret_cast<char *>(&sx), sizeof(int));
        ss.read(reinterpret_cast<char *>(&sy), sizeof(int));
        ss.read(reinterpret_cast<char *>(&sz), sizeof(int));


        Chunk xyzi = loadChunk();

        int numVoxels;
        ss.read(reinterpret_cast<char *>(&numVoxels), sizeof(int));

        std::vector<XYZI> voxels;
        voxels.resize(static_cast<unsigned long>(numVoxels));
        ss.read(reinterpret_cast<char *>(voxels.data()), numVoxels * 4);

        models.push_back(voxels);
    }


    size_t pos = static_cast<size_t>(ss.tellg());
    ss.seekg(0, std::ios::end);
    size_t end = static_cast<size_t>(ss.tellg());
    ss.seekg(pos);

    size_t len = end - pos;

    //region palette
    unsigned int palette[256] = {
            0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc, 0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc, 0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99, 0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699, 0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66, 0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066, 0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00, 0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300, 0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044, 0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
    };
    //endregion



    if (len >= sizeof(Chunk)) {
        Chunk rgba = loadChunk();

        ss.read(reinterpret_cast<char *>(palette), 4 * 255);
    }


    int voxelCount = static_cast<int>(models[0].size());
    for (int j = 0; j < voxelCount; ++j) {
        XYZI voxel = models[0][j];

        octree.insert(voxel.x, voxel.z, voxel.y, palette[voxel.i - 1]);
    }

    return octree;
}


int main() {
    Octree octree = loadVox("vox/monument/monu16.vox");

    //region Init
    Log::setReportingLevel(INFO);
    std::cout << "Hello, World!" << std::endl;

    // Initialize OpenGL
    if (!glfwInit()) throw std::runtime_error("Failed to init GLFW!");

    // Create a window with an OpenGL context
    auto monitor = glfwGetPrimaryMonitor();
    GLFWwindow* window = createWindow(1280, 720, monitor);


    // Create the OpenCL device and platform
    cl_device_id device;
    cl_platform_id platform;

    findDevicePlatform(CL_DEVICE_TYPE_GPU, window, &platform, &device);


    // Create a context, queue and program
    cl_context context;
    cl_command_queue queue;
    cl_program program;

    createProQue(device, platform, window, &context, &queue, &program, "kernel/ray_trace.cl");


    // Create a kernel
    Log().get(INFO) << "Creating kernel...";
    cl_int error;
    cl_kernel kernel = clCreateKernel(program, "ray_trace", &error);
    checkCLError(error);
    Log().get(INFO) << "Kernel created!";



    // Create a texture
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    size_t width = static_cast<size_t>(w);
    size_t height = static_cast<size_t>(h);

    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GLsizei(width), GLsizei(height), GL_FALSE, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);


    // Create a framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);




    // Create an image
    Log().get(INFO) << "Creating image...";

    cl_mem image = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture, &error);
    checkCLError(error);

    Log().get(INFO) << "Image created!";

    error = clSetKernelArg(kernel, 0, sizeof(image), &image);
    checkCLError(error);

    //endregion

    // Define an Octree


    // Create voxels


    std::vector<Node> nodes = octree.getNodes();

    for (int i = 0; i < 7; ++i) {
        nodes[0].children[i] = nodes[0].children[7];
    }

    int size = (1u << nodes[0].size);
    Log().get(INFO) << "Size: " << size << "^3 = " << powl(size, 3);

    cl_mem voxels = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, nodes.size() * sizeof(Node), nodes.data(), &error);
    checkCLError(error);


    error = clSetKernelArg(kernel, 5, sizeof(voxels), &voxels);
    checkCLError(error);

    // Create loop variables
    float time = 0;
    auto last = std::chrono::high_resolution_clock::now();

    float frameTime = 0;
    int frames = 0;


    glm::vec3 eye = glm::vec3(0.0, 0.0, -size);


    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec2 lastMousePos(x, y);
    float yaw = 0.0, pitch = 0.0;


    std::cout << "\n\n";

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now - last;
        last = now;
        float deltaTime = duration.count() * 1e-9;
        time += deltaTime;


        frames++;
        frameTime += deltaTime;
        if (frameTime > 0.5) {
            int fps = (int) round(frames / frameTime);
            std::cout << "FPS: " << fps << std::endl;
            frames = 0;
            frameTime = 0;
        }


        glfwPollEvents();


        double x, y;
        glfwGetCursorPos(window, &x, &y);
        glm::vec2 mousePos(x, y);
        glm::vec2 mouseDelta = mousePos - lastMousePos;
        lastMousePos = mousePos;


        float sensitivity = 0.0005;
        yaw -= mouseDelta.x * sensitivity;
        pitch -= mouseDelta.y * sensitivity;


        float lim = 0.99f * (float) M_PI_2;
        if (pitch > lim) pitch = lim;
        if (pitch < -lim) pitch = -lim;


        glm::vec3 direction(
                sin(yaw) * cos(pitch),
                sin(pitch),
                cos(yaw) * cos(pitch)
        );

        //std::cout << direction.x << " " << direction.y << " " << direction.z << std::endl;


        glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));

        float speed = deltaTime * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) ? 100.0f : 10.0f);

        if (glfwGetKey(window, GLFW_KEY_A)) {
            eye -= right * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_D)) {
            eye += right * speed;
        }

        if (glfwGetKey(window, GLFW_KEY_W)) {
            eye += direction * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            eye -= direction * speed;
        }

        if (glfwGetKey(window, GLFW_KEY_E)) {
            eye.y += speed;
        }
        if (glfwGetKey(window, GLFW_KEY_Q)) {
            eye.y -= speed;
        }


        // Render

        // Calculate projection
        glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(width) / float(height), 0.01f, 100.0f);
        glm::mat4 view = glm::lookAt(eye, eye + direction, glm::vec3(0, 1, 0));

        glm::mat4 inverse_matrix = glm::inverse(projection * view);


        // Calculate light
//        glm::vec3 lightDirection = glm::normalize(glm::vec3(30.0 * sin(time), 50.0, 30.0 * cos(time)));
        glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.5, 1.0, -0.75));


        // Upload arguments
        error = clSetKernelArg(kernel, 1, sizeof(inverse_matrix), &inverse_matrix);
        checkCLError(error);

        error = clSetKernelArg(kernel, 2, 4 * sizeof(float), &eye);
        checkCLError(error);

        error = clSetKernelArg(kernel, 3, sizeof(float), &time);
        checkCLError(error);

        error = clSetKernelArg(kernel, 4, 4 * sizeof(float), &lightDirection);
        checkCLError(error);


        // Execute the kernel
        glFlush();
        error = clEnqueueAcquireGLObjects(queue, 1, &image, 0, nullptr, nullptr);
        checkCLError(error);

        const size_t global_work_size[] = {width, height, 0};
        error = clEnqueueNDRangeKernel(queue, kernel, 2, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
        checkCLError(error);

        error = clEnqueueReleaseGLObjects(queue, 1, &image, 0, nullptr, nullptr);
        checkCLError(error);

        error = clFinish(queue);
        checkCLError(error);


        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);


        glfwSwapBuffers(window);
    }



    // Release all OpenCL objects
    clReleaseMemObject(image);

    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);

    clReleaseContext(context);
    clReleaseDevice(device);

    glfwTerminate();
}


