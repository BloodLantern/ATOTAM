#include "save.h"

// Setups a Json object corresponding to this file and then creates and returns a Save object using that Json object.
Save Save::load(std::ifstream file)
{
    nlohmann::json json;
    file >> json;
    return Save(json);
}

Save::Save(nlohmann::json json)
    : samosHealth(json["samos"]["health"]), samosMaxHealth(json["samos"]["maxHealth"]), samosMissiles(json["samos"]["missiles"]), samosMaxMissiles(json["samos"]["maxMissiles"]),
      samosGrenades(json["samos"]["grenades"]), samosMaxGrenades(json["samos"]["maxGrenade"]), playTime(json["playTime"]), deaths(json["deaths"]), damageDone(json["damageDone"]),
      damageReceived(json["damageReceived"])
{

}

// Setups a Json object with all the fields of this object and serializes it in this file.
void Save::save(std::ofstream file)
{
    nlohmann::json json;
    json["samos"]["health"] = samosHealth;
    json["samos"]["maxHealth"] = samosMaxHealth;
    json["samos"]["missiles"] = samosMissiles;
    json["samos"]["maxMissiles"] = samosMaxMissiles;
    json["samos"]["grenades"] = samosGrenades;
    json["samos"]["maxGrenades"] = samosMaxGrenades;
    json["playTime"] = playTime;
    json["deaths"] = deaths;
    json["damageDone"] = damageDone;
    json["damageReceived"] = damageReceived;
    file << json;
}

int Save::getSamosHealth() const
{
    return samosHealth;
}

void Save::setSamosHealth(int newSamosHealth)
{
    samosHealth = newSamosHealth;
}

int Save::getSamosMaxHealth() const
{
    return samosMaxHealth;
}

void Save::setSamosMaxHealth(int newSamosMaxHealth)
{
    samosMaxHealth = newSamosMaxHealth;
}

int Save::getSamosMissiles() const
{
    return samosMissiles;
}

void Save::setSamosMissiles(int newSamosMissiles)
{
    samosMissiles = newSamosMissiles;
}

int Save::getSamosMaxMissiles() const
{
    return samosMaxMissiles;
}

void Save::setSamosMaxMissiles(int newSamosMaxMissiles)
{
    samosMaxMissiles = newSamosMaxMissiles;
}

int Save::getSamosGrenades() const
{
    return samosGrenades;
}

void Save::setSamosGrenades(int newSamosGrenades)
{
    samosGrenades = newSamosGrenades;
}

int Save::getSamosMaxGrenades() const
{
    return samosMaxGrenades;
}

void Save::setSamosMaxGrenades(int newSamosMaxGrenades)
{
    samosMaxGrenades = newSamosMaxGrenades;
}

unsigned long Save::getPlayTime() const
{
    return playTime;
}

void Save::setPlayTime(unsigned long newPlayTime)
{
    playTime = newPlayTime;
}

int Save::getDeaths() const
{
    return deaths;
}

void Save::setDeaths(int newDeaths)
{
    deaths = newDeaths;
}

int Save::getDamageDone() const
{
    return damageDone;
}

void Save::setDamageDone(int newDamageDone)
{
    damageDone = newDamageDone;
}

int Save::getDamageReceived() const
{
    return damageReceived;
}

void Save::setDamageReceived(int newDamageReceived)
{
    damageReceived = newDamageReceived;
}