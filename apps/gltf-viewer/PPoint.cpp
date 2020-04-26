#include "PPoint.hpp"

PPoint::PPoint(const glm::vec3 &xyz, const float mass)
: m_position(xyz),
  m_speed(0, 0, 0),
  m_force(0, 0, 0),
  m_mass(mass)
{

}

PPoint::PPoint(const float x, const float y, const float z, const float mass)
: PPoint(glm::vec3(x, y, z), mass)
{
  // Void
}

PPoint::PPoint(const PPoint &ppoint)
: PPoint(ppoint.m_position, ppoint.m_mass)
{

}

PPoint::~PPoint() {

}

PFixedPoint::PFixedPoint(const glm::vec3 &xyz, const float mass)
: PPoint(xyz, mass)
{

}

PFixedPoint::PFixedPoint(const float x, const float y, const float z, const float mass)
: PFixedPoint(glm::vec3(x, y, z), mass)
{

}