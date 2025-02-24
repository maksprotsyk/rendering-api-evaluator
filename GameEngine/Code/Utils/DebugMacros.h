#pragma once

#ifdef _DEBUG

#include <GL/glew.h>

#define ASSERT(condition, message, ...) \
    do { \
        if (!(condition)) { \
            std::cout << "Assertion failed: (" << #condition << "), " \
                      << "file " << __FILE__ << ", line " << __LINE__ << "." << std::endl \
                      << "Message: " << std::format(message, __VA_ARGS__) << std::endl; \
            __debugbreak(); \
        } \
    } while (false)

#define ASSERT_OPENGL(message, ...) \
    do { \
        GLenum error = glGetError(); \
        if (error != GL_NO_ERROR) { \
            std::cout << "OpenGL error check failed: error code=(" << error  << "), " \
                      << "file " << __FILE__ << ", line " << __LINE__ << "." << std::endl \
                      << "Message: " << std::format(message, __VA_ARGS__) << std::endl; \
            __debugbreak(); \
        } \
    } while (false)

#else

#define ASSERT(condition, message, ...) \
    do { } while (false)

#define ASSERT_OPENGL(message, ...) \
    do {} while (false)

#endif
