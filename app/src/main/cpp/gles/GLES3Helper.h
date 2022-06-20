#include "NDKHelper.h"
#include "EGL/egl.h"
#include "assert.h"
#include "cstring"
#include "cstdlib"

#define BUFFER_OFFSET(i) ((void*)(i))

GLuint createVbo(const GLsizeiptr size, const GLvoid *data, const GLenum usage);

bool checkGlError(const char *funcName);

void printGlString(const char *name, GLenum s);

GLuint createShader(GLenum shaderType, const char *src);

GLuint createProgram(const char *vtxSrc, const char *fragSrc);

GLuint loadTexture(const GLsizei width, const GLsizei height, const GLenum type, const GLvoid *pixels);

