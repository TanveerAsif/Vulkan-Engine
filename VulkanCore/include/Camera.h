#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>

namespace VulkanCore {

class Camera {
private:
  bool m_bIsAttached;

protected:
  glm::vec3 m_vPos, m_vFrwdDir, m_vUp;

  float m_fSpeed, m_fRotSpeed;
  float m_fRoll, m_fPitch, m_fYaw;

  glm::mat4 m_matView;

  glm::vec3 GetLeft();
  glm::vec3 GetRight();

  glm::mat4 mProjectionMatrix;

public:
  Camera(glm::vec3 _vPos, glm::vec3 _vLookAt, glm::vec3 _vUp, float _fFov,
         float _fAspectRatio, float _fNearPlane, float _fFarPlane);
  Camera();
  ~Camera();

  glm::vec3 GetPos();
  glm::vec3 getDir();
  void SetPos(glm::vec3 _vPos);
  void setYaw(float _fYaw);
  void setPitch(float _fPitch);
  void setRoll(float _fRoll);
  float getYaw();
  float getPitch();
  glm::mat4 getCameraMatrix() { return m_matView; };
  void process(GLFWwindow *_pWindow, double _fTick);
  glm::mat4 getVPMatrix() { return mProjectionMatrix * m_matView; };

  void update();
  void updateAttachment();
  bool isAttached();
  void setTopView(const float_t duration);
};

} // namespace VulkanCore

#endif
