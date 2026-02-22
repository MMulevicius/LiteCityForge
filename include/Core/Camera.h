#pragma once 
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera
{
private:

    // core pose
    glm::vec3 mPos; 
    glm::vec3 mTarget; 
    glm::vec3 mUp; 

    // 2D behaviour
    float mSpeed; 
    float mOrthoSize; 
    float mFovDeg = 55.0f; 

    //mode and mouse state
    bool mIs3D = false; 
    
    // for making sure that yaw/pitch doesn't jump when RMB is first pressed
    bool mMouseInit = false; 
    double mLastMouseX = 0.0;
    double mLastMouseY = 0.0;

    //3D look and motion parameters
    float mYawDeg = -45.0f; 
    float mPitchDeg = -20.0f; 
    float mMouseSens = 0.12f; 
    float mFlySpeed = 60.0f; 
    glm::vec3 GetForwardFromAngles() const;

    
    

public:

    // sets the default pose (mPos, mTarget, etc)
    Camera();

    //camera position reset
    void ResetViewToCity(const glm::vec2& cityCenterXZ, float cityRadius); // resets position in current mode


    //detectors
    void OnMouseMove(double xpos, double ypos, bool rmbDown);  

    //updaters
    void UpdatePanXZ(GLFWwindow *window, float deltaTime); 
    void UpdateFly3D(GLFWwindow* window, float deltaTime); 

    //getters
    glm::mat4 GetVP(float aspect) const; // creates a view matrix 
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