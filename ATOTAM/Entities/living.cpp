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
    nlohmann::json livJson = values["names"][name];
    health = livJson["maxHealth"];
    maxHealth = livJson["maxHealth"];
    invulnerable = livJson["invulnerable"];
    groundBox = new CollisionBox(getBox()->getX(), getBox()->getY() + getBox()->getHeight(), getBox()->getWidth(), 1);
}

Living::Living(const Living &living)
    : Living(living.getX(), living.getY(), living.getFacing(), living.getName())
{

}

Living::~Living()
{
    if (groundBox != nullptr) {
        delete groundBox;
        groundBox = nullptr;
    }
}

bool Living::hit(int damage, Entity *origin, double kb, bool forced)
{
    if (damage != 0)
        iTime = Entity::values["names"][origin->getName()]["iTime"];
    health -= damage;
    if (origin != nullptr && kb != 0.0) {
        if (forced)
            forceKnockback(origin, kb);
        else
            applyKnockback(origin, kb);
    }
    return health <= 0;
}

int Living::getHealth()
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

void Living::setGroundBoxNoDel(CollisionBox *newGroundBox)
{
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

double Living::getITime() const
{
    return iTime;
}

void Living::setITime(double newITime)
{
    iTime = newITime;
}

Entity *Living::getStandingOn() const
{
    return standingOn;
}

void Living::setStandingOn(Entity *newStandingOn)
{
    standingOn = newStandingOn;
}
