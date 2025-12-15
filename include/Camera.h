#pragma once 
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera
{
private:
    glm::vec3 mPos;
    glm::vec3 mTarget;
    glm::vec3 mUp;

    float mSpeed;
    float mOrthoSize;

public:
    Camera();

    void UpdatePanXZ(GLFWwindow *window, float dt);
    glm::mat4 GetVPOrtho(float aspect) const;

    void SetOrthoSize(float size) { mOrthoSize = size;}
    void SetSpeed(float speed) {mSpeed = speed;}

};