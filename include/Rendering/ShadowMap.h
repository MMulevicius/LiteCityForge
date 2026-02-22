#pragma once
#include <glad/glad.h>

class ShadowMap
{
public:
    bool Init(int size);
    void Shutdown();

    // bind FBO + viewport
    void BeginDepthPass();    
    // restore default framebuffer + viewport
    void EndDepthPass(int w, int h); 

    GLuint GetDepthTexture() const { return mDepthTex; }
    int GetSize() const { return mSize; }

private:
    GLuint mFBO = 0;
    GLuint mDepthTex = 0;
    int mSize = 0;
};
