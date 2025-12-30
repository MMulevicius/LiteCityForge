#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera()
{
    mPos = glm::vec3 (0, 40, 40);
    mTarget = glm::vec3(0, 0, 0);
    mUp = glm::vec3(0, 1, 0);
    mSpeed = 30.0f;
    mOrthoSize = 50.0f;
}

static float Clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

float Camera::SmoothStep(float t)
{
    // classic smoothstep
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}


glm::vec3 Camera::Lerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    return a + (b - a) * t;
}

float Camera::LerpFloat(float a, float b, float t)
{
    return a + (b - a) * t;
}


void Camera::Update(float dt)
{
    // smoothly move towards target pose whenever mBlend < 1
    if (mBlend < 1.0f)
    {
        mBlend = Clamp01(mBlend + dt * mBlendSpeed);
        float t = SmoothStep(mBlend);

        mPos      = Lerp(mPos, mPosTarget, t);
        mTarget   = Lerp(mTarget, mTargetTarget, t);
        mOrthoSize= LerpFloat(mOrthoSize, mOrthoSizeTarget, t);

        // lock to target and finalize mode
        if (mBlend >= 1.0f)
        {
            mPos = mPosTarget;
            mTarget = mTargetTarget;
            mOrthoSize = mOrthoSizeTarget;
            mIs3D = mTargetIs3D;
        }
    }
}


void Camera::Set3DEnabled(bool enabled, const glm::vec2& cityCenterXZ, float cityRadius)
{
    // if already heading to that mode, ignore
    if (enabled == mTargetIs3D) return;

    mTargetIs3D = enabled;
    mBlend = 0.0f; 

    // city center in your world: x,z plane with y up
    glm::vec3 center(cityCenterXZ.x, 0.0f, cityCenterXZ.y);

    if (!enabled)
    {

        float h = std::max(20.0f, cityRadius * 0.9f);
        mTargetTarget = center;
        mPosTarget    = center + glm::vec3(0.0f, h, h); 

        mOrthoSizeTarget = std::max(30.0f, cityRadius + 20.0f);
    }
    else
    {

        float back = std::max(25.0f, cityRadius * 1.2f);
        float up   = std::max(15.0f, cityRadius * 0.35f);

        mTargetTarget = center;

        mPosTarget = center + glm::vec3(-back * 0.6f, up, back * 0.6f);

        mOrthoSizeTarget = mOrthoSize;
    }
}

void Camera::UpdatePanXZ(GLFWwindow *window, float dt)
{
    if (mTargetIs3D || mIs3D) return;

    glm::vec3 forward = glm::normalize(mTarget - mPos);
    glm::vec3 right   = glm::normalize(glm::cross(forward, mUp));

    forward.y = 0.0f;
    right.y   = 0.0f;

    if (glm::length(forward) > 0.0f) forward = glm::normalize(forward);
    if (glm::length(right) > 0.0f)   right   = glm::normalize(right);

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
        mPosTarget += move;
        mTargetTarget += move;
    }
}


void Camera::ApplyScrollZoom(float scrollDelta)
{
    if (scrollDelta == 0.0f) return;

    if (mTargetIs3D || mIs3D) return;
    // sensitivity
    const float zoomSpeed = 0.12f; 
    const float minSize   = 2.0f;
    const float maxSize   = 500.0f;

    mOrthoSize *= (1.0f - scrollDelta * zoomSpeed);
    mOrthoSize = std::max(minSize, std::min(maxSize, mOrthoSize));

    mOrthoSizeTarget = mOrthoSize;
}

glm::mat4 Camera::GetVP(float aspect) const
{
   glm::mat4 view = glm::lookAt(mPos, mTarget, mUp);

    if (!mIs3D)
    {
        float halfH = mOrthoSize;
        float halfW = mOrthoSize * aspect;
        glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, -2000.0f, 2000.0f);
        return proj * view;
    }
    else
    {
        // perspective for 3D
        float nearZ = 0.1f;
        float farZ  = 5000.0f;
        glm::mat4 proj = glm::perspective(glm::radians(mFovDeg), aspect, nearZ, farZ);
        return proj * view;
    }
}
