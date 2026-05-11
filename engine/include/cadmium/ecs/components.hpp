#ifndef CADMIUM_COMPONENTS_HPP
#define CADMIUM_COMPONENTS_HPP

#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Cadmium
{
  struct Transform
  {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f}; // identity quaternion
    glm::vec3 scale{1.f, 1.f, 1.f};

    //  Convenience: position
    float GetX() const { return position.x; }
    float GetY() const { return position.y; }
    float GetZ() const { return position.z; }

    void SetX(float v) { position.x = v; }
    void SetY(float v) { position.y = v; }
    void SetZ(float v) { position.z = v; }

    //  Convenience: scale
    float GetScaleX() const { return scale.x; }
    float GetScaleY() const { return scale.y; }
    float GetScaleZ() const { return scale.z; }

    void SetScaleX(float v) { scale.x = v; }
    void SetScaleY(float v) { scale.y = v; }
    void SetScaleZ(float v) { scale.z = v; }

    //  Convenience: Euler angles in degrees
    // These convert to/from the quaternion on every call.
    // Use them for scripting and editor display, not in hot loops.

    float GetRotationX() const
    {
      return glm::degrees(glm::eulerAngles(rotation).x);
    }
    float GetRotationY() const
    {
      return glm::degrees(glm::eulerAngles(rotation).y);
    }
    float GetRotationZ() const
    {
      return glm::degrees(glm::eulerAngles(rotation).z);
    }

    void SetRotationX(float degrees)
    {
      glm::vec3 euler = glm::eulerAngles(rotation);
      euler.x = glm::radians(degrees);
      rotation = glm::quat(euler);
    }
    void SetRotationY(float degrees)
    {
      glm::vec3 euler = glm::eulerAngles(rotation);
      euler.y = glm::radians(degrees);
      rotation = glm::quat(euler);
    }
    void SetRotationZ(float degrees)
    {
      glm::vec3 euler = glm::eulerAngles(rotation);
      euler.z = glm::radians(degrees);
      rotation = glm::quat(euler);
    }

    //  Convenience: 2D rotation alias
    // Maps to rotationZ - the only rotation axis used in 2D
    float GetRotation() const { return GetRotationZ(); }
    void SetRotation(float d) { SetRotationZ(d); }

    //  Convenience: 2D transform factory
    static Transform From2D(float x, float y, float rotationDegZ = 0.f,
                            float scaleX = 1.f, float scaleY = 1.f) {
        Transform t;
        t.position  = {x, y, 0.f};
        t.rotation  = glm::angleAxis(glm::radians(rotationDegZ), glm::vec3(0,0,1));
        t.scale     = {scaleX, scaleY, 1.f};
        return t;
    }

    //  Quaternion direct access
    const glm::quat &GetQuaternion() const { return rotation; }
    void SetQuaternion(const glm::quat &q) { rotation = glm::normalize(q); }

    void Rotate(const glm::quat &q) { rotation = glm::normalize(q * rotation); }
    void RotateAxis(glm::vec3 axis, float deg)
    {
      rotation = glm::normalize(
          glm::angleAxis(glm::radians(deg), glm::normalize(axis)) * rotation);
    }

    //  Matrix
    // TRS matrix - used by renderer and physics
    glm::mat4 GetMatrix() const
    {
      glm::mat4 t = glm::translate(glm::mat4(1.f), position);
      glm::mat4 r = glm::toMat4(rotation);
      glm::mat4 s = glm::scale(glm::mat4(1.f), scale);
      return t * r * s;
    }

    // Forward, right, up vectors - useful for 3D movement
    glm::vec3 Forward() const
    {
      return rotation * glm::vec3(0.f, 0.f, -1.f);
    }
    glm::vec3 Right() const
    {
      return rotation * glm::vec3(1.f, 0.f, 0.f);
    }
    glm::vec3 Up() const
    {
      return rotation * glm::vec3(0.f, 1.f, 0.f);
    }
  };
  struct Velocity
  {
    float x{0.0f}, y{0.0f};
  };

  struct Tag
  {
    std::string name;
  };

} // namespace Cadmium

#endif // CADMIUM_COMPONENTS_HPP
