#include <math.h>
#include "GLES3Helper.h"

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define TEX_COORD_ATTRIB 1
#define TEX_UNIT_ATTRIB 2
#define Y_ATTRIB 3
#define U_ATTRIB 4
#define V_ATTRIB 5

// position X, Y, texture S, T
static const float rect[] = {-1.0f, -1.0f, 0.0f, 0.0f,
                             -1.0f, 1.0f, 0.0f, 1.0f,
                             1.0f, -1.0f, 1.0f, 0.0f,
                             1.0f, 1.0f, 1.0f, 1.0f};


static const char VERTEX_SHADER[] =
        "#version 300 es\n"
        "layout (location = " STRV(POS_ATTRIB) ") in vec4 aPos;\n"
        "layout (location = " STRV(TEX_COORD_ATTRIB) ") in vec2 aTexCoord;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "   gl_Position = aPos;\n"
        "   vec2 newTexCoord = aTexCoord;\n"           // Flip the image because coordinates system
        "   newTexCoord.y = 1.0 - newTexCoord.y;\n"     // starts from the bottom instead of the top
        "   vTexCoord = newTexCoord;\n"
        //        "   vTexCoord = aTexCoord;\n"
        "}\n";

static const char FRAGMENT_SHADER[] =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 vTexCoord;\n"
        "layout (location = " STRV(TEX_UNIT_ATTRIB) ") uniform sampler2D uTexUnit;\n"
        "out vec4 colorOut;\n"
        "void main() {\n"
        "   colorOut = texture(uTexUnit, vTexCoord);\n"
        "}\n";
//        "layout (location = " STRV(Y_ATTRIB) " ) uniform sampler2D samplerY;\n"
//        "layout (location = " STRV(U_ATTRIB) " ) uniform sampler2D samplerU;\n"
//        "layout (location = " STRV(V_ATTRIB) " ) uniform sampler2D samplerV;\n"
//        "out vec4 colorOut;\n"
//        "void main() {\n"
//        "   vec3 yuv;\n"
//        "   vec3 rgb;\n"
//        "   yuv.r = texture(samplerY, vTexCoord).g;\n"
//        "   yuv.g = texture(samplerU, vTexCoord).g - 0.5;\n"
//        "   yuv.b = texture(samplerV, vTexCoord).g - 0.5;\n"
//        "   rgb = mat3(1.0, 1.0, 1.0,\n"
//        "              0.0, -0.39465, 2.03211,\n"
//        "              1.13983, -0.5806, 0.0) * yuv;\n"
//        "   colorOut = vec4(rgb, 1.0);\n"
//        "}\n";

class RendererES3 {
protected:
    EGLContext mEglContext;
    GLuint mProgram;

    // Buffer handler
    GLuint mBufferId;

    // Texture handler
    GLuint mTextureId = 0;

public:
    RendererES3();

    ~RendererES3();

    bool init();

    void resize(int w, int h);

    void render(int width, int height, GLenum pixFmt, GLint internalPixFmt, const int linesize, const void *pixels);

    void clearSurface();
};

