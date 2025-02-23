#pragma once

#ifdef _DEBUG

#define ASSERT(condition, message, ...) \
    do { \
        if (!(condition)) { \
            std::cout << "Assertion failed: (" << #condition << "), " \
                      << "file " << __FILE__ << ", line " << __LINE__ << "." << std::endl \
                      << "Message: " << std::format(message, __VA_ARGS__) << std::endl; \
            __debugbreak(); \
        } \
    } while (false)

#else

#define ASSERT(condition, message, ...) \
    do { } while (false)

#endif
