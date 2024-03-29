#include "addedit.h"
#include "removeedit.h"

RemoveEdit::RemoveEdit(Map* map, Entity* entity, std::vector<Entity *>* entityList, Entity **selectedEntity)
    : MultiTypeEdit(map, entity)
    , entityList(entityList)
    , selectedEntity(selectedEntity)
{

}

RemoveEdit::RemoveEdit(Map* map, int roomId)
    : MultiTypeEdit(map, roomId)
{

}

RemoveEdit::~RemoveEdit()
{
    if (!roomEdit && made && entity) {
        delete entity;
        entity = nullptr;
    }
}


void RemoveEdit::unmake()
{
    if (roomEdit) {
    } else {
        AddEdit edit(map, entity, entityList);
        edit.make();
    }
    made = false;
}

void RemoveEdit::make()
{
    if (roomEdit) {
    } else {
        // Get map Json pointer
        nlohmann::json* mapJson = map->getJson();
        // json_pointer to the entity list
        nlohmann::json::json_pointer ptr("/rooms/" + entity->getRoomId() + "/content/" + entity->getEntType() + "/" + entity->getFullName());
        // Find Entity json node in the map
        nlohmann::json entityJson = map->find(entity);
        // Iterate over the entities with the same entType and name
        for (auto& ent : mapJson->at(ptr).items())
            // Find the correct one
            if (ent.value() == entityJson) {
                // Remove it
                mapJson->at(ptr).erase(std::stoi(ent.key()));
                for (auto ent = entityList->begin(); ent != entityList->end(); ent++)
                    if (*ent == entity) {
                        if (entityList)
                            entityList->erase(ent);
                        if (selectedEntity)
                            *selectedEntity = nullptr;
                        break;
                    }
                break;
            }

        // If the entity name array is empty, remove it
        if (mapJson->at(ptr).empty()) {
            ptr = nlohmann::json::json_pointer("/rooms/" + entity->getRoomId() + "/content/" + entity->getEntType());
            mapJson->at(ptr).erase(entity->getFullName());
        }
        // If the entity name array is empty, remove it
        if (mapJson->at(ptr).empty()) {
            ptr = nlohmann::json::json_pointer("/rooms/" + entity->getRoomId() + "/content");
            mapJson->at(ptr).erase(entity->getEntType());
        }
    }
    made = true;
}
