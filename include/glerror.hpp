#pragma once

#include "glad/glad.h"
#include "log.hpp"

#define _GL_MAX_ERROR 1024

inline void GLClearError() {
    static unsigned int count = 0;
    count = 0;
    GLenum err = glGetError();
    while (err != GL_NO_ERROR && count < _GL_MAX_ERROR) {
        err = glGetError();
        count ++;
    }
}

inline const char* GLError2Str(GLenum error) {
    #define CASE(x) case x: return #x;
    switch (error) {
        CASE(GL_INVALID_ENUM)
        CASE(GL_INVALID_VALUE)
        CASE(GL_INVALID_OPERATION)
        CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
        CASE(GL_OUT_OF_MEMORY)
#ifdef GL_STACK_UNDERFLOW
        CASE(GL_STACK_UNDERFLOW)
#endif

#ifdef GL_STACK_OVERFLOW
        CASE(GL_STACK_OVERFLOW)
#endif
        default: return "GL_UNKNOWN_ERROR";
    } 
    #undef CASE
}

#define GL_CALL(expr) do { \
    GLClearError(); \
    expr; \
    GLenum e = glGetError(); \
    if (e != GL_NO_ERROR) { \
        LOGE("[GL]: ", GLError2Str(e)); \
    } \
} while(0)