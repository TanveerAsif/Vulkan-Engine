#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VulkanCore {

glm::vec3 Camera::GetLeft() {
  glm::vec3 left = glm::cross(m_vFrwdDir, m_vUp);
  left = glm::normalize(left);
  return left;
}

glm::vec3 Camera::GetRight() {
  glm::vec3 right = glm::cross(m_vUp, m_vFrwdDir);
  right = glm::normalize(right);
  return right;
}

Camera::Camera()
    : m_bIsAttached{false}, m_fTick(0.01f), m_vPos(0.0f, 2.0f, -40.0f),
      m_vFrwdDir(0, 0, -1), m_vUp(0, 1, 0), m_fSpeed(10.0f), m_fRotSpeed(1.0f),
      m_fRoll(0.0f), m_fPitch(0.0f), m_fYaw(0.0f), m_matView(glm::mat4(1.0f)) {

  this->process();
}

Camera::Camera(glm::vec3 _vPos, glm::vec3 _vLookAt, glm::vec3 _vUp, float _fFov,
               float _fAspectRatio, float _fNearPlane, float _fFarPlane)
    : m_bIsAttached{false},
      m_fTick(0.01f), m_vPos{_vPos}, m_vUp{_vUp}, m_fSpeed{10.0f},
      m_fRotSpeed{1.0f}, m_fRoll{0.0f}, m_fPitch{0.0f}, m_fYaw{0.0f},
      m_matView{glm::mat4(1.0f)} {
  this->process();
  mProjectionMatrix = glm::perspective(glm::radians(_fFov), _fAspectRatio,
                                       _fNearPlane, _fFarPlane);
}

Camera::~Camera() {}

glm::vec3 Camera::GetPos() { return m_vPos; }

void Camera::setPosition(glm::vec3 _vPos) { m_vPos = _vPos; }

void Camera::setYaw(float _fYaw) { m_fYaw = _fYaw; }

void Camera::setPitch(float _fPitch) { m_fPitch = _fPitch; }

void Camera::process() {

  glm::vec3 vDir;
  vDir.x = sin(glm::radians(m_fYaw)) * cos(glm::radians(m_fPitch));
  vDir.y = sin(glm::radians(m_fPitch));
  vDir.z = cos(glm::radians(m_fYaw)) * cos(glm::radians(m_fPitch));

  // To move camera on x-z plane only
  // glm::vec2 vFrwdDir = glm::vec2(vDir.x, vDir.z);
  // vFrwdDir = glm::normalize(vFrwdDir);
  // m_vFrwdDir = glm::vec3(vFrwdDir.x, 0.0f, vFrwdDir.y);
  m_vFrwdDir = glm::normalize(vDir);

  glm::vec3 vTarget = m_vPos + m_vFrwdDir;
  m_matView = glm::lookAt(m_vPos, vTarget, m_vUp);
}

float Camera::getYaw() { return m_fYaw; }

float Camera::getPitch() { return m_fPitch; }

glm::vec3 Camera::getDir() { return m_vFrwdDir; }

void Camera::updateAttachment() {
  m_bIsAttached = m_bIsAttached ? false : true;
}

bool Camera::isAttached() { return m_bIsAttached; }

void Camera::setRoll(float _fRoll) { m_fRoll = _fRoll; }

void Camera::setTopView(const float_t duration) {
  this->setPosition(glm::vec3(22.0F, 157.0F, -58.0F));
  this->setPitch(-81.0F);
  this->setYaw(-5.0F);
  this->setRoll(0.0F);
  this->setTick(duration);
  this->process();
}
} // namespace VulkanCore
