#include "living.h"

Living::Living(double x, double y, CollisionBox* box, QImage* texture, Entity::EntityType entityType, int health, int maxHealth, bool isAffectedByGravity, Direction facing)
    : Entity(x, y, box, texture, entityType, isAffectedByGravity, facing, true),
      health(health), maxHealth(maxHealth), groundBox(new CollisionBox(box->getX(), box->getY() + box->getHeight(), box->getWidth(), 2)), onGround(false)
{

}

Living::~Living()
{
    delete groundBox;
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