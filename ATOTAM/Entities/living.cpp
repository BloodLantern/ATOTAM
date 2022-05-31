#include "living.h"

Living::Living(double x, double y, CollisionBox* box, QImage* texture, std::string entityType, int health, int maxHealth, bool isAffectedByGravity, std::string facing, double frictionFactor, std::string name, bool isMovable)
    : Entity(x, y, box, texture, entityType, isAffectedByGravity, facing, frictionFactor, name, isMovable),
      health(health), maxHealth(maxHealth), groundBox(new CollisionBox(box->getX(), box->getY() + box->getHeight(), box->getWidth(), 2)), onGround(false)
{

}

Living::Living(double x, double y, std::string facing, std::string name)
    : Entity(x, y, facing, name)
{
    //fast constructor using the json file
    nlohmann::json livJson = values["livings"][name];
    health = livJson["maxHealth"];
    maxHealth = livJson["maxHealth"];
    damage = livJson["damage"];
    invulnerable = livJson["invulnerable"];
    groundBox = new CollisionBox(this->getBox()->getX(), this->getBox()->getY() + this->getBox()->getHeight(), this->getBox()->getWidth(), 2);
}

Living::~Living()
{
    if (groundBox != nullptr)
        delete groundBox;
}

void Living::hit()
{

}

int Living::getHealth() const
{
    return health;
}

void Living::setHealth(int newHealth)
{
    health = newHealth;
}

int Living::getMaxHealth() const
{
    return maxHealth;
}

void Living::setMaxHealth(int newMaxHealth)
{
    maxHealth = newMaxHealth;
}

CollisionBox *Living::getGroundBox() const
{
    return groundBox;
}

void Living::setGroundBox(CollisionBox *newGroundBox)
{
    if (groundBox != nullptr)
        delete groundBox;
    groundBox = newGroundBox;
}

bool Living::getOnGround() const
{
    return onGround;
}

void Living::setOnGround(bool newOnGround)
{
    onGround = newOnGround;
}

bool Living::getInvulnerable() const
{
    return invulnerable;
}

void Living::setInvulnerable(bool newInvulnerable)
{
    invulnerable = newInvulnerable;
}

int Living::getDamage() const
{
    return damage;
}

void Living::setDamage(int newDamage)
{
    damage = newDamage;
}