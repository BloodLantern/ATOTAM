#ifndef SAVE_H
#define SAVE_H

#include <fstream>
#include "nlohmann/json.hpp"

class Save
{
public:
    static Save load(std::string fileName); // Creates and returns a Save object corresponding to the Json file represented by the stream
    Save(nlohmann::json json); // Creates a new Save object using this Json object
    void save(std::string fileName); // Converts this Save object in Json and then serializes it
    void copyStats(Save value);

    int getSamosHealth() const;
    void setSamosHealth(int newSamosHealth);

    int getSamosMaxHealth() const;
    void setSamosMaxHealth(int newSamosMaxHealth);

    int getSamosMissiles() const;
    void setSamosMissiles(int newSamosMissiles);

    int getSamosMaxMissiles() const;
    void setSamosMaxMissiles(int newSamosMaxMissiles);

    int getSamosGrenades() const;
    void setSamosGrenades(int newSamosGrenades);

    int getSamosMaxGrenades() const;
    void setSamosMaxGrenades(int newSamosMaxGrenades);

    unsigned long getPlayTime() const;
    void setPlayTime(unsigned long newPlayTime);

    int getDeaths() const;
    void setDeaths(int newDeaths);
    void addDeaths(int newDeaths);

    int getDamageDone() const;
    void setDamageDone(int newDamageDone);
    void addDamageDone(int newDamageDone);

    int getDamageReceived() const;
    void setDamageReceived(int newDamageReceived);
    void addDamageReceived(int newDamageReceived);

    int getSavepointID() const;
    void setSavepointID(int newSavepointID);

    const std::string &getSaveMapName() const;
    void setSaveMapName(const std::string &newSaveMapName);

    std::string getRoomID() const;
    void setRoomID(std::string newRoomID);

    std::map<std::string, std::vector<std::string> > &getRoomsDiscovered();
    void setRoomsDiscovered(const std::map<std::string, std::vector<std::string>> &newRoomsDiscovered);
    void addRoomDiscovered(std::string mapName, std::string roomID);

private:
    int samosHealth;
    int samosMaxHealth;
    int samosMissiles;
    int samosMaxMissiles;
    int samosGrenades;
    int samosMaxGrenades;
    int savepointID;
    std::string roomID;
    std::string saveMapName;
    // Items
    std::map<std::string, std::vector<std::string>> roomsDiscovered;
    unsigned long playTime; // In seconds
    int deaths;
    int damageDone;
    int damageReceived;
    // And more ?
};

#endif // SAVE_H
