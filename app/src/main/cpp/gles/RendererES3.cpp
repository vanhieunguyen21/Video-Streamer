#include "RendererES3.h"
#include "../common/JNILogHelper.h"

#define LOG_TAG "RendererES3"

RendererES3::RendererES3() : mEglContext(eglGetCurrentContext()), mProgram(0) {
    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("RendererES3", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);
}

RendererES3::~RendererES3() {
    /* The destructor may be called after the context has already been
     * destroyed, in which case our objects have already been destroyed.
     *
     * If the context exists, it must be current. This only happens when we're
     * cleaning up after a failed init().
     */
    if (eglGetCurrentContext() != mEglContext) return;
    glDeleteProgram(mProgram);

    if (mTextureId != 0) glDeleteTextures(1, &mTextureId);
    if (mBufferId != 0) glDeleteBuffers(1, &mBufferId);
}

bool RendererES3::init() {
    mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!mProgram) return false;

    // Generate texture handler
    glGenTextures(1, &mTextureId);

    // Generate buffer handler
    mBufferId = createVbo(sizeof(rect), rect, GL_STATIC_DRAW);

    LOGV("Using OpenGL ES 3 renderer");
    return true;
}

void RendererES3::resize(int w, int h) {
    glViewport(0, 0, w, h);
}

void RendererES3::render(int width, int height, GLenum pixFmt, GLint internalPixFmt, const int linesize, const void *pixels) {
    if (mTextureId == 0) return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(mProgram);

    // Select texture unit 0 as the active unit to bind texture
    glActiveTexture(GL_TEXTURE0);
    // Set texture draw target
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    // Bind the texture to texture unit 0
    glUniform1i(TEX_UNIT_ATTRIB, 0);
    // Set up texture attributes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Calculate exact each row size
    int rowLength = width;
    switch (pixFmt) {
        case GL_RGB:
            rowLength = linesize / 3;
            break;
        case GL_RGBA:
            rowLength = linesize / 4;
            break;
        default:
            break;
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
    // Upload the texture into texture unit
    glTexImage2D(GL_TEXTURE_2D, 0, internalPixFmt, width, height, 0, pixFmt, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set up texture attributes
    glBindBuffer(GL_ARRAY_BUFFER, mBufferId);
    glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BUFFER_OFFSET(0));
    glVertexAttribPointer(TEX_COORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BUFFER_OFFSET(2 * sizeof(GLfloat)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(POS_ATTRIB);
    glEnableVertexAttribArray(TEX_COORD_ATTRIB);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGlError("RendererES3::render");
}

void RendererES3::clearSurface() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}