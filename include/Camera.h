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

    glm::vec3 mPosTarget;
    glm::vec3 mTargetTarget;
    float mOrthoSizeTarget;

    bool mIs3D = false;
    bool mTargetIs3D = false;
    float mFovDeg = 55.0f;

    float mBlend = 1.0f;
    float mBlendSpeed = 3.0f;

    static float SmoothStep(float t);
    static glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float t);
    static float LerpFloat(float a, float b, float t);

public:
    Camera();

    void Update(float dt);
    void UpdateZoomOrtho(GLFWwindow* window, float dt);
    void UpdatePanXZ(GLFWwindow *window, float dt);
    glm::mat4 GetVP(float aspect) const;
    float GetOrthoSize() const { return mOrthoSize;}
    const glm::vec3& GetPos() const {return mPos;}
    const glm::vec3& GetTarget() const { return mTarget;}


    void Set3DEnabled(bool enabled, const glm::vec2& cityCenterXZ, float cityRadius);
    void SetOrthoSize(float size) { mOrthoSize = size;}
    void SetSpeed(float speed) {mSpeed = speed;}
    void ApplyScrollZoom(float scrollDelta);
};