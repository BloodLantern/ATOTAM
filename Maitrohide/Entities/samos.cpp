#include "samos.h"
#include <iostream>

Samos::Samos(double x, double y, int maxHealth, int maxGrenadeCount, int maxMissileCount)
    : Living(x, y, "Right", "Samos"),
      isInAltForm(false), maxGrenadeCount(maxGrenadeCount), maxMissileCount(maxMissileCount),
      wallBoxR(new CollisionBox(getBox()->getX() + getBox()->getWidth(), getBox()->getY(), 2, getBox()->getHeight())),
      wallBoxL(new CollisionBox(getBox()->getX() - 2, getBox()->getY(), 2, getBox()->getHeight()))
{
    setMaxHealth(maxHealth);
}

Samos::Samos(double x, double y, int maxHealth, int maxGrenadeCount, int maxMissileCount, CollisionBox *box, QImage *texture, std::string entityType, int health, bool isAffectedByGravity, std::string facing, double frictionFactor, std::string name, bool isMovable)
    : Living(x, y, box, texture, entityType, health, maxHealth, isAffectedByGravity, facing, frictionFactor, name, isMovable),
      maxGrenadeCount(maxGrenadeCount),
      maxMissileCount(maxMissileCount),
      wallBoxR(new CollisionBox(box->getX() + box->getWidth(), box->getY(), 2, box->getHeight())),
      wallBoxL(new CollisionBox(box->getX() - 2, box->getY(), 2, box->getHeight()))
{

}

Samos::~Samos()
{
    delete wallBoxL;
    delete wallBoxR;
}

Projectile* Samos::shoot(std::string type)
{
    Projectile* projectile = nullptr;
    double renderingM = values["general"]["renderingMultiplier"];

    //Can't shoot a missile/grenade if you don't have any
    if (type == "Grenade") {
        if (grenadeCount > 0)
            grenadeCount--;
        else
            return projectile;
    } else if (type == "Missile") {
        if (missileCount > 0)
            missileCount--;
        else
            return projectile;
    }


    std::string shootState;
    if (getState() == "Falling" || getState() == "FallingAimUpDiag" || getState() == "FallingAimDownDiag" || getState() == "FallingAimUp" || getState() == "FallingAimDown"
            || getState() == "SpinJump" || getState() == "WallJump" || getState() == "Jumping" || getState() == "JumpEnd")
        shootState = "Falling";
    else if (getState() == "IdleCrouch" || getState() == "CrouchAimUp" || getState() == "CrouchAimUpDiag" || getState() == "Crouching")
        shootState = "Crouching";
    else if (getState() == "Walking" || getState() == "WalkingAimForward" || getState() == "WalkingAimDown" || getState() == "WalkingAimUp")
        shootState = "Walking";
    else
        shootState = "Standing";

    if (canonDirection == "None")
        canonDirection = "Right";

    nlohmann::json offsetJson = values["names"]["Samos"]["shootOffset"][getFacing()][shootState][canonDirection];
    int offset_x = offsetJson.is_null() ? 0 : static_cast<int>(offsetJson["x"]);
    int offset_y = offsetJson.is_null() ? 0 : static_cast<int>(offsetJson["y"]);


    //Spawn the projectile at certain coordinates to match the sprite
    projectile = new Projectile(getX() + offset_x * renderingM, getY() + offset_y * renderingM, canonDirection, type, type);

    return projectile;
}

bool Samos::getIsInAltForm() const
{
    return isInAltForm;
}

void Samos::setIsInAltForm(bool newIsInAltForm)
{
    isInAltForm = newIsInAltForm;
}

int Samos::getGrenadeCount() const
{
    return grenadeCount;
}

void Samos::setGrenadeCount(int newGrenadeCount)
{
    grenadeCount = newGrenadeCount;
}

int Samos::getMaxGrenadeCount() const
{
    return maxGrenadeCount;
}

void Samos::setMaxGrenadeCount(int newMaxGrenadeCount)
{
    maxGrenadeCount = newMaxGrenadeCount;
}

int Samos::getMissileCount() const
{
    return missileCount;
}

void Samos::setMissileCount(int newMissileCount)
{
    missileCount = newMissileCount;
}

int Samos::getMaxMissileCount() const
{
    return maxMissileCount;
}

void Samos::setMaxMissileCount(int newMaxMissileCount)
{
    maxMissileCount = newMaxMissileCount;
}

double Samos::getJumpTime() const
{
    return jumpTime;
}

void Samos::setJumpTime(double newJumpTime)
{
    jumpTime = newJumpTime;
}

CollisionBox *Samos::getWallBoxR() const
{
    return wallBoxR;
}

void Samos::setWallBoxR(CollisionBox *newWallBoxR)
{
    delete wallBoxR;
    wallBoxR = newWallBoxR;
}

CollisionBox *Samos::getWallBoxL() const
{
    return wallBoxL;
}

void Samos::setWallBoxL(CollisionBox *newWallBoxL)
{
    delete wallBoxL;
    wallBoxL = newWallBoxL;
}

const std::string &Samos::getCanonDirection() const
{
    return canonDirection;
}

void Samos::setCanonDirection(const std::string &newCanonDirection)
{
    canonDirection = newCanonDirection;
}

double Samos::getShootTime() const
{
    return shootTime;
}

void Samos::setShootTime(double newShootTime)
{
    shootTime = newShootTime;
}
