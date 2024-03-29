#include "editorpreview.h"
#include "qmimedata.h"
#include "resizeedit.h"
#include "moveedit.h"
#include "qpainter.h"
#include "removeedit.h"
#include "addedit.h"
#include "propertiesedit.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <iostream>
#include <windows.h>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include "../ATOTAM/Entities/terrain.h"
#include "../ATOTAM/Entities/door.h"
#include "../ATOTAM/Entities/area.h"
#include "../ATOTAM/Entities/npc.h"
#include "../ATOTAM/Entities/monster.h"

EditorPreview::EditorPreview(Map* map, QImage* errorTexture, QImage* emptyTexture, int renderingMultiplier, nlohmann::json editorJson, double physicsFrameRate, std::string assetsPath)
    : assetsPath(assetsPath)
    , editorJson(editorJson)
    , windowsKeyCodes(loadJson("windowsKeyCodes"))
    , currentMap(*map)
    , entities(map->loadRooms())
    , errorTexture(errorTexture)
    , emptyTexture(emptyTexture)
    , renderingMultiplier(renderingMultiplier)
    , physicsFrameRate(physicsFrameRate)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    setAcceptDrops(true);
    camera.setX(editorJson["lastLaunch"]["map"]["camera"][0]);
    camera.setY(editorJson["lastLaunch"]["map"]["camera"][1]);
    zoomFactor = editorJson["lastLaunch"]["map"]["zoomFactor"];
    roomId = editorJson["lastLaunch"]["map"]["roomId"];
}

EditorPreview::~EditorPreview()
{
    if (emptyTexture) {
        delete emptyTexture;
        emptyTexture = nullptr;
    }
    if (errorTexture) {
        delete errorTexture;
        errorTexture = nullptr;
    }
}

void EditorPreview::getInputs()
{
    for (std::map<std::string, bool>::iterator i = inputList.begin(); i != inputList.end(); i ++) {
        if (i->second)
            inputList[i->first] += 1 / physicsFrameRate;
        else
            inputList[i->first] = 0;
    }

    //Only listen for inputs if the window is currently selected
    if (isActiveWindow()) {
        //Check every key
        for (nlohmann::json::iterator i = editorJson["inputs"].begin(); i != editorJson["inputs"].end(); i++) {
            for (nlohmann::json::iterator j = i.value().begin(); j != i.value().end(); j++)
                if (GetKeyState(windowsKeyCodes[j.value()].get<int>()) & 0x8000) {
                    inputList[i.key()] = true;
                    break;
                } else
                    inputList[i.key()] = false;
        }
    } else
        //Reset the keys if the window is not selected
        for (nlohmann::json::iterator i = editorJson["inputs"].begin(); i != editorJson["inputs"].end(); i++) {
            inputList[i.key()] = 0;
        }
}

void EditorPreview::updateProperty(std::string key, std::string value)
{
    clearUnmadeEdits();

    nlohmann::json property;
    if (selected->getJsonRepresentation(true)[key].is_number())
        try {
            // Try to set the value as an integer
            property[key] = stoi(value);
            if (key == "x")
                property[key] += currentMap.getJson()->at(""_json_pointer)["rooms"][selected->getRoomId()]["position"][0];
            else if (key == "y")
                property[key] += currentMap.getJson()->at(""_json_pointer)["rooms"][selected->getRoomId()]["position"][1];
        } catch (...) {
            // If it fails, set it as a string instead
            property[key] = value;
        }
    else
        property[key] = value;

    PropertiesEdit* edit = new PropertiesEdit(&currentMap, selected, property);
    edit->make();
    edits.push_back(edit);

    update();
}

nlohmann::json EditorPreview::loadJson(std::string fileName)
{
    std::ifstream file(assetsPath + "/" + fileName + ".json");
    nlohmann::json temp;
    file >> temp;
    return temp;
}

void EditorPreview::saveJson(nlohmann::json json, std::string fileName)
{
    std::ofstream file(assetsPath + "/" + fileName + ".json");
    file << json.dump(4);
    file.close();
}

QPoint EditorPreview::getClickAreaOffset(Entity *ent)
{
    if (ent->getTexture() != nullptr && ent->getTexture() != emptyTexture)
        return ent->getTexture()->offset();
    else
        return QPoint(ent->getBox()->getX(), ent->getBox()->getY());
}

std::pair<int, int> EditorPreview::getClickAreaSize(Entity *ent)
{
    if (ent->getTexture() != nullptr && ent->getTexture() != emptyTexture)
        return std::pair<int, int>(ent->getTexture()->width(), ent->getTexture()->height());
    else
        return std::pair<int, int>(ent->getBox()->getWidth(), ent->getBox()->getHeight());
}

void EditorPreview::updateCursor(QPoint pos) // Relative to the entity
{
    bool changedCursor = false;

    // Setting up variables
    int maxDistance = editorJson["values"]["resizeMaximumCursorDistance"];

    if (selected) {
        if (pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() - maxDistance) * zoomFactor // If on the left of the right edge
                && pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() + maxDistance) * zoomFactor // If on the right of the left edge
                && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() - maxDistance) * zoomFactor // If over the bottom edge
                && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() + maxDistance) * zoomFactor) { // If under the top edge
            setCursor(Qt::CursorShape::OpenHandCursor);
            changedCursor = true;
        }

        bool left = false;
        if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() - maxDistance) * zoomFactor // Not too high
                && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor) // Not too low
            left = true;
        bool right = false;
        if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() - maxDistance) * zoomFactor // Not too high
                && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor) // Not too low
            right = true;
        bool up = false;
        if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() - maxDistance) * zoomFactor // Not too high
                && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() - camera.y() + maxDistance) * zoomFactor) // Not too low
            up = true;
        bool down = false;
        if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() - maxDistance) * zoomFactor // Not too high
                && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor) // Not too low
            down = true;

        if ((left && up) || (right && down)) {
            setCursor(Qt::CursorShape::SizeFDiagCursor);
            changedCursor = true;
        }
        else if ((left && down) || (right && up)) {
            setCursor(Qt::CursorShape::SizeBDiagCursor);
            changedCursor = true;
        }
        else if (left || right) {
            setCursor(Qt::CursorShape::SizeHorCursor);
            changedCursor = true;
        }
        else if (up || down) {
            setCursor(Qt::CursorShape::SizeVerCursor);
            changedCursor = true;
        }
    }

    if (!changedCursor)
        setCursor(Qt::CursorShape::ArrowCursor);
}

void EditorPreview::undoEdit()
{
    for (auto edit = edits.rbegin(); edit != edits.rend(); edit++)
        if ((*edit)->getMade()) {
            (*edit)->unmake();
            update();
            updateCursor(lastMousePosition);
            break;
        }
}

void EditorPreview::redoEdit()
{
    for (auto edit = edits.begin(); edit != edits.end(); edit++)
        if (!(*edit)->getMade()) {
            (*edit)->make();
            update();
            updateCursor(lastMousePosition);
            break;
        }
}

void EditorPreview::duplicateObject()
{
    if (selected) {
        clearUnmadeEdits();

        std::string type = selected->getEntType();
        Entity* entity = nullptr;
        if (type == "Terrain")
            entity = new Terrain(*dynamic_cast<Terrain*>(selected));
        else if (type == "NPC")
            entity = new NPC(*dynamic_cast<NPC*>(selected));
        else if (type == "Area") {
            Area* a = dynamic_cast<Area*>(selected);
            if (a->getAreaType() == "Door")
                entity = new Door(*dynamic_cast<Door*>(a));
            else
                entity = new Area(*a);
        } else if (type == "Monster")
            entity = new Monster(*dynamic_cast<Monster*>(selected));

        if (entity == nullptr)
            throw std::logic_error("duplicateObject() couldn't create a copy of the selected Entity because its entType differs from the hardcoded ones.");

        entity->setX(entity->getX() + 32);
        entity->setY(entity->getY() + 32);
        entity->setRoomId(selected->getRoomId());

        addObject(entity);
    }
}

void EditorPreview::deleteObject()
{
    if (selected) {
        clearUnmadeEdits();
        RemoveEdit* remove = new RemoveEdit(&currentMap, selected, &entities, &selected);
        remove->make();
        edits.push_back(remove);
        update();
        updateCursor(lastMousePosition);
    }
}

void EditorPreview::moveObjectByOnePixel(std::string direction)
{
    if (!selected)
        return;

    QPoint delta;
    if (direction == "left")
        delta.setX(-1);
    else if (direction == "right")
        delta.setX(1);
    else if (direction == "up")
        delta.setY(-1);
    else if (direction == "down")
        delta.setY(1);

    MoveEdit* move = new MoveEdit(&currentMap, selected, delta);
    move->make();
    edits.push_back(move);
    update();
}

void EditorPreview::resetSizeObject()
{
    if (selected) {
        ResizeEdit* resize = new ResizeEdit(&currentMap, selected, ResizeEdit::UpLeft,
                                            std::pair<int, int>(1 - selected->getHorizontalRepeat(), 1 - selected->getVerticalRepeat()));
        resize->make();
        edits.push_back(resize);
    }
}

void EditorPreview::clearUnmadeEdits()
{
    while (!edits.empty() && !(*(edits.end() - 1))->getMade()) {
        delete *(edits.end() - 1);
        edits.pop_back();
    }
}

void EditorPreview::addObject(Entity *entity)
{
    if (entity) {
        AddEdit* add = new AddEdit(&currentMap, entity, &entities, &selected);
        add->make();
        edits.push_back(add);
        update();
        updateCursor(lastMousePosition);
    }
}

void EditorPreview::clearEntities()
{
    for (auto ent = entities.begin(); ent != entities.end(); ent++)
        delete *ent;
    entities.clear();
}

void EditorPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    visibleEntities.clear();
    removeNullEntities();
    std::vector<Entity*> currentRoom;

    for (std::vector<Entity*>::iterator ent = entities.begin(); ent != entities.end(); ent++) {
        // Make sure not to use a null pointer in case there is one
        if (*ent == nullptr)
            continue;

        // The current room's entities are drawn on top
        if ((*ent)->getRoomId() == roomId) {
            currentRoom.push_back(*ent);
            continue;
        }

        // The selected entity is drawn on top
        if (*ent == selected)
            continue;

        drawEntity(*ent, &painter);
    }

    // Draw current room's entities on top
    for (std::vector<Entity*>::iterator ent = currentRoom.begin(); ent != currentRoom.end(); ent++)
        drawEntity(*ent, &painter);

    if (selected != nullptr) {

        // Draw the selected entity on top
        drawEntity(selected, &painter);

        int x = selected->getX();
        int y = selected->getY();

        if (dragging && !edits.empty()) {
            if (MoveEdit* move = dynamic_cast<MoveEdit*>(*(edits.end() - 1))) {
                x += move->getDelta().x();
                y += move->getDelta().y();
            } else if (ResizeEdit* resize = dynamic_cast<ResizeEdit*>(*(edits.end() - 1)))
                if (MoveEdit* move = resize->getMove()) {
                    x += move->getDelta().x();
                    y += move->getDelta().y();
                }
        }

        // Selected entity overlay
        painter.fillRect((x + selected->getTexture()->offset().x() - camera.x()) * zoomFactor,
                        (y + selected->getTexture()->offset().y() - camera.y()) * zoomFactor,
                        selected->getTexture()->width() * zoomFactor * renderingMultiplier,
                        selected->getTexture()->height() * zoomFactor * renderingMultiplier,
                        QColor(0, 0, 255, editorJson["values"]["selectedEntitiesOverlayOpacity"]));

        if (dragging && !edits.empty())
            if (ResizeEdit* resize = dynamic_cast<ResizeEdit*>(*(edits.end() - 1)))
                if (resize->getDeltaChangedLastFrame()) {
                    resize->setDeltaChangedLastFrame(false);
                }
    }

    // Room outline
    painter.setPen(QColor("magenta"));
    nlohmann::json roomJson = (*currentMap.getJson())["rooms"][roomId];
    painter.drawRect((roomJson["position"][0].get<int>() - camera.x()) * zoomFactor,
            (roomJson["position"][1].get<int>() - camera.y()) * zoomFactor,
            roomJson["size"][0].get<int>() * zoomFactor,
            roomJson["size"][1].get<int>() * zoomFactor);

    painter.end();

    // Update the properties page
    properties->updateProperties();
}

void EditorPreview::drawEntity(Entity* ent, QPainter* painter)
{
    int x = ent->getX();
    int y = ent->getY();

    if (ent == selected)
        if (dragging && !edits.empty()) {
            if (MoveEdit* move = dynamic_cast<MoveEdit*>(*(edits.end() - 1))) {
                x += move->getDelta().x();
                y += move->getDelta().y();
            } else if (ResizeEdit* resize = dynamic_cast<ResizeEdit*>(*(edits.end() - 1))) {
                if (resize->getDeltaChangedLastFrame())
                    selected->setCurrentAnimation(selected->updateAnimation(selected->getState(),
                                              std::pair<int, int>(selected->getHorizontalRepeat() + resize->getDelta().first,
                                                                  selected->getVerticalRepeat() + resize->getDelta().second)));
                if (MoveEdit* move = resize->getMove()) {
                    x += move->getDelta().x();
                    y += move->getDelta().y();
                }
            }
        }

    // If the texture is null, set it to the empty texture
    if (ent->getTexture() == nullptr)
        ent->setTexture(emptyTexture);

    if (ent->getTexture() == emptyTexture) {
        // Draw the outline of the entity to make sure it is visible anyway
        painter->setPen(QColor(0, 255, 0, editorJson["values"]["invisibleEntitiesHitboxOpacity"]));
        painter->drawRect(x + ent->getBox()->getX(),
                         y + ent->getBox()->getY(),
                         ent->getBox()->getWidth(),
                         ent->getBox()->getHeight());
    }

    // Make sure not to draw the Entities that aren't visible
    if (x + getClickAreaOffset(ent).x() + getClickAreaSize(ent).first * renderingMultiplier < camera.x() // If too much on the left
            || x + getClickAreaOffset(ent).x() > camera.x() + width() / zoomFactor // If too much on the right
            || y + getClickAreaOffset(ent).y() + getClickAreaSize(ent).second * renderingMultiplier < camera.y() // If too high
            || y + getClickAreaOffset(ent).y() > camera.y() + height() / zoomFactor) // If too low
        return;

    // This Entity is visible so add it to the visible Entities vector
    visibleEntities.push_back(ent);

    // Try to draw the texture: if it fails, set it to the error texture and try again
    try {
        if (ent->getRoomId() != roomId) {
            // Dark overlay over other room's entities
            QColor base_color(50, 50, 50, editorJson["values"]["otherRoomEntityOverlayOpacity"]);

            QImage new_image = *ent->getTexture();
            QPainter pa(&new_image);
            pa.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            pa.fillRect(new_image.rect(), base_color);
            pa.end();
            painter->drawImage(QRect((x + ent->getTexture()->offset().x() - camera.x()) * zoomFactor,
                                    (y + ent->getTexture()->offset().y() - camera.y()) * zoomFactor,
                                    ent->getTexture()->width() * zoomFactor * renderingMultiplier,
                                    ent->getTexture()->height() * zoomFactor * renderingMultiplier),
                                    new_image);
        } else
            painter->drawImage(QRect((x + ent->getTexture()->offset().x() - camera.x()) * zoomFactor,
                                    (y + ent->getTexture()->offset().y() - camera.y()) * zoomFactor,
                                    ent->getTexture()->width() * zoomFactor * renderingMultiplier,
                                    ent->getTexture()->height() * zoomFactor * renderingMultiplier),
                                    *ent->getTexture());
    } catch (...) {
        ent->setTexture(errorTexture);
        painter->drawImage(QRect((x + ent->getTexture()->offset().x() - camera.x()) * zoomFactor,
                                (y + ent->getTexture()->offset().y() - camera.y()) * zoomFactor,
                                ent->getTexture()->width() * zoomFactor * renderingMultiplier,
                                ent->getTexture()->height() * zoomFactor * renderingMultiplier),
                                *ent->getTexture());
    }
}

void EditorPreview::removeNullEntities()
{
    std::vector<Entity*> nonNullEntities;
    for (auto ent = entities.begin(); ent != entities.end(); ent++)
        if (*ent != nullptr)
            nonNullEntities.push_back(*ent);
    entities = nonNullEntities;
}

void EditorPreview::mousePressEvent(QMouseEvent *event)
{
    lastMousePosition = event->pos();
    if (dragging) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::RightButton) {
        clickStart = event->pos();
        cameraStart = camera;
        event->accept();
    } else if (event->button() == Qt::LeftButton) {
        // Room check
        nlohmann::json mapJson = *currentMap.getJson();
        QPoint pos = event->pos();
        for (auto room : mapJson["rooms"].items())
            if (pos.x() < (room.value()["position"][0].get<int>() + room.value()["size"][0].get<int>() - camera.x()) * zoomFactor // If on the left of the right edge
                    && pos.x() > (room.value()["position"][0].get<int>() - camera.x()) * zoomFactor // If on the right of the left edge
                    && pos.y() < (room.value()["position"][1].get<int>() + room.value()["size"][1].get<int>() - camera.y()) * zoomFactor // If over the bottom edge
                    && pos.y() > (room.value()["position"][1].get<int>() - camera.y()) * zoomFactor) // If under the top edge
                // The click was made in this room so set it as the current one
                roomId = room.key();


        // Entity check
        Entity* clicked = nullptr;
        int maxDistance = editorJson["values"]["resizeMaximumCursorDistance"];
        // Try to select the entity at the mouse position
        for (std::vector<Entity*>::iterator ent = visibleEntities.begin(); ent != visibleEntities.end(); ent++)
                if (pos.x() < ((*ent)->getX() + getClickAreaOffset(*ent).x() + getClickAreaSize(*ent).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // If on the left of the right edge
                        && pos.x() > ((*ent)->getX() + getClickAreaOffset(*ent).x() - camera.x() - maxDistance) * zoomFactor // If on the right of the left edge
                        && pos.y() < ((*ent)->getY() + getClickAreaOffset(*ent).y() + getClickAreaSize(*ent).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor // If over the bottom edge
                        && pos.y() > ((*ent)->getY() + getClickAreaOffset(*ent).y() - camera.y() - maxDistance) * zoomFactor) // If under the top edge
                    clicked = *ent;

        if (clicked == nullptr) {
            // Deselection
            selected = nullptr;
            setCursor(Qt::CursorShape::ArrowCursor);
        } else {
            // Selection
            if (clicked->getRoomId() != roomId)
                // If the object was in another room, change the current room id accordingly
                roomId = clicked->getRoomId();

            // If false, it is a resize
            bool move = false;

            // If inside the block (not on the edges)
            if (pos.x() < (clicked->getX() + getClickAreaOffset(clicked).x() + getClickAreaSize(clicked).first * renderingMultiplier - camera.x() - maxDistance) * zoomFactor // If on the left of the right edge
                    && pos.x() > (clicked->getX() + getClickAreaOffset(clicked).x() - camera.x() + maxDistance) * zoomFactor // If on the right of the left edge
                    && pos.y() < (clicked->getY() + getClickAreaOffset(clicked).y() + getClickAreaSize(clicked).second * renderingMultiplier - camera.y() - maxDistance) * zoomFactor // If over the bottom edge
                    && pos.y() > (clicked->getY() + getClickAreaOffset(clicked).y() - camera.y() + maxDistance) * zoomFactor) // If under the top edge
                move = true;

            if (selected != clicked)
                // If a new object was clicked, select it
                selected = clicked;
            else {
                if (move)
                    // If the selected object was pressed, set the moving cursor
                    setCursor(Qt::CursorShape::ClosedHandCursor);
            }

            dragging = true;
            clickStart = pos;

            // Remove the unmade edits if they exist
            clearUnmadeEdits();

            // Create and push back the right edit object
            if (move)
                edits.push_back(new MoveEdit(&currentMap, selected));
            else {
                bool left = false;
                if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                        && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                        && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() - maxDistance) * zoomFactor // Not too high
                        && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor) // Not too low
                    left = true;
                bool right = false;
                if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                        && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                        && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() - maxDistance) * zoomFactor // Not too high
                        && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor) // Not too low
                    right = true;
                bool up = false;
                if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                        && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                        && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() - camera.y() - maxDistance) * zoomFactor // Not too high
                        && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() - camera.y() + maxDistance) * zoomFactor) // Not too low
                    up = true;
                bool down = false;
                if (pos.x() > (selected->getX() + getClickAreaOffset(selected).x() - camera.x() - maxDistance) * zoomFactor // Not too much on the left
                        && pos.x() < (selected->getX() + getClickAreaOffset(selected).x() + getClickAreaSize(selected).first * renderingMultiplier - camera.x() + maxDistance) * zoomFactor // Not too much on the right
                        && pos.y() > (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() - maxDistance) * zoomFactor // Not too high
                        && pos.y() < (selected->getY() + getClickAreaOffset(selected).y() + getClickAreaSize(selected).second * renderingMultiplier - camera.y() + maxDistance) * zoomFactor) // Not too low
                    down = true;

                if (left && up)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::UpLeft));
                else if (right && up)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::UpRight));
                else if (left && down)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::DownLeft));
                else if (right && down)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::DownRight));
                else if (left)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::Left));
                else if (right)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::Right));
                else if (up)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::Up));
                else if (down)
                    edits.push_back(new ResizeEdit(&currentMap, selected, ResizeEdit::Down));
            }
        }

        event->accept();
        update();
    } else
        event->ignore();
}

void EditorPreview::mouseReleaseEvent(QMouseEvent *event)
{
    lastMousePosition = event->pos();
    if (event->button() == Qt::RightButton || event->button() == Qt::LeftButton) {
        farEnough = false;
    }

    if (event->button() == Qt::LeftButton) {
        dragging = false;
        if (selected != nullptr)
            if (!edits.empty()) {
                if (MoveEdit* move = dynamic_cast<MoveEdit*>(*(edits.end() - 1))) {
                    setCursor(Qt::CursorShape::OpenHandCursor);
                    if (!move->getDelta().isNull())
                        move->make();
                    else {
                        delete move;
                        edits.pop_back();
                    }
                } else if (ResizeEdit* resize = dynamic_cast<ResizeEdit*>(*(edits.end() - 1))) {
                    if (resize->getDelta().first != 0 || resize->getDelta().second != 0)
                        resize->make();
                    else {
                        delete resize;
                        edits.pop_back();
                    }
                }
            }
    }
}

void EditorPreview::mouseMoveEvent(QMouseEvent *event)
{
    lastMousePosition = event->pos();
    if ((event->buttons() & Qt::LeftButton) && dragging && !edits.empty()) {
        if (MoveEdit* move = dynamic_cast<MoveEdit*>(*(edits.end() - 1))) {
            QPoint pos = event->pos();
            setCursor(Qt::CursorShape::ClosedHandCursor);
            // If the Shift key is held
            getInputs();
            QPoint delta = move->getDelta();
            if (inputList["disableMovingSteps"]) {
                // Move object without steps
                delta.setX((pos.x() - clickStart.x()) / zoomFactor);
                delta.setY((pos.y() - clickStart.y()) / zoomFactor);
            } else {
                // Move object with steps
                delta.setX(((int)(std::round(pos.x() - clickStart.x()) / zoomFactor))
                           / editorJson["values"]["selectedEntityMoveStep"].get<int>()
                           * editorJson["values"]["selectedEntityMoveStep"].get<int>());
                delta.setY(((int)(std::round(pos.y() - clickStart.y()) / zoomFactor))
                           / editorJson["values"]["selectedEntityMoveStep"].get<int>()
                           * editorJson["values"]["selectedEntityMoveStep"].get<int>());
            }
            move->setDelta(delta);
            event->accept();
            update();
        } else if (ResizeEdit* resize = dynamic_cast<ResizeEdit*>(*(edits.end() - 1))) {
            QPoint pos = event->pos();
            std::pair<int, int> delta = resize->getDelta();
            ResizeEdit::Direction direction = resize->getDirection();
            if (direction == ResizeEdit::Down
                    || direction == ResizeEdit::Right
                    || direction == ResizeEdit::DownRight) {
                if (direction != ResizeEdit::Down)
                    delta.first = std::max(std::round(((pos.x() - clickStart.x()) / zoomFactor)
                                            / Entity::values["names"][selected->getName()]["width"].get<int>() + 0.25),
                            1.0 - selected->getHorizontalRepeat());
                if (direction != ResizeEdit::Right)
                    delta.second = std::max(std::round(((pos.y() - clickStart.y()) / zoomFactor)
                                             / Entity::values["names"][selected->getName()]["height"].get<int>() + 0.25),
                            1.0 - selected->getVerticalRepeat());
            } else {
                MoveEdit* move = resize->getMove();
                if (!move)
                    move = new MoveEdit(&currentMap, selected);
                if (direction != ResizeEdit::Up)
                    delta.first = std::max(-std::round(((pos.x() - clickStart.x()) / zoomFactor)
                                              / Entity::values["names"][selected->getName()]["width"].get<int>() + 0.25),
                            1.0 - selected->getHorizontalRepeat());
                if (direction != ResizeEdit::Left)
                    delta.second = std::max(-std::round(((pos.y() - clickStart.y()) / zoomFactor)
                                               / Entity::values["names"][selected->getName()]["height"].get<int>() + 0.25),
                            1.0 - selected->getVerticalRepeat());
                if (direction == ResizeEdit::UpRight)
                    delta.first = std::max(std::round(((pos.x() - clickStart.x()) / zoomFactor)
                                              / Entity::values["names"][selected->getName()]["width"].get<int>() + 0.25),
                            1.0 - selected->getHorizontalRepeat());
                if (direction == ResizeEdit::DownLeft)
                    delta.second = std::max(std::round(((pos.y() - clickStart.y()) / zoomFactor)
                                               / Entity::values["names"][selected->getName()]["height"].get<int>() + 0.25),
                            1.0 - selected->getVerticalRepeat());

                if (direction != ResizeEdit::UpRight
                        && direction != ResizeEdit::DownLeft)
                    move->setDelta(QPoint(-delta.first * Entity::values["names"][selected->getName()]["width"].get<int>(),
                            -delta.second * Entity::values["names"][selected->getName()]["height"].get<int>()));
                else if (direction == ResizeEdit::UpRight)
                    move->setDelta(QPoint(0, -delta.second * Entity::values["names"][selected->getName()]["height"].get<int>()));
                else if (direction == ResizeEdit::DownLeft)
                    move->setDelta(QPoint(-delta.first * Entity::values["names"][selected->getName()]["width"].get<int>(), 0));
            }
            // For calls inside setter
            resize->setDelta(delta);

            event->accept();
            update();
        }
    } else if (event->buttons() & Qt::RightButton) {
        if (dragging) {
            event->ignore();
            return;
        }
        QPoint pos = event->pos();
        // If the mouse was moved far enough
        if (std::sqrt(std::pow(pos.x() - clickStart.x(), 2) + std::pow(pos.y() - clickStart.y(), 2))
                > editorJson["values"]["cameraMinimumMove"])
            farEnough = true;

        if (farEnough) {
            // Move camera
            camera.setX(cameraStart.x() + (clickStart.x() - pos.x()) / zoomFactor);
            camera.setY(cameraStart.y() + (clickStart.y() - pos.y()) / zoomFactor);
            event->accept();
            update();
        }
    } else {
        if (selected) {
            updateCursor(event->pos());
            event->accept();
        }
    }
}

void EditorPreview::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        // Camera offset after zoom
        camera.setX(camera.x() + event->position().x() * 0.2 / zoomFactor);
        camera.setY(camera.y() + event->position().y() * 0.2 / zoomFactor);
        // Zoom in
        zoomFactor *= editorJson["values"]["zoomMultiplier"].get<float>();
    } else if (event->angleDelta().y() < 0) {
        // Camera offset after zoom
        camera.setX(camera.x() - event->position().x() * 0.25 / zoomFactor);
        camera.setY(camera.y() - event->position().y() * 0.25 / zoomFactor);
        // Zoom out
        zoomFactor /= editorJson["values"]["zoomMultiplier"].get<float>();
    } else
        event->ignore();

    if (event->angleDelta().y() != 0) {
        // Zoom was modified
        event->accept();
        update();
    }
}

void EditorPreview::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        // Tell Qt to accept the drag and drop action
        event->acceptProposedAction();

        // Split the drag and drop data to obtain the EntityType and the EntityName
        std::stringstream str(event->mimeData()->text().toStdString());
        std::string segment;
        std::vector<std::string> seglist;
        while(std::getline(str, segment, ':'))
        {
           seglist.push_back(segment);
        }

        // Here 'seglist' contains 2 strings: entType and entName
        std::string type = seglist[0];
        std::string name = seglist[1];

        float x = (event->position().x() + camera.x()) / zoomFactor;
        float y = (event->position().y() + camera.y()) / zoomFactor;
        clickStart = event->position().toPoint();

        // So now we can instantiate the Entity
        Entity* entity = nullptr;
        if (type == "Terrain")
            entity = new Terrain(x, y, name);
        else if (type == "NPC")
            entity = new NPC(x, y, "Right", name);
        else if (type == "Area") {
            if (name.substr(name.size() - 4, 4) == "Door")
                entity = new Door(x, y, name);
            else
                entity = new Area(x, y, name);
        } else if (type == "Monster")
            entity = new Monster(x, y, "Right", name);

        entity->setRoomId(roomId);

        // Rendering (should be the last function calls)
        entity->setState(Entity::values["names"][name]["defaultState"]);
        entity->setCurrentAnimation(entity->updateAnimation());
        entity->setFrame(0);
        entity->updateTexture();

        clearUnmadeEdits();
        edits.push_back(new AddEdit(&currentMap, entity, &entities, &selected));
        update();
        updateCursor(lastMousePosition);

        update();
    }
}

void EditorPreview::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasText()) {
        // Tell Qt to accept the drag and drop action
        event->acceptProposedAction();

        if (selected != nullptr)
            if (dynamic_cast<AddEdit*>(edits[edits.size() - 1])) {
                QPointF pos = event->position();
                setCursor(Qt::CursorShape::ClosedHandCursor);
                getInputs();
                if (inputList["disableMovingSteps"]) {
                    // If the Shift key is held
                    // Move object without steps
                    selected->setX((pos.x() - clickStart.x()) / zoomFactor + clickStart.x() + camera.x());
                    selected->setY((pos.y() - clickStart.y()) / zoomFactor + clickStart.y() + camera.y());
                } else {
                    // Otherwise
                    // Move object with steps
                    selected->setX(((int)(std::round(pos.x() - clickStart.x()) / zoomFactor + clickStart.x() + camera.x()))
                                   / editorJson["values"]["selectedEntityMoveStep"].get<int>()
                                   * editorJson["values"]["selectedEntityMoveStep"].get<int>());
                    selected->setY(((int)(std::round(pos.y() - clickStart.y()) / zoomFactor + clickStart.y() + camera.y()))
                                   / editorJson["values"]["selectedEntityMoveStep"].get<int>()
                                   * editorJson["values"]["selectedEntityMoveStep"].get<int>());
                }
                update();
            }
    }
}

void EditorPreview::dragLeaveEvent(QDragLeaveEvent *)
{
    if (AddEdit* add = dynamic_cast<AddEdit*>(edits[edits.size() - 1])) {
        delete add;
        edits.pop_back();
        setCursor(Qt::ArrowCursor);
        update();
    }
}

void EditorPreview::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
    if (AddEdit* add = dynamic_cast<AddEdit*>(edits[edits.size() - 1])) {
        add->make();
        setFocus(Qt::MouseFocusReason);
        updateCursor(event->position().toPoint());
        update();
    }
}

nlohmann::json &EditorPreview::getEditorJson()
{
    return editorJson;
}

void EditorPreview::setEditorJson(nlohmann::json &newEditorJson)
{
    editorJson = newEditorJson;
}

const std::string &EditorPreview::getAssetsPath() const
{
    return assetsPath;
}

void EditorPreview::setAssetsPath(const std::string &newAssetsPath)
{
    assetsPath = newAssetsPath;
}

QPoint EditorPreview::getCamera() const
{
    return camera;
}

void EditorPreview::setCamera(QPoint newCamera)
{
    camera = newCamera;
}

Map* EditorPreview::getCurrentMap()
{
    return &currentMap;
}

void EditorPreview::setCurrentMap(const Map &newCurrentMap)
{
    currentMap = newCurrentMap;
}

std::string EditorPreview::getRoomId() const
{
    return roomId;
}

void EditorPreview::setRoomId(std::string newRoomId)
{
    roomId = newRoomId;
}

double EditorPreview::getZoomFactor() const
{
    return zoomFactor;
}

void EditorPreview::setZoomFactor(double newZoomFactor)
{
    zoomFactor = newZoomFactor;
}

Entity *EditorPreview::getSelected() const
{
    return selected;
}

void EditorPreview::setSelected(Entity *newSelected)
{
    selected = newSelected;
}

Entity **EditorPreview::getSelectedPointer()
{
    return &selected;
}

std::vector<Edit*> &EditorPreview::getEdits()
{
    return edits;
}

void EditorPreview::setEdits(std::vector<Edit*> &newEdits)
{
    edits = newEdits;
}

std::map<std::string, bool> &EditorPreview::getInputList()
{
    return inputList;
}

void EditorPreview::setInputList(std::map<std::string, bool> &newInputList)
{
    inputList = newInputList;
}

PropertiesModel *EditorPreview::getProperties() const
{
    return properties;
}

void EditorPreview::setProperties(PropertiesModel *newProperties)
{
    properties = newProperties;
}

const std::vector<Entity *> &EditorPreview::getEntities() const
{
    return entities;
}

void EditorPreview::setEntities(const std::vector<Entity *> &newEntities)
{
    entities = newEntities;
}
