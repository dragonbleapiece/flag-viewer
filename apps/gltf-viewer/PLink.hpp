#include <glm/glm.hpp>
#include "PPoint.hpp"

class PLink
{
  public:

    // CONSTRUCTORS
    PLink(const PPoint &p1, const PPoint &p2);
    PLink(PPoint *const p1, PPoint *const p2);
    PLink(const std::shared_ptr<PPoint> &p1, const std::shared_ptr<PPoint> &p2);
    PLink(const PLink &plink);
    ~PLink();

    // STATIC ATTRIBUTES
    static float s_k;
    static float s_z;
    static glm::vec3 s_l;

    enum Functor {
      SPRING = 0,
      BRAKE,
      SPRING_BRAKE
    };

    // STATIC METHODS
    static void SpringHook(const PLink &link);
    static void SpringBrake(const PLink &link);
    static void Brake(const PLink &link);
    static void Execute();
    
    // METHODS
    virtual void execute() { SpringBrake(*this); };

  protected:
    const std::shared_ptr<PPoint> m_p1, m_p2;
    float m_k; // k, raideur
    glm::vec3 m_l; // l, longueur à vide 
    float m_z; // z, viscosité;
};