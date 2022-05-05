#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "entity.h"

class Projectile : public Entity
{
public:
    static const int unknownProjectileType = -3;
    //enum ProjectileType {Beam, Missile, Grenade, Bomb};
    Projectile(double x, double y, std::string facing, std::string type, std::string name);

    int getDamage() const;
    void setDamage(int newDamage);

    int getLifeTime() const;
    void setLifeTime(int newLifeTime);

    const std::string &getProjectileType() const;
    void setProjectileType(const std::string &newProjectileType);

private:
    int damage;
    int lifeTime; // in ms, starts with a positive value, destroys the object when null or negative
    std::string projectileType;
};

#endif // PROJECTILE_H
