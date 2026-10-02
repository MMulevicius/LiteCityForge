#include "Textures/TextureUtils.h"
#include <stb_image/stb_image.h>

namespace textures
{

    // loads a 2D texture from disk and creates an OpenGL texture object
    GLuint LoadTexture2D(const std::string &path)
    {
        int w = 0, h = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path.c_str(), &w, &h, &channels, 0);
        if (!data)
            return 0;

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return tex;
    }

    // deletes a texture safely and resets its handle
    void SafeDeleteTexture(GLuint &tex)
    {
        if (tex != 0)
        {
            glDeleteTextures(1, &tex);
            tex = 0;
        }
    }

}