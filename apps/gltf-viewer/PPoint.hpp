#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>

class PPoint
{
  public:
    // CONSTRUCTORS
    PPoint(const glm::vec3 &xyz, const float mass);
    PPoint(const float x, const float y, const float z, const float mass);
    PPoint(const PPoint& ppoint);
    ~PPoint();

    // STATIC METHODS
    static void Execute(const float h);
    
    // GETTERS
    inline const glm::vec3 position() const { return m_position; };
    inline const glm::vec3 speed() const { return m_speed; };
    inline const glm::vec3 force() const { return m_force; };
    inline const float mass() const { return m_mass; };

    // METHODS
    virtual inline void applyForce(const glm::vec3 &force) { m_force += force; };
    virtual void execute(const float h) {
      LeapFrog(h);
    }
    inline void clearForce() { m_force = glm::vec3(0.); }

    void LeapFrog(const float h) {
      m_speed += h * m_force / m_mass;
      m_position += h * m_speed;
    }

    void EulerExplicit(const float h) {
      m_position += h * m_speed;
      m_speed += h * m_force / m_mass;
    }

  protected:
    glm::vec3 m_position, m_speed, m_force, m_normal;
    float m_mass;
};

class PFixedPoint : public PPoint
{
  public:
    // CONSTRUCTORS
    PFixedPoint(const glm::vec3 &xyz, const float mass);
    PFixedPoint(const float x, const float y, const float z, const float mass);

    // METHODS
    inline void applyForce(const glm::vec3 &force) override {};
    inline void execute(const float h) override {};

};