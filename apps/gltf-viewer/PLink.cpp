#include "PLink.hpp"

PLink::PLink(const PPoint &p1, const PPoint &p2)
: m_p1(std::make_shared<PPoint>(p1)),
  m_p2(std::make_shared<PPoint>(p2)),
  m_k(0.f),
  m_z(0.f),
  m_l(p2.position() - p1.position())
{

}

PLink::PLink(PPoint *const p1, PPoint *const p2)
: m_p1(p1),
  m_p2(p2),
  m_k(0.f),
  m_z(0.f),
  m_l(p2->position() - p1->position())
{

}

PLink::PLink(const std::shared_ptr<PPoint> &p1, const std::shared_ptr<PPoint> &p2)
: m_p1(p1),
  m_p2(p2),
  m_k(0.f),
  m_z(0.f),
  m_l(p2->position() - p1->position())
{

}

PLink::PLink(const PLink &plink)
: m_p1(plink.m_p1),
  m_p2(plink.m_p2),
  m_k(plink.m_k),
  m_z(plink.m_z),
  m_l(plink.m_l)
{

}

PLink::~PLink() {
  
}

glm::vec3 PLink::s_force(0.);
float PLink::s_k(0.);
float PLink::s_z(0.);
glm::vec3 PLink::s_l(0.);

void PLink::SpringHook(const PLink &link) {
  glm::vec3 d = link.m_p2->position() - link.m_p1->position();
  glm::vec3 f = (link.m_k + s_k) * (d - link.m_l - s_l); // raideur * allongement
  // distrib
  link.m_p1->applyForce(f + s_force);
  link.m_p2->applyForce(-f + s_force);
}

void PLink::SpringBrake(const PLink &link) {
  glm::vec3 d = link.m_p2->position() - link.m_p1->position();
  glm::vec3 s = link.m_p2->speed() - link.m_p1->speed();
  glm::vec3 f = (link.m_k + s_k) * (d - link.m_l - s_l) + (link.m_z + s_z) * s;
  link.m_p1->applyForce(f + s_force);
  link.m_p2->applyForce(-f + s_force);
}

void PLink::Brake(const PLink &link) {
  glm::vec3 s = link.m_p2->speed() - link.m_p1->speed();
  glm::vec3 f = (link.m_z + s_z) * s;
  // distrib
  link.m_p1->applyForce(f + s_force);
  link.m_p2->applyForce(-f + s_force);
}