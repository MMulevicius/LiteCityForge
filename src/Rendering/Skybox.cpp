#include "Rendering/Skybox.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stb_image/stb_image.h>
#include "Core/AssetPaths.h"

// unit cube vertices used to render the skybox (36 verts, 12 triangles)
static const float SKYBOX_VERTS[] = {
    // positions
    -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

    1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

// loads the cubemap, creates cube VAO/VBO, and prepares the skybox shader
bool Skybox::Initialize(const std::string &directory)
{
    // faces:
    // px.png nx.png py.png ny.png pz.png nz.png

    std::vector<std::string> faces = {
        directory + "/px.png",
        directory + "/nx.png",
        directory + "/py.png",
        directory + "/ny.png",
        directory + "/pz.png",
        directory + "/nz.png"};

    mCubemapTex = LoadCubemap(faces);
    if (!mCubemapTex)
    {
        std::cout << "[Skybox] Failed to load cubemap.\n";
        return false;
    }

    // upload static cube mesh to GPU
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTS), SKYBOX_VERTS, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindVertexArray(0);

    // skybox shader uses a cubemap sampler
    mShader = new Shader(
        assets::Path("shaders/skybox.vert"),
        assets::Path("shaders/skybox.frag"));

    // set sampler once
    mShader->use();
    glUniform1i(glGetUniformLocation(mShader->ID, "skybox"), 0);

    std::cout << "[Skybox] Initialized OK.\n";
    return true;
}

// frees GPU buffers/textures and deletes the skybox shader instance
void Skybox::Shutdown()
{
    if (mVBO)
        glDeleteBuffers(1, &mVBO);
    if (mVAO)
        glDeleteVertexArrays(1, &mVAO);
    if (mCubemapTex)
        glDeleteTextures(1, &mCubemapTex);

    mVBO = mVAO = mCubemapTex = 0;

    delete mShader;
    mShader = nullptr;
}

// loads 6 images and uploads them into a GL_TEXTURE_CUBE_MAP texture
GLuint Skybox::LoadCubemap(const std::vector<std::string> &faces)
{
    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    // do not flip cubemap faces
    stbi_set_flip_vertically_on_load(false);

    int width, height, channels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);
        if (!data)
        {
            std::cout << "[Skybox] Failed to load face: " << faces[i] << "\n";
            stbi_image_free(data);
            glDeleteTextures(1, &texID);
            return 0;
        }

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        // upload this face into the correct cubemap slot
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return texID;
}
// renders the skybox as the scene background
void Skybox::Draw(const Camera &camera, float aspect, bool is3D)
{
    if (!mShader || mCubemapTex == 0 || mVAO == 0)
        return;

    glm::mat4 view(1.0f);

    if (is3D)
    {
        // normal 3D: follow the camera orientation
        view = glm::lookAt(camera.GetPos(), camera.GetTarget(), glm::vec3(0, 1, 0));
        // remove translation from the view matrix so the skybox stays centered on the camera
        view = glm::mat4(glm::mat3(view));
    }
    else
    {

        glm::vec3 eye(0.0f, 0.0f, 0.0f);
        glm::vec3 dir = glm::normalize(glm::vec3(0.2f, 0.25f, -1.0f)); // slight up tilt
        // in 2d mode use a fixed view direction to keep the background stable
        view = glm::lookAt(eye, eye + dir, glm::vec3(0, 1, 0));
        view = glm::mat4(glm::mat3(view));
    }

    glm::mat4 proj = glm::perspective(glm::radians(55.0f), aspect, 0.1f, 5000.0f);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    mShader->use();
    glUniformMatrix4fv(glGetUniformLocation(mShader->ID, "uView"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mShader->ID, "uProj"), 1, GL_FALSE, &proj[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTex);

    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
