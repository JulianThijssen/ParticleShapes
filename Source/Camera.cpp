#include "Camera.h"

#include <GDT/Maths.h>

Camera::Camera() :
    pos(0, 0, 0),
    rot(0, 0, 0),
    _fieldOfView(60),
    _aspectRatio(1.0f),
    _zNear(0.1f),
    _zFar(100.0f)
{

}

void Camera::loadProjectionMatrix(Matrix4f& projMatrix)
{
    float fieldOfViewRadians = _fieldOfView * (Math::PI / 180);
    projMatrix.setIdentity();
    projMatrix[0] = (1.0f / tan(fieldOfViewRadians / 2)) / _aspectRatio;
    projMatrix[5] = (1.0f / tan(fieldOfViewRadians / 2));
    projMatrix[10] = (_zNear + _zFar) / (_zNear - _zFar);
    projMatrix[11] = -1;
    projMatrix[14] = (2 * _zNear * _zFar) / (_zNear - _zFar);
    projMatrix[15] = 0;
}

void Camera::loadViewMatrix(Matrix4f& viewMatrix)
{
    viewMatrix.setIdentity();
    viewMatrix.rotate(rot);
    viewMatrix.translate(pos);
}
