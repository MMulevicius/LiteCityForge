#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
    mPos = glm::vec3 (0, 40, 40);
    mTarget = glm::vec3(0, 0, 0);
    mUp = glm::vec3(0, 1, 0);
    mSpeed = 30.0f;
    mOrthoSize = 50.0f;
}

void Camera::UpdatePanXZ(GLFWwindow *window, float dt)
{
    glm::vec3 forward = glm::normalize(mTarget - mPos);
    glm::vec3 right = glm::normalize(glm::cross(forward, mUp));

    forward.y = 0.0f;
    right.y = 0.0f;
    if (glm::length(forward) > 0.0f) forward = glm::normalize(forward);
    if (glm::length(right) > 0.0f) right = glm::normalize(right);

    glm::vec3 move(0.0f);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += right;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= forward;

    if (glm::length(move) > 0.0f)
    {
        move = glm::normalize(move) * mSpeed * dt;
        mPos += move;
        mTarget += move;
    }
}

glm::mat4 Camera::GetVPOrtho(float aspect) const
{
    glm::mat4 proj = glm::ortho(
        -mOrthoSize * aspect, mOrthoSize * aspect,
        -mOrthoSize, mOrthoSize,
        -100.0f, 100.0f
    );

    glm::mat4 view = glm::lookAt(mPos, mTarget, mUp);
    return proj * view;
}