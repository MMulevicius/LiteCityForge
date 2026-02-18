#include "Rendering/ShadowMap.h"
#include <iostream>

bool ShadowMap::Init(int size)
{
    mSize = size;

    glGenFramebuffers(1, &mFBO);
    glGenTextures(1, &mDepthTex);

    glBindTexture(GL_TEXTURE_2D, mDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 mSize, mSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = {1.f,1.f,1.f,1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTex, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    bool ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!ok)
        std::cout << "ShadowMap FBO incomplete!\n";

    return ok;
}

void ShadowMap::Shutdown()
{
    if (mDepthTex) glDeleteTextures(1, &mDepthTex);
    if (mFBO) glDeleteFramebuffers(1, &mFBO);
    mDepthTex = 0;
    mFBO = 0;
    mSize = 0;
}

void ShadowMap::BeginDepthPass()
{
    glViewport(0, 0, mSize, mSize);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // DO NOT change cull face here.
    // Your meshes have mixed winding, so GL_FRONT culling creates holes in the shadow map.
}

void ShadowMap::EndDepthPass(int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
}

