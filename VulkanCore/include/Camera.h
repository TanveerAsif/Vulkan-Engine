#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>

namespace VulkanCore
{

class Camera
{
  private:
    bool m_bIsAttached;
    float_t m_fTick;

  protected:
    glm::vec3 m_vPos, m_vFrwdDir, m_vUp;

    float m_fSpeed, m_fRotSpeed;
    float m_fRoll, m_fPitch, m_fYaw;

    glm::mat4 m_matView;

    glm::mat4 mProjectionMatrix;

  public:
    Camera(glm::vec3 _vPos, glm::vec3 _vLookAt, glm::vec3 _vUp, float _fFov, float _fAspectRatio, float _fNearPlane,
           float _fFarPlane);
    Camera();
    ~Camera();

    glm::vec3 GetPos();
    glm::vec3 getDir();
    void setPosition(glm::vec3 _vPos);
    void setYaw(float _fYaw);
    void setPitch(float _fPitch);
    void setRoll(float _fRoll);
    void setTick(float _fTick)
    {
        m_fTick = _fTick;
    };
    void setSpeed(float _fSpeed)
    {
        m_fSpeed = _fSpeed;
    };
    void setRotSpeed(float _fRotSpeed)
    {
        m_fRotSpeed = _fRotSpeed;
    };
    glm::vec3 GetLeft();
    glm::vec3 GetRight();
    float getYaw();
    float getPitch();
    float getRoll();
    float getTick()
    {
        return m_fTick;
    };
    float getSpeed()
    {
        return m_fSpeed;
    };
    float getRotSpeed()
    {
        return m_fRotSpeed;
    };
    glm::vec3 getUp()
    {
        return m_vUp;
    };
    glm::vec3 getPosition()
    {
        return m_vPos;
    };
    glm::mat4 getCameraMatrix()
    {
        return m_matView;
    };
    void process();
    glm::mat4 getVPMatrix()
    {
        return mProjectionMatrix * m_matView;
    };

    void update();
    void updateAttachment();
    bool isAttached();
    void setTopView(const float_t duration);

    void handleMouseButton(int button, int action, int mods);
};

} // namespace VulkanCore

#endif
