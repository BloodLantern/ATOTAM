#include "savepoint.h"

Savepoint::Savepoint(double x, double y, int spID, std::string mapName)
    : NPC(x, y, "Right", "Savepoint"),
      savepointID(spID),
      mapName(mapName)
{

}

Savepoint::~Savepoint()
{

}

nlohmann::json Savepoint::getJsonRepresentation(bool defaultValues)
{
    nlohmann::json result = NPC::getJsonRepresentation(defaultValues);
    result["spID"] = savepointID;
    return result;
}

void Savepoint::setJsonValues(nlohmann::json json)
{
    Entity::setJsonValues(json);
    if (!json["spID"].is_null())
        savepointID = json["spID"];
}

int Savepoint::getSavepointID() const
{
    return savepointID;
}

void Savepoint::setSavepointID(int newSavepointID)
{
    savepointID = newSavepointID;
}

const std::string &Savepoint::getMapName() const
{
    return mapName;
}

void Savepoint::setMapName(const std::string &newMapName)
{
    mapName = newMapName;
}
