#ifndef SAMOS_H
#define SAMOS_H

#include "living.h"
#include "projectile.h"

class Samos : public Living
{
public:
    Samos(double x, double y, int maxHealth, int maxGrenadeCount, int maxMissileCount);
    Samos(double x, double y, int maxHealth, int maxGrenadeCount, int maxMissileCount, CollisionBox* box, QImage* texture, std::string entityType, int health, bool isAffectedByGravity, std::string facing, double frictionFactor, std::string name, bool isMovable);
    ~Samos();

    void shoot(std::string type);
    bool checkWall(CollisionBox* wallBox, Entity* wall);

    bool getIsInAltForm() const;
    void setIsInAltForm(bool newIsInAltForm);

    int getGrenadeCount() const;
    void setGrenadeCount(int newGrenadeCount);

    int getMaxGrenadeCount() const;
    void setMaxGrenadeCount(int newMaxGrenadeCount);

    int getMissileCount() const;
    void setMissileCount(int newMissileCount);

    int getMaxMissileCount() const;
    void setMaxMissileCount(int newMaxMissileCount);

    int getJumpTime() const;
    void setJumpTime(int newJumpTime);

    CollisionBox *getWallBoxR() const;
    void setWallBoxR(CollisionBox *newWallBoxR);

    CollisionBox *getWallBoxL() const;
    void setWallBoxL(CollisionBox *newWallBoxL);

private:
    bool isInAltForm;
    int grenadeCount;
    int maxGrenadeCount;
    int missileCount;
    int maxMissileCount;
    int jumpTime = 20;
    std::string canonDirection = "Right";
    CollisionBox* wallBoxR;
    CollisionBox* wallBoxL;
};

#endif // SAMOS_H
