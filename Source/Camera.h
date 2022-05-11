#pragma once

#include <GDT/Matrix4f.h>
#include <GDT/Vector3f.h>

class Camera
{
public:
    Camera();

    void loadProjectionMatrix(Matrix4f& projMatrix);
    void loadViewMatrix(Matrix4f& viewMatrix);

    Vector3f pos;
    Vector3f rot;

private:
    float _fieldOfView;
    float _aspectRatio;
    float _zNear;
    float _zFar;
};
