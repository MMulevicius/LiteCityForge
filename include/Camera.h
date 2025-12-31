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
    
    //for 3D camera
    bool mIs3D = false;
    bool mTargetIs3D = false;
    bool mMouseInit = false;

    double mLastMouseX = 0.0;
    double mLastMouseY = 0.0;

    float mFovDeg = 55.0f;
    float mBlend = 1.0f;
    float mBlendSpeed = 3.0f;
    float mYawDeg = -45.0f;
    float mPitchDeg = -20.0f;
    float mMouseSens = 0.12f;
    float mFlySpeed = 60.0f;

    static float SmoothStep(float t);
    static glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float t);
    static float LerpFloat(float a, float b, float t);
    glm::vec3 GetForwardFromAngles() const;

    
    

public:
    Camera();

    //camera position reset
    void ResetViewToCity(const glm::vec2& cityCenterXZ, float cityRadius);


    //detectors
    void OnMouseMove(double xpos, double ypos, bool rmbDown);

    //updaters
    void Update(float dt);
    void UpdateZoomOrtho(GLFWwindow* window, float dt);
    void UpdatePanXZ(GLFWwindow *window, float dt);
    void UpdateFly3D(GLFWwindow* window, float dt);

    //getters
    glm::mat4 GetVP(float aspect) const;
    float GetOrthoSize() const { return mOrthoSize;}
    const glm::vec3& GetPos() const {return mPos;}
    const glm::vec3& GetTarget() const { return mTarget;}
    float GetFlySpeed() const { return mFlySpeed; }

    //setters
    void Set3DEnabled(bool enabled, const glm::vec2& cityCenterXZ, float cityRadius);
    void SetOrthoSize(float size) { mOrthoSize = size;}
    void SetSpeed(float speed) {mSpeed = speed;}
    void SetFlySpeed(float s) { mFlySpeed = s;}
    void ApplyScrollZoom(float scrollDelta);
};