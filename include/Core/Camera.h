#pragma once 
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera
{
private:

    // core pose
    glm::vec3 mPos; // camera position in world space
    glm::vec3 mTarget; // point camera looks at
    glm::vec3 mUp; // up direction for "glm::lookAt"

    // 2D behaviour
    float mSpeed; // pan speed in 2D mode
    float mOrthoSize; // "zoom level" for orthographic projection (half-height of view volume)
    float mFovDeg = 55.0f; // perspective FOV

    //mode and mouse state
    bool mIs3D = false; //what mode the camera is currently using
    
    // for making sure that yaw/pitch doesn't jump when RMB is first pressed
    bool mMouseInit = false; 
    double mLastMouseX = 0.0;//prevents the "first RMB frame jump"
    double mLastMouseY = 0.0;

    //3D look and motion parameters
    float mYawDeg = -45.0f; // camera rotation (in degrees)
    float mPitchDeg = -20.0f; 
    float mMouseSens = 0.12f; // mouse sensitivity 
    float mFlySpeed = 60.0f; // movement speed in 3D

    glm::vec3 GetForwardFromAngles() const;

    
    

public:

    // sets the default pose (mPos, mTarget, etc)
    Camera();

    //camera position reset
    void ResetViewToCity(const glm::vec2& cityCenterXZ, float cityRadius); // resets position in current mode


    //detectors
    void OnMouseMove(double xpos, double ypos, bool rmbDown); // updates angle when RMB is down 

    //updaters
    void UpdatePanXZ(GLFWwindow *window, float deltaTime); //only runs in 2D mode, reads WASD, movement.
    void UpdateFly3D(GLFWwindow* window, float deltaTime); // runs in 3D only, moves with WASD, speed modifiers: SHIFT, CTRL.

    //getters
    glm::mat4 GetVP(float aspect) const; // creates a view matrix 
    float GetOrthoSize() const { return mOrthoSize;}
    const glm::vec3& GetPos() const {return mPos;}
    const glm::vec3& GetTarget() const { return mTarget;}
    float GetFlySpeed() const { return mFlySpeed; }

    //setters
    void Set3DEnabled(bool enabled, const glm::vec2& cityCenterXZ, float cityRadius); //enabled == false (2D), enabled == true (3D)
    void SetOrthoSize(float size) { mOrthoSize = size;}
    void SetSpeed(float speed) {mSpeed = speed;}
    void SetFlySpeed(float s) { mFlySpeed = s;}
    void ApplyScrollZoom(float scrollDelta); // only runs in 2D, multiplies mOrthoSize.
};