#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

//initializes the default camera
Camera::Camera()
{
    //behind and above origin
    mPos = glm::vec3 (0, 40, 40);
    //looking at origin
    mTarget = glm::vec3(0, 0, 0);
    //Y is set as "up"
    mUp = glm::vec3(0, 1, 0);
    //2D pan speed
    mSpeed = 30.0f;

    mOrthoSize = 50.0f;
}
//keeps value in [0,1]
static float Clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

    
// classic smoothstep
float Camera::SmoothStep(float t)
{
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

//linear interpolation
glm::vec3 Camera::Lerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    return a + (b - a) * t;
}

float Camera::LerpFloat(float a, float b, float t)
{
    return a + (b - a) * t;
}


void Camera::Update(float deltaTime)
{
    // smoothly move towards target pose whenever mBlend < 1
    if (mBlend < 1.0f)
    {
        mBlend = Clamp01(mBlend + deltaTime * mBlendSpeed);
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
    //if (enabled == mTargetIs3D) return;

    glm::vec3 center(cityCenterXZ.x, 0.0f, cityCenterXZ.y);

    mTargetIs3D = enabled;
    mIs3D = enabled;

    
    mBlend = 0.0f;

    if (!enabled)
    {
        // top-down orthographic
        float height = std::max(20.0f, cityRadius * 0.9f);

        mUp = glm::vec3(0, 0, -1);                 
        mPos = glm::vec3(center.x, height, center.z);
        mTarget = glm::vec3(center.x, 0.0f, center.z);

        mOrthoSize = std::max(30.0f, cityRadius + 20.0f);

        mPosTarget = mPos;
        mTargetTarget = mTarget;
        mOrthoSizeTarget = mOrthoSize;
    }
    else
    {
        // 3D view
        float back = std::max(25.0f, cityRadius * 1.2f);
        float up   = std::max(15.0f, cityRadius * 0.35f);

        mUp = glm::vec3(0, 1, 0);
        mTarget = center;
        mPos    = center + glm::vec3(-back * 0.6f, up, back * 0.6f);

        // sync yaw/pitch to match view direction
        glm::vec3 f = glm::normalize(mTarget - mPos);
        mYawDeg   = glm::degrees(atan2f(f.z, f.x));
        mPitchDeg = glm::degrees(asinf(f.y));

        mPosTarget = mPos;
        mTargetTarget = mTarget;
        mOrthoSizeTarget = mOrthoSize;
    }
}



void Camera::UpdatePanXZ(GLFWwindow* window, float deltaTime)
{
    if (mTargetIs3D || mIs3D) return;
    mUp = glm::vec3(0, 0, -1);


    glm::vec3 move(0.0f);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move.z -= 1.0f; // "north"
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move.z += 1.0f; // "south"

    if (glm::length(move) > 0.0f)
    {
        move = glm::normalize(move) * mSpeed * deltaTime;

        // pan camera position
        mPos += move;

        const float groundY = 0.0f;
        mTarget = glm::vec3(mPos.x, groundY, mPos.z);

        mPosTarget = mPos;
        mTargetTarget = mTarget;
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
    glm::vec3 up = mUp;

    if (!mIs3D)
    {
        up = glm::vec3(0, 0, -1);
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

//3D camera movement
glm::vec3 Camera::GetForwardFromAngles() const
{
    float yaw   = glm::radians(mYawDeg);
    float pitch = glm::radians(mPitchDeg);

    glm::vec3 f;
    f.x = cosf(pitch) * cosf(yaw);
    f.y = sinf(pitch);
    f.z = cosf(pitch) * sinf(yaw);
    return glm::normalize(f);
}

void Camera::OnMouseMove(double xpos, double ypos, bool rmbDown)
{
    if (!rmbDown)
    {
        // reset so next RMB press doesn't jump
        mMouseInit = false;
        return;
    }

    if (!mMouseInit)
    {
        mLastMouseX = xpos;
        mLastMouseY = ypos;
        mMouseInit = true;
        return;
    }

    double dx = xpos - mLastMouseX;
    double dy = ypos - mLastMouseY;
    mLastMouseX = xpos;
    mLastMouseY = ypos;

    mYawDeg   += (float)dx * mMouseSens;
    mPitchDeg -= (float)dy * mMouseSens;

    // clamp pitch to avoid flipping
    mPitchDeg = std::max(-89.0f, std::min(89.0f, mPitchDeg));
}

void Camera::UpdateFly3D(GLFWwindow* window, float deltaTime)
{
    if (!mIs3D && !mTargetIs3D) return;

    glm::vec3 forward = GetForwardFromAngles();
    glm::vec3 right   = glm::normalize(glm::cross(forward, mUp));
    glm::vec3 up = glm::vec3(0, 1, 0);

    glm::vec3 move(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= forward;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += right;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= right;

    // vertical movement 
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) move += up;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) move -= up;

    // speed modifiers
    float speed = mFlySpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.5f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 0.35f;

    if (glm::length(move) > 0.0f)
    {
        move = glm::normalize(move) * speed * deltaTime;
        mPos += move;
        mPosTarget += move; 
    }

    // look direction driven by yaw/pitch
    mTarget = mPos + forward * 10.0f;
    mTargetTarget = mTarget;
}


void Camera::ResetViewToCity(const glm::vec2& cityCenterXZ, float cityRadius)
{
    Set3DEnabled(mIs3D, cityCenterXZ, cityRadius);
}


