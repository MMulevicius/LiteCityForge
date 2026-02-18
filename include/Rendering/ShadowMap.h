#pragma once
#include <glad/glad.h>

class ShadowMap
{
public:
    bool Init(int size);
    void Shutdown();

    void BeginDepthPass();    // bind FBO + viewport
    void EndDepthPass(int w, int h); // restore default framebuffer + viewport

    GLuint GetDepthTexture() const { return mDepthTex; }
    int GetSize() const { return mSize; }

private:
    GLuint mFBO = 0;
    GLuint mDepthTex = 0;
    int mSize = 0;
};
