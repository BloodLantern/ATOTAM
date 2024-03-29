#include "game.h"
#include "Entities/door.h"
#include "physics.h"
#include <io.h>
#include <iostream>
#include <Entities/savepoint.h>

nlohmann::json Game::loadJson(std::string fileName)
{
    std::ifstream file(assetsPath + "/" + fileName + ".json");
    nlohmann::json temp;
    file >> temp;
    return temp;
}

void Game::saveJson(nlohmann::json json, std::string fileName)
{
    std::ofstream file(assetsPath + "/" + fileName + ".json");
    file << json.dump(4);
}

void Game::loadGeneral()
{
    //Loading the general settings of the game
    gameSpeed = Entity::values["general"]["gameSpeed"];
    updateRate = Entity::values["general"]["updateRate"];
    showFpsUpdateRate = Entity::values["general"]["showFpsUpdateRate"];
    Physics::gravity = Entity::values["general"]["gravity"];
    mapViewer = Entity::values["general"]["mapViewer"];
    currentMap = Map::loadMap(Entity::values["general"]["map"], assetsPath);
    mapViewerCameraSpeed = Entity::values["general"]["mapViewerCameraSpeed"];
    Physics::frameRate = params["frameRate"];
    resolution.first = params["resolution_x"];
    resolution.second = params["resolution_y"];
    language = params["language"];
    renderHitboxes = Entity::values["general"]["renderHitboxes"];
    showFps = params["showFps"];
    fullscreen = params["fullscreen"];
    mapCameraSpeed = Entity::values["general"]["mapCameraSpeed"];
    cameraSize.first = Entity::values["general"]["camera_size_x"];
    cameraSize.second = Entity::values["general"]["camera_size_y"];
    debugEnabled = Entity::values["general"]["debugEnabled"];
    tasToolEnabled = Entity::values["general"]["frameAdvanceEnabled"];
    showDebugInfo = Entity::values["general"]["showDebugInfo"];
}

void Game::loadSave(Save save)
{
    currentProgress = save;

    // Load map
    currentMap = Map::loadMap(save.getSaveMapName(), assetsPath);
    currentMap.setCurrentRoomId(save.getRoomID());

    // Async room loading to avoid segfaults
    // Unload every room
    for (auto room = roomEntities.begin(); room != roomEntities.end(); room++)
        roomsToUnload.push_back(room->first);
    if (roomWorker) {
        if (roomWorker->joinable())
            roomWorker->join();
        // Reset the worker
        updateAsyncRoomLoading();
    }
    // Unload the rooms
    updateAsyncRoomLoading();
    // Wait for the worker to finish the unloading by joining the threads
    if (roomWorker)
        if (roomWorker->joinable())
            roomWorker->join();
    // Clear the lists to avoid corrupted pointers
    terrains = {};
    monsters = {};
    NPCs = {};
    projectiles = {};
    areas = {};
    dynamicObjs = {};
    entities = {};
    // Then load the needed rooms because at this point all the Entities have been deleted
    updateLoadedRooms();
    updateAsyncRoomLoading();

    // Place Samos
    std::pair<int, int> coords = loadRespawnPosition(save, currentMap);
    addEntity(new Samos(coords.first, coords.second, save.getSamosHealth(), save.getSamosMaxHealth(),
                  save.getSamosGrenades(), save.getSamosMaxGrenades(),
                        save.getSamosMissiles(), save.getSamosMaxMissiles()));

    currentDialogue = Dialogue();
}

void Game::addRoomDiscovered(std::string mapName, std::string roomID)
{
    currentProgress.addRoomDiscovered(mapName, roomID);
}

void Game::die()
{
    updateProgress();
    currentProgress.addDeaths(1);
    lastCheckpoint.copyStats(currentProgress);
    lastSave.copyStats(currentProgress);
    lastSave.save(assetsPath + "/saves/" + saveFile + ".json");
    s->setHealth(0);
    isPaused = true;
    menu = "death";
    menuOptions = {"Respawn", "Quit"};
    selectedOption = 0;
}

void Game::updateTas()
{
    if (!tas)
        return;

    std::ifstream f(assetsPath + "/tas/" + Entity::values["general"]["tasFile"].get<std::string>() + ".tas");
    std::string content;
    // Count the lines and detect the breakpoints
    long i;
    for (i = 0; std::getline(f, content); ++i)
        if (content == "$") {
            furtherBreakpoint = i;
            if (line == 1)
                ultraFastForward = true;
        }

    // If we reached the breakpoint, go in frame advance mode and stop the ultra fast forward
    if (line + linesSkipped - 1 == furtherBreakpoint && furtherBreakpoint > 1) {
        frameAdvance = true;
        ultraFastForward = false;
        line++;
    }

    // Stop the TAS if the last line has been read
    if (i < line + linesSkipped) {
        tas = false;
        line = 1;
        currentInstructionFrames = 1;
        return;
    }

    // Reset the stream value
    f.clear();
    f.seekg(0);

    // Go back to the line we were at the last update + 1
    linesSkipped = 0;
    for (int i = 0; i < line; i++) {
        std::getline(f, content);
        // Remove ' ' and '\t' from the line
        content.erase(std::remove(content.begin(), content.end(), ' '), content.end());
        content.erase(std::remove(content.begin(), content.end(), '\t'), content.end());
        while (content == "" || (content.size() >= 2 && content.substr(0, 2) == "//")) {
            std::getline(f, content);
            // Remove ' ' and '\t' from the line
            content.erase(std::remove(content.begin(), content.end(), ' '), content.end());
            content.erase(std::remove(content.begin(), content.end(), '\t'), content.end());
            linesSkipped++;
        }
    }

    // Remove the not special inputs
    std::map<std::string, bool> toDel;
    for (auto input : inputList) {
        if (input.first.size() > 8)
            if (input.first.substr(0, 8) == "SPECIAL_")
                continue;
        toDel.emplace(input.first, input.second);
    }
    for (const auto &input : toDel)
        inputList.erase(input.first);

    // Analyse the tokens
    std::vector<std::string> tokens = split(content, ',');
    for (unsigned int i = 0; i < tokens.size(); i++) {
        std::string token = tokens[i];
        try {
            unsigned long t = std::stoul(token);
            // Update the frame count
            lastInstructionFrames = currentInstructionFrames;
            if (currentInstructionFrames == t)
                currentInstructionFrames = 1;
            else if (currentInstructionFrames > t)
                currentInstructionFrames -= t + 1;
            else
                currentInstructionFrames++;
        } catch (const std::invalid_argument&) {
            // Add the TASed inputs
            for (const auto &j : keyCodes.items())
                for (const auto &k : j.value())
                    if (k == token) {
                        if (!toDel[j.key()])
                            inputTime[j.key()] = 0;
                        else
                            inputTime[j.key()] += 1 / Physics::frameRate;
                        inputList[j.key()] = true;
                    }
        }
    }

    // Increment the line count if the frame count was reset
    if (currentInstructionFrames == 1)
        line++;
}

// Returns whether a worker thread was started. Also returns true if there's nothing to load/unload
void Game::updateAsyncRoomLoading()
{
    if (roomsToLoad.empty() && roomsToUnload.empty())
        return;

    // A worker thread is necessary

    if (roomWorker == nullptr) {
        roomWorker = new std::thread([this] {
            // For each room to load
            for (auto room = roomsToLoad.begin(); room != roomsToLoad.end(); room++) {
                // Get the room entity vector
                std::vector<Entity*>* ents = roomEntities[*room];

                if (ents == nullptr)
                    // Create it if null
                    ents = new std::vector<Entity*>;
                else
                    // If the room is already loaded, don't go any further
                    if (!ents->empty())
                        continue;

                // For each entity to load
                for (Entity* e : currentMap.loadRoom(*room))
                    // Add it to the vector
                    ents->push_back(e);

                // Eventually add the vector to the map
                roomEntities[*room] = ents;
            }
            roomsToLoad.clear();

            // For each room to unload
            for (auto room = roomsToUnload.begin(); room != roomsToUnload.end(); room++) {
                // Get the room entity vector
                std::vector<Entity*>* ents = roomEntities[*room];

                // If null, nothing needs to be unloaded
                if (ents == nullptr)
                    continue;

                // For each entity to unload
                for (Entity* e : *ents)
                    // Delete it
                    delete e;

                // Also delete the vector
                delete ents;

                // Remove the vector from the map
                roomEntities[*room] = nullptr;
            }
            roomsToUnload.clear();

            workerFinished = true;
        });
    } else if (workerFinished) {
        // Thread finished execution
        if (roomWorker->joinable())
            roomWorker->join();
        delete roomWorker;
        roomWorker = nullptr;

        // Reset value for next thread
        workerFinished = false;
    }
}

void Game::updateLoadedRooms()
{
    std::vector<std::string> loadedRooms = { currentMap.getCurrentRoomId() };
    // If the current room isn't loaded: if the game is starting
    if (roomEntities[currentMap.getCurrentRoomId()] == nullptr) {
        roomEntities[currentMap.getCurrentRoomId()] = new std::vector<Entity*>(currentMap.loadRoom());
        addEntities(*roomEntities[currentMap.getCurrentRoomId()]);
    }

    // For each door in the current room, load its ending room
    for (auto area = areas.begin(); area != areas.end(); area++)
        if ((*area)->getAreaType() == "Door") {
            // This shouldn't cause a crash
            std::string room = dynamic_cast<Door*>(*area)->getEndingRoom();

            // If the room isn't loaded yet, add it to the list
            if (roomEntities[room] == nullptr)
                roomsToLoad.push_back(room);

            loadedRooms.push_back(room);
        }

    // Unload all the other rooms
    for (auto room = roomEntities.begin(); room != roomEntities.end(); room++) {
        bool listed = false;
        for (const std::string &r : loadedRooms)
            if (r == room->first) {
                listed = true;
                break;
            }
        if (!listed)
            roomsToUnload.push_back(room->first);
    }
}

void Game::updateSpecialInputs()
{
    if (inputList["SPECIAL_toggleFrameAdvance"] && inputTime["SPECIAL_toggleFrameAdvance"] == 0)
        frameAdvance = !frameAdvance;
    if (inputList["SPECIAL_toggleTAS"] && inputTime["SPECIAL_toggleTAS"] == 0)
        tas = !tas;
    if (inputList["SPECIAL_toggleHitboxes"] && inputTime["SPECIAL_toggleHitboxes"] == 0)
        renderHitboxes = !renderHitboxes;
    if (inputList["SPECIAL_toggleFreeCamera"] && inputTime["SPECIAL_toggleFreeCamera"] == 0)
        mapViewer = !mapViewer;
    if (inputList["SPECIAL_restartTAS"] && inputTime["SPECIAL_restartTAS"] == 0) {
        tas = true;
        currentInstructionFrames = 1;
        line = 1;
    }
    if (inputList["SPECIAL_toggleDebugInfo"] && inputTime["SPECIAL_toggleDebugInfo"] == 0)
        showDebugInfo = !showDebugInfo;
}

template <typename Out>
void Game::split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> Game::split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

Game::Game(std::string assetsPath, std::string saveNumber)
    : assetsPath(assetsPath)
    , running(true)
    , keyCodes(loadJson("inputs"))
    , windowsKeyCodes(loadJson("windowsKeyCodes"))
    , stringsJson(loadJson("strings"))
    , isPaused(false)
    , resolution({1920,1080})
    , doorTransition("")
    , params(loadJson("params"))
    , fullscreen(false)
    , currentProgress(Save::load(assetsPath + "/saves/" + saveNumber + ".json"))
    , lastSave(Save::load(assetsPath + "/saves/" + saveNumber + ".json"))
    , lastCheckpoint(Save::load(assetsPath + "/saves/" + saveNumber + ".json"))
    , saveFile(saveNumber)
    , timeAtLaunch(currentProgress.getPlayTime())
    , currentMap(Map::loadMap(currentProgress.getSaveMapName(), assetsPath))
{
    loadGeneral();

    // Load map
    currentMap.setCurrentRoomId(currentProgress.getRoomID());
    updateLoadedRooms();

    std::pair<int, int> coords = loadRespawnPosition(currentProgress, currentMap);

    addEntity(new Samos(coords.first, coords.second, currentProgress.getSamosHealth(), currentProgress.getSamosMaxHealth(),
                  currentProgress.getSamosGrenades(), currentProgress.getSamosMaxGrenades(),
                  currentProgress.getSamosMissiles(), currentProgress.getSamosMaxMissiles()));
}

QString Game::translate(std::string text, std::vector<std::string> subCategories)
{
    nlohmann::json json = stringsJson[language];
    for (const std::string &category : subCategories)
        json = json[category];
    return QString::fromStdString(json[text]);
}

void Game::updateMenu()
{
    if (isPaused) {
        if (inputList["menu"] && inputTime["menu"] == 0.0) {
            if (menu == "main")
                isPaused = false;
            else {
                menu = "main";
                selectedOption = 0;
            }
        }

        if (inputList["down"] && !inputList["up"] && !inputList["left"] && !inputList["right"]) {
            if (menuArrowsTime == 0.0) {
                selectedOption += 1;
                if (selectedOption >= static_cast<int>(menuOptions.size()))
                    selectedOption = 0;
            } else if (menuArrowsTime >= 5 * static_cast<double>(Entity::values["general"]["menuCoolDown"]) || (menuArrowsTime >= -1 && menuArrowsTime < 0.0)) {
                selectedOption += 1;
                if (selectedOption >= static_cast<int>(menuOptions.size()))
                    selectedOption = 0;
                menuArrowsTime = -1 - static_cast<double>(Entity::values["general"]["menuCoolDown"]);
            }

            menuArrowsTime += + 1 / Physics::frameRate;

        } else if (!inputList["down"] && inputList["up"] && !inputList["left"] && !inputList["right"]) {
            if (menuArrowsTime == 0.0) {
                selectedOption -= 1;
                if (selectedOption < 0)
                    selectedOption = menuOptions.size() - 1;
            } else if (menuArrowsTime >= 5 * static_cast<double>(Entity::values["general"]["menuCoolDown"]) || (menuArrowsTime >= -1 && menuArrowsTime < 0.0)) {
                selectedOption -= 1;
                if (selectedOption < 0)
                    selectedOption = menuOptions.size() - 1;
                menuArrowsTime = -1 - static_cast<double>(Entity::values["general"]["menuCoolDown"]);
            }

            menuArrowsTime += + 1 / Physics::frameRate;

        } else if (!inputList["down"] && !inputList["up"] && inputList["left"] && !inputList["right"]) {
            if (menuArrowsTime == 0.0) {
                if (menuOptions[selectedOption].substr(0,8) == "< FPS : ") {
                    if (Physics::frameRate > 60.0) {
                        Physics::frameRate--;
                        params["frameRate"] = Physics::frameRate;
                        std::ofstream paramsfile(assetsPath + "/params.json");
                        paramsfile << params.dump(4);
                        paramsfile.close();
                    }
                } else if (menuOptions[selectedOption].substr(0,15) == "< Game speed : ") {
                    if (gameSpeed > 0.1) {
                        gameSpeed -= 0.1;
                    }
                }
            } else if (menuArrowsTime >= 5 * static_cast<double>(Entity::values["general"]["menuCoolDown"]) || (menuArrowsTime >= -1 && menuArrowsTime < 0.0)) {
                if (menuOptions[selectedOption].substr(0,8) == "< FPS : ") {
                    if (Physics::frameRate > 60.0) {
                        Physics::frameRate--;
                        params["frameRate"] = Physics::frameRate;
                        std::ofstream paramsfile(assetsPath + "/params.json");
                        paramsfile << params.dump(4);
                        paramsfile.close();
                    }
                } else if (menuOptions[selectedOption].substr(0,15) == "< Game speed : ") {
                    if (gameSpeed > 0.1) {
                        gameSpeed -= 0.1;
                    }
                }
                menuArrowsTime = -1 - static_cast<double>(Entity::values["general"]["menuCoolDown"]) / 2;
            }

            menuArrowsTime += + 1 / Physics::frameRate;

        } else if (!inputList["down"] && !inputList["up"] && !inputList["left"] && inputList["right"]) {
            if (menuArrowsTime == 0.0) {
                if (menuOptions[selectedOption].substr(0,8) == "< FPS : ") {
                    if (Physics::frameRate < 144.0) {
                        Physics::frameRate++;
                        params["frameRate"] = Physics::frameRate;
                        std::ofstream paramsfile(assetsPath + "/params.json");
                        paramsfile << params.dump(4);
                        paramsfile.close();
                    }
                } else if (menuOptions[selectedOption].substr(0,15) == "< Game speed : ") {
                    if (gameSpeed < 10.0) {
                        gameSpeed += 0.1;
                    }
                }
            } else if (menuArrowsTime >= 5 * static_cast<double>(Entity::values["general"]["menuCoolDown"]) || (menuArrowsTime >= -1 && menuArrowsTime < 0.0)) {
                if (menuOptions[selectedOption].substr(0,8) == "< FPS : ") {
                    if (Physics::frameRate < 144.0) {
                        Physics::frameRate++;
                        params["frameRate"] = Physics::frameRate;
                        std::ofstream paramsfile(assetsPath + "/params.json");
                        paramsfile << params.dump(4);
                        paramsfile.close();
                    }
                } else if (menuOptions[selectedOption].substr(0,15) == "< Game speed : ") {
                    if (gameSpeed < 10.0) {
                        gameSpeed += 0.1;
                    }
                }
                menuArrowsTime = -1 - static_cast<double>(Entity::values["general"]["menuCoolDown"]) / 2;
            }

            menuArrowsTime += + 1 / Physics::frameRate;

        } else
            menuArrowsTime = 0.0;

        if ((inputList["enter"] && inputTime["enter"] == 0.0) || (inputList["shoot"] && inputTime["shoot"] == 0.0)) {
            if (menuOptions[selectedOption] == "Resume")
                isPaused = false;
            else if (menuOptions[selectedOption] == "Options") {
                menu = "options";
                selectedOption = 0;
            } else if (menuOptions[selectedOption] == "Debug") {
                menu = "debug";
                selectedOption = 0;
            } else if (menuOptions[selectedOption] == "Quit")
                running = false;
            else if (menuOptions[selectedOption] == "Back") {
                menu = "main";
                selectedOption = 0;
            } else if (menuOptions[selectedOption] == "Show FPS : ON") {
                showFps = false;
                params["showFps"] = showFps;
                std::ofstream paramsfile(assetsPath + "/params.json");
                paramsfile << params.dump(4);
                paramsfile.close();
            } else if (menuOptions[selectedOption] == "Show FPS : OFF") {
                showFps = true;
                params["showFps"] = showFps;
                std::ofstream paramsfile(assetsPath + "/params.json");
                paramsfile << params.dump(4);
                paramsfile.close();
            } else if (menuOptions[selectedOption] == "Show hitboxes : ON")
                renderHitboxes = false;
            else if (menuOptions[selectedOption] == "Show hitboxes : OFF")
                 renderHitboxes = true;
            else if (menuOptions[selectedOption] == "Max missiles")
                 s->setMissileCount(s->getMaxMissileCount());
            else if (menuOptions[selectedOption] == "Max grenades")
                 s->setGrenadeCount(s->getMaxGrenadeCount());
            else if (menuOptions[selectedOption] == "Max hp")
                 s->setHealth(s->getMaxHealth());
            else if (menuOptions[selectedOption] == "Reload entities.json") {
                std::string rID = currentMap.getCurrentRoomId();
                Entity::values = Entity::loadValues(assetsPath);
                loadGeneral();
                currentMap.setCurrentRoomId(rID);
            } else if (menuOptions[selectedOption] == "Reload room") {
                clearEntities("Samos");
                addEntities(currentMap.loadRoom());
            } else if (menuOptions[selectedOption] == "Reload map") {
                std::string mapId = currentMap.getCurrentRoomId();
                currentMap = Map::loadMap(currentMap.getName(), assetsPath);
                currentMap.setCurrentRoomId(mapId);
            } else if (menuOptions[selectedOption] == "Map viewer mode : ON")
                mapViewer = false;
            else if (menuOptions[selectedOption] == "Map viewer mode : OFF")
                mapViewer = true;
            else if (menuOptions[selectedOption] == "Fullscreen : ON") {
                fullscreen = false;
                params["fullscreen"] = false;
                std::ofstream paramsfile(assetsPath + "/params.json");
                paramsfile << params.dump(4);
                paramsfile.close();
            } else if (menuOptions[selectedOption] == "Fullscreen : OFF") {
                fullscreen = true;
                params["fullscreen"] = true;
                std::ofstream paramsfile(assetsPath + "/params.json");
                paramsfile << params.dump(4);
                paramsfile.close();
            } else if (menuOptions[selectedOption] == "Reload last checkpoint") {
                loadSave(lastCheckpoint);
                isPaused = false;
            } else if (menuOptions[selectedOption] == "Reload last save") {
                loadSave(lastSave);
                isPaused = false;
            } else if (menuOptions[selectedOption] == "Respawn") {
                loadSave(lastCheckpoint);
                isPaused = false;
            } else if (menuOptions[selectedOption] == "Debug info : ON")
                showDebugInfo = false;
            else if (menuOptions[selectedOption] == "Debug info : OFF")
                showDebugInfo = true;
            else if (menuOptions[selectedOption] == "TAS Tool : ON")
                tasToolEnabled = false;
            else if (menuOptions[selectedOption] == "TAS Tool : OFF")
                tasToolEnabled = true;
        }


        if (menu == "main") {
            menuOptions.clear();
            menuOptions.push_back("Resume");
            menuOptions.push_back("Options");
            menuOptions.push_back("Reload last checkpoint");
            menuOptions.push_back("Reload last save");
            if (debugEnabled)
                menuOptions.push_back("Debug");
            menuOptions.push_back("Quit");
        } else if (menu == "options") {
            menuOptions.clear();
            menuOptions.push_back("Back");
            menuOptions.push_back(std::string("< FPS : ") + std::to_string(int(Physics::frameRate)) + " >");
            menuOptions.push_back(std::string("Show FPS : ") + (showFps ? "ON" : "OFF"));
            menuOptions.push_back(std::string("Fullscreen : ") + (fullscreen ? "ON" : "OFF"));
        } else if (menu == "debug") {
            menuOptions.clear();
            menuOptions.push_back("Back");
            menuOptions.push_back(std::string("< Game speed : ") + std::to_string(gameSpeed).substr(0,4) + " >");
            menuOptions.push_back("Max missiles");
            menuOptions.push_back("Max grenades");
            menuOptions.push_back("Max hp");
            menuOptions.push_back(std::string("Show hitboxes : ") + (renderHitboxes ? "ON" : "OFF"));
            menuOptions.push_back("Reload entities.json");
            menuOptions.push_back(std::string("Map viewer mode : ") + (mapViewer ? "ON" : "OFF"));
            menuOptions.push_back("Reload room");
            menuOptions.push_back("Reload map");
            menuOptions.push_back(std::string("Debug info : ") + (showDebugInfo ? "ON" : "OFF"));
            menuOptions.push_back(std::string("TAS Tool : ") + (tasToolEnabled ? "ON" : "OFF"));
        } else if (menu == "death") {
            menuOptions.clear();
            menuOptions.push_back("Respawn");
            menuOptions.push_back("Quit");
        }

    } else
        if (inputList["menu"] && inputTime["menu"] == 0.0) {
            isPaused = true;
            menu = "main";
            selectedOption = 0;
        }
}

void Game::updateNPCs()
{
    for (std::vector<NPC*>::iterator j = NPCs.begin(); j != NPCs.end(); j++) {
        if (Entity::checkCollision(s, s->getBox(), *j, (*j)->getBox())) {
            if (inputList["interact"] && inputTime["interact"] == 0.0) {
                if ((*j)->getNpcType() == "Talking") {
                    // Set maxInteractions here because npc.cpp cannot include Physics.h
                    if ((*j)->getMaxInteractions() == 0)
                        (*j)->setMaxInteractions(stringsJson[language]["dialogues"][(*j)->getName()].size());

                    bool increased = false;
                    if (!currentDialogue.isNull())
                        if (currentDialogue.getText().size() - 1 > currentDialogue.getTextAdvancement()) {
                            currentDialogue.setTextAdvancement(currentDialogue.getTextAdvancement() + 1);
                            increased = true;
                        }
                    if (!increased) {
                        // Set new Dialogue
                        if (stringsJson[language]["dialogues"][(*j)->getName()][std::min((*j)->getTimesInteracted(), (*j)->getMaxInteractions() - 1)]["talking"].is_null())
                          currentDialogue = Dialogue(
                              stringsJson[language]["dialogues"][(*j)->getName()][std::min((*j)->getTimesInteracted(), (*j)->getMaxInteractions() - 1)]["text"], *j);
                        else
                            currentDialogue = Dialogue(stringsJson[language]["dialogues"][(*j)->getName()][std::min((*j)->getTimesInteracted(),(*j)->getMaxInteractions())]["text"],
                                    *j, stringsJson[language]["dialogues"][(*j)->getName()][std::min((*j)->getTimesInteracted(),(*j)->getMaxInteractions())]["talking"]);
                    }
                } else if ((*j)->getNpcType() == "Savepoint") {
                    updateProgress();
                    Savepoint* sp = static_cast<Savepoint*>(*j);
                    currentProgress.setSavepointID(sp->getSavepointID());
                    currentProgress.setRoomID(sp->getRoomId());
                    currentProgress.setSaveMapName(currentMap.getName());
                    lastCheckpoint = currentProgress;
                    lastSave = currentProgress;
                    lastSave.save(assetsPath + "/saves/" + saveFile + ".json");
                    currentDialogue = Dialogue(stringsJson[language]["ui"]["savepoint"]["Saved"], *j);
                }
                // Don't forget to increment the Dialogue advancement
                (*j)->setTimesInteracted((*j)->getTimesInteracted() + 1);
            }
        } else if (currentDialogue.getTalking() != nullptr)
            if (!Entity::checkCollision(s, s->getBox(), currentDialogue.getTalking(), currentDialogue.getTalking()->getBox()))
                // Reset Dialogue if the player went too far away
                currentDialogue = Dialogue();
    }
}

void Game::updateCamera()
{
    nlohmann::json mapJson = (*currentMap.getJson())["rooms"][currentMap.getCurrentRoomId()];

    int roomS_x = mapJson["position"][0];
    int roomS_y = mapJson["position"][1];
    int roomE_x = mapJson["size"][0];
    int roomE_y = mapJson["size"][1];
    roomE_x += roomS_x;
    roomE_y += roomS_y;

    int cam_dist_x = s->getX() + static_cast<int>(Entity::values["general"]["camera_rx"]) - camera.x();
    int cam_dist_y = s->getY() + static_cast<int>(Entity::values["general"]["camera_ry"]) - camera.y();
    camera.setX(camera.x() + cam_dist_x);
    camera.setY(camera.y() + cam_dist_y);
    if (camera.x() < roomS_x)
        camera.setX(roomS_x);
    else if (camera.x() + cameraSize.first > roomE_x)
        camera.setX(roomE_x - cameraSize.first);
    if (camera.y() < roomS_y)
        camera.setY(roomS_y);
    else if (camera.y() + cameraSize.second > roomE_y)
        camera.setY(roomE_y - cameraSize.second);
}

void Game::updateProgress()
{
    currentProgress.setSamosHealth(s->getHealth());
    currentProgress.setSamosMaxHealth(s->getMaxHealth());
    currentProgress.setSamosGrenades(s->getGrenadeCount());
    currentProgress.setSamosMaxGrenades(s->getMaxGrenadeCount());
    currentProgress.setSamosMissiles(s->getMissileCount());
    currentProgress.setSamosMaxMissiles(s->getMaxMissileCount());
    currentProgress.setPlayTime(timeAtLaunch + updateCount / updateRate);
}

void Game::updateInventory()
{
    if (inInventory || inMap) {
        if ((inputList["map"] && inputTime["map"] == 0.0) || (inputList["menu"] && inputTime["menu"] == 0.0)) {
            inputTime["menu"] += 1 / Physics::frameRate;
            inMap = false;
            inInventory = false;
        }
        if (inMap) {
            nlohmann::json mapJson = *currentMap.getJson();
            std::vector<std::string> tRooms = currentProgress.getRoomsDiscovered()[currentMap.getName()];
            nlohmann::json currentRoom = mapJson["rooms"][currentMap.getCurrentRoomId()];
            int mapScaleDown = Entity::values["general"]["mapScaleDown"].get<int>();

            int x_min = currentRoom["position"][0].get<int>();
            int x_max = currentRoom["position"][0].get<int>() + currentRoom["size"][0].get<int>();
            int y_min = currentRoom["position"][1].get<int>();
            int y_max = currentRoom["position"][1].get<int>() + currentRoom["size"][1].get<int>();

            for (std::string i : tRooms) {
                nlohmann::json room = mapJson["rooms"][i];
                if (room["position"][0].get<int>() + room["size"][0].get<int>() > x_max)
                    x_max = room["position"][0].get<int>() + room["size"][0].get<int>();
                if (room["position"][0].get<int>() < x_min)
                    x_min = room["position"][0].get<int>();
                if (room["position"][1].get<int>() + room["size"][1].get<int>() > y_max)
                    y_max = room["position"][1].get<int>() + room["size"][1].get<int>();
                if (room["position"][1].get<int>() < y_min)
                    y_min = room["position"][0].get<int>();
            }

            x_min += cameraSize.first / 10;
            x_max -= cameraSize.first / 10;
            y_min += cameraSize.first / 10;
            y_max -= cameraSize.first / 10;

            if (inputList["down"] && !inputList["up"] && inputList["left"] && !inputList["right"]
                       && (x_min < mapCameraPosition.x() + cameraSize.first * mapScaleDown) && (y_max > mapCameraPosition.y())) {
                // Down-Left
                mapCameraPosition.setY(mapCameraPosition.y() + 0.707 * mapCameraSpeed / Physics::frameRate);
                mapCameraPosition.setX(mapCameraPosition.x() - 0.707 * mapCameraSpeed / Physics::frameRate);
            } else if (!inputList["down"] && inputList["up"] && inputList["left"] && !inputList["right"]
                       && (x_min < mapCameraPosition.x() + cameraSize.first * mapScaleDown) && (y_min < mapCameraPosition.y() + cameraSize.second * mapScaleDown)) {
                // Up-Left
                mapCameraPosition.setY(mapCameraPosition.y() - 0.707 * mapCameraSpeed / Physics::frameRate);
                mapCameraPosition.setX(mapCameraPosition.x() - 0.707 * mapCameraSpeed / Physics::frameRate);
            } else if (inputList["down"] && !inputList["up"] && !inputList["left"] && inputList["right"]
                       && (x_max > mapCameraPosition.x()) && (y_max > mapCameraPosition.y())) {
                // Down-Right
                mapCameraPosition.setY(mapCameraPosition.y() + 0.707 * mapCameraSpeed / Physics::frameRate);
                mapCameraPosition.setX(mapCameraPosition.x() + 0.707 * mapCameraSpeed / Physics::frameRate);
            } else if (!inputList["down"] && inputList["up"] && !inputList["left"] && inputList["right"]
                       && (x_max > mapCameraPosition.x()) && (y_min < mapCameraPosition.y() + cameraSize.second * mapScaleDown)) {
                // Up-Right
                mapCameraPosition.setY(mapCameraPosition.y() - 0.707 * mapCameraSpeed / Physics::frameRate);
                mapCameraPosition.setX(mapCameraPosition.x() + 0.707 * mapCameraSpeed / Physics::frameRate);
            } else if (inputList["down"] && !inputList["up"]
                       && (y_max > mapCameraPosition.y())) {
                // Down
                mapCameraPosition.setY(mapCameraPosition.y() + mapCameraSpeed / Physics::frameRate);
            } else if (!inputList["down"] && inputList["up"]
                       && (y_min < mapCameraPosition.y() + cameraSize.second * mapScaleDown)) {
                // Up
                mapCameraPosition.setY(mapCameraPosition.y() - mapCameraSpeed / Physics::frameRate);
            } else if (inputList["left"] && !inputList["right"]
                       && (x_min < mapCameraPosition.x() + cameraSize.first * mapScaleDown)) {
                // Left
                mapCameraPosition.setX(mapCameraPosition.x() - mapCameraSpeed / Physics::frameRate);
            } else if (!inputList["left"] && inputList["right"]
                       && (x_max > mapCameraPosition.x())) {
                // Right
                mapCameraPosition.setX(mapCameraPosition.x() + mapCameraSpeed / Physics::frameRate);
            }
        }

    } else
        if (inputList["map"] && inputTime["map"] == 0.0) {
            inMap = true;
            mapCameraPosition.setX(camera.x() + cameraSize.first * (1 - static_cast<int>(Entity::values["general"]["mapScaleDown"])) / 2);
            mapCameraPosition.setY(camera.y() + cameraSize.second * (1 - static_cast<int>(Entity::values["general"]["mapScaleDown"])) / 2);
        }
}

std::pair<int, int> Game::loadRespawnPosition(Save respawnSave, Map respawnMap)
{
    nlohmann::json mapJson = (*respawnMap.getJson())["rooms"][respawnSave.getRoomID()];
    nlohmann::json sJson = Entity::values["names"]["Samos"];
    nlohmann::json spJson = Entity::values["names"]["Savepoint"];

    int x = static_cast<int>(mapJson["position"][0]) + static_cast<int>(spJson["offset_x"]) + static_cast<int>(spJson["width"]) / 2 - static_cast<int>(sJson["offset_x"]) - static_cast<int>(sJson["width"]) / 2;
    int y = static_cast<int>(mapJson["position"][1]) + static_cast<int>(spJson["offset_y"]) + static_cast<int>(spJson["height"]) - static_cast<int>(sJson["offset_y"]) - static_cast<int>(sJson["height"]);

    for (nlohmann::json sp : mapJson["content"]["NPC"]["Savepoint"]) {
        if (static_cast<int>(sp["spID"]) == respawnSave.getSavepointID()) {
            x += static_cast<int>(sp["x"]);
            y += static_cast<int>(sp["y"]);
            break;
        }
    }

    return std::pair<int, int>(x, y);
}

void Game::updateMapViewer()
{
    if (inputList["enter"] && inputTime["enter"] == 0.0) {
        std::string mapId = currentMap.getCurrentRoomId();
        currentMap = Map::loadMap(currentMap.getName(), assetsPath);
        currentMap.setCurrentRoomId(mapId);
        clearEntities("Samos");
        addEntities(currentMap.loadRoom());
    }

    if (inputList["down"] && !inputList["up"]) {
        camera.setY(camera.y() + mapViewerCameraSpeed / Physics::frameRate);
    } else if (!inputList["down"] && inputList["up"]) {
        camera.setY(camera.y() - mapViewerCameraSpeed / Physics::frameRate);
    }
    if (inputList["left"] && !inputList["right"]) {
        camera.setX(camera.x() - mapViewerCameraSpeed / Physics::frameRate);
    } else if (!inputList["left"] && inputList["right"]) {
        camera.setX(camera.x() + mapViewerCameraSpeed / Physics::frameRate);
    }

}

void Game::addEntity(Entity *entity)
{
    entities.push_back(entity);
    if (entity->getEntType() == "Terrain") {
        Terrain* t = static_cast<Terrain*>(entity);
        terrains.push_back(t);
    } else if (entity->getEntType() == "Projectile") {
        Projectile* p = static_cast<Projectile*>(entity);
        projectiles.push_back(p);
    } else if (entity->getEntType() == "DynamicObj") {
        DynamicObj* d = static_cast<DynamicObj*>(entity);
        dynamicObjs.push_back(d);
    } else if (entity->getEntType() == "Monster") {
        Monster* m = static_cast<Monster*>(entity);
        monsters.push_back(m);
    } else if (entity->getEntType() == "Area") {
        Area* a = static_cast<Area*>(entity);
        areas.push_back(a);
    } else if (entity->getEntType() == "NPC") {
        NPC* n = static_cast<NPC*>(entity);
        NPCs.push_back(n);
    } else if (entity->getEntType() == "Samos") {
        s = static_cast<Samos*>(entity);
    }
}

void Game::addEntities(std::vector<Entity*> es)
{
    for (std::vector<Entity*>::iterator entity = es.begin(); entity != es.end(); entity++) {
        entities.push_back(*entity);
        if ((*entity)->getEntType() == "Terrain") {
            Terrain* t = static_cast<Terrain*>(*entity);
            terrains.push_back(t);
        } else if ((*entity)->getEntType() == "Projectile") {
            Projectile* p = static_cast<Projectile*>(*entity);
            projectiles.push_back(p);
        } else if ((*entity)->getEntType() == "DynamicObj") {
            DynamicObj* d = static_cast<DynamicObj*>(*entity);
            dynamicObjs.push_back(d);
        } else if ((*entity)->getEntType() == "Monster") {
            Monster* m = static_cast<Monster*>(*entity);
            monsters.push_back(m);
        } else if ((*entity)->getEntType() == "Area") {
            Area* a = static_cast<Area*>(*entity);
            areas.push_back(a);
        } else if ((*entity)->getEntType() == "NPC") {
            NPC* n = static_cast<NPC*>(*entity);
            NPCs.push_back(n);
        } else if ((*entity)->getEntType() == "Samos") {
            s = static_cast<Samos*>(*entity);
        }
    }
}

void Game::clearEntities(std::string excludedType, bool deleteEntities)
{
    std::vector<Entity*> nextRen;
    for (std::vector<Entity*>::iterator ent = entities.begin(); ent != entities.end(); ent ++)
        if ((*ent)->getEntType() != excludedType) {
            if (deleteEntities)
                delete *ent;
        } else
            nextRen.push_back(*ent);

    terrains = {};
    monsters = {};
    NPCs = {};
    projectiles = {};
    areas = {};
    dynamicObjs = {};
    entities = {};
    addEntities(nextRen);
}

void Game::removeOtherRoomsEntities()
{
    std::vector<Entity*> newentities;
    std::vector<Entity*> toDelete;
    for (std::vector<Entity*>::iterator ent = entities.begin(); ent != entities.end(); ent++)
        if ((*ent)->getRoomId() == currentMap.getCurrentRoomId())
            newentities.push_back(*ent);
        else
            toDelete.push_back(*ent);
    clearEntities("", false);
    addEntities(newentities);
}

void Game::removeEntities(std::vector<Entity *> es)
{
    std::vector<Entity*> newRen;
    for (std::vector<Entity*>::iterator j = entities.begin(); j != entities.end(); j++) {
        bool td = false;
        for (std::vector<Entity*>::iterator i = es.begin(); i != es.end(); i++) {
            if (*j == *i) {
                td = true;
                break;
            }
        }
        if (!td)
            newRen.push_back(*j);
    }

    monsters = {};
    terrains = {};
    s = nullptr;
    dynamicObjs = {};
    areas = {};
    projectiles = {};
    NPCs = {};
    entities = {};
    addEntities(newRen);
    for (std::vector<Entity*>::iterator i = es.begin(); i != es.end(); i++)
        delete *i;
}

void Game::updateAnimations()
{
    unsigned long long uC = updateCount;
    for (std::vector<Entity*>::iterator ent = entities.begin(); ent != entities.end(); ent++) {
        std::string state = (*ent)->getState();
        std::string facing = (*ent)->getFacing();
        // Every 'refreshRate' frames
        if (!Entity::values["textures"][(*ent)->getName()][state]["refreshRate"].is_null())
            if (uC % static_cast<int>(Entity::values["textures"][(*ent)->getName()][state]["refreshRate"]) == 0)
                // If the animation index still exists
                if ((*ent)->getCurrentAnimation().size() > (*ent)->getFrame())
                    // Increment the animation index
                    (*ent)->setFrame((*ent)->getFrame() + 1);

        // If the (*ent) state is different from the last frame
        if (state != (*ent)->getLastFrameState()
                || facing != (*ent)->getLastFrameFacing()) {
            // Update the QImage array representing the animation
            (*ent)->setCurrentAnimation((*ent)->updateAnimation());
            // If the animation should reset the next one
            if (!Entity::values["textures"]
                    [Entity::values["names"][(*ent)->getName()]["texture"]]
                    [(*ent)->getLastFrameState()]
                    ["dontReset"])
                // Because the animation changed, reset it
                (*ent)->setFrame(0);
            else
                // Else, make sure not to end up with a too high index
                if ((*ent)->getFrame() >= (*ent)->getCurrentAnimation().size())
                    (*ent)->setFrame(0);
        }

        // Every 'refreshRate' frames
        if (!Entity::values["textures"][(*ent)->getName()][state]["refreshRate"].is_null())
            if (uC % static_cast<int>(Entity::values["textures"][(*ent)->getName()][state]["refreshRate"]) == 0)
                // If the animation has to loop
                if (!Entity::values["textures"][(*ent)->getName()][state]["loop"].is_null()) {
                    if (Entity::values["textures"][(*ent)->getName()][state]["loop"]) {
                        // If the animation index still exists
                        if ((*ent)->getCurrentAnimation().size() - 1 < (*ent)->getFrame())
                            // Reset animation
                            (*ent)->setFrame(0);
                    } else
                        // If the animation index still exists
                        if ((*ent)->getCurrentAnimation().size() - 1 < (*ent)->getFrame())
                            // If the animation doesn't loop, make sure it stays on its last frame
                            (*ent)->setFrame((*ent)->getFrame() - 1);
                }

        // Update the texture with the animation index
        (*ent)->updateTexture();

        // Make sure to update these values for the next frame
        (*ent)->setLastFrameState(state);
        (*ent)->setLastFrameFacing(facing);
    }
}

std::vector<Entity *> *Game::getEntities()
{
    return &entities;
}

void Game::setEntities(const std::vector<Entity *> &newRendering)
{
    entities = newRendering;
}

int Game::getSelectedOption() const
{
    return selectedOption;
}

void Game::setSelectedOption(int newSelectedOption)
{
    selectedOption = newSelectedOption;
}

const std::string &Game::getMenu() const
{
    return menu;
}

void Game::setMenu(const std::string &newMenu)
{
    menu = newMenu;
}

const std::vector<std::string> &Game::getMenuOptions() const
{
    return menuOptions;
}

void Game::setMenuOptions(const std::vector<std::string> &newMenuOptions)
{
    menuOptions = newMenuOptions;
}

QPoint Game::getCamera()
{
    return camera;
}

void Game::setCamera(QPoint newCamera)
{
    camera = newCamera;
}

std::vector<Terrain *>* Game::getTerrains()
{
    return &terrains;
}

void Game::setTerrains(const std::vector<Terrain *> &newTerrains)
{
    terrains = newTerrains;
}

std::vector<Monster *>* Game::getMonsters()
{
    return &monsters;
}

void Game::setMonsters(const std::vector<Monster *> &newMonsters)
{
    monsters = newMonsters;
}

std::vector<NPC *>* Game::getNPCs()
{
    return &NPCs;
}

void Game::setNPCs(const std::vector<NPC *> &newNPCs)
{
    NPCs = newNPCs;
}

std::vector<Projectile *>* Game::getProjectiles()
{
    return &projectiles;
}

void Game::setProjectiles(const std::vector<Projectile *> &newProjectiles)
{
    projectiles = newProjectiles;
}

std::vector<Area *>* Game::getAreas()
{
    return &areas;
}

void Game::setAreas(const std::vector<Area *> &newAreas)
{
    areas = newAreas;
}

std::vector<DynamicObj *>* Game::getDynamicObjs()
{
    return &dynamicObjs;
}

void Game::setDynamicObjs(const std::vector<DynamicObj *> &newDynamicObjs)
{
    dynamicObjs = newDynamicObjs;
}

Samos *Game::getS()
{
    return s;
}

void Game::setS(Samos *newS)
{
    s = newS;
}

bool Game::getMapViewer() const
{
    return mapViewer;
}

void Game::setMapViewer(bool newMapViewer)
{
    mapViewer = newMapViewer;
}

bool Game::getRunning() const
{
    return running;
}

void Game::setRunning(bool newRunning)
{
    running = newRunning;
}

double Game::getUpdateRate() const
{
    return updateRate;
}

void Game::setUpdateRate(double newUpdateRate)
{
    updateRate = newUpdateRate;
}

double Game::getGameSpeed() const
{
    return gameSpeed;
}

void Game::setGameSpeed(double newGameSpeed)
{
    gameSpeed = newGameSpeed;
}

unsigned long long Game::getFrameCount() const
{
    return frameCount;
}

void Game::setFrameCount(unsigned long long newFrameCount)
{
    frameCount = newFrameCount;
}

unsigned long long Game::getUpdateCount() const
{
    return updateCount;
}

void Game::setUpdateCount(unsigned long long newUpdateCount)
{
    updateCount = newUpdateCount;
}

nlohmann::json &Game::getKeyCodes()
{
    return keyCodes;
}

void Game::setKeyCodes(const nlohmann::json &newKeyCodes)
{
    keyCodes = newKeyCodes;
}

Map &Game::getCurrentMap()
{
    return currentMap;
}

void Game::setCurrentMap(const Map &newCurrentMap)
{
    currentMap = newCurrentMap;
}

const nlohmann::json &Game::getStringsJson() const
{
    return stringsJson;
}

void Game::setStringsJson(const nlohmann::json &newStringsJson)
{
    stringsJson = newStringsJson;
}

std::map<std::string, bool>* Game::getInputList()
{
    return &inputList;
}

void Game::setInputList(const std::map<std::string, bool> &newInputList)
{
    inputList = newInputList;
}

std::map<std::string, double>* Game::getInputTime()
{
    return &inputTime;
}

void Game::setInputTime(const std::map<std::string, double> &newInputTime)
{
    inputTime = newInputTime;
}

std::chrono::system_clock::time_point Game::getLastFpsShown() const
{
    return lastFpsShown;
}

void Game::setLastFpsShown(std::chrono::system_clock::time_point newLastFpsShown)
{
    lastFpsShown = newLastFpsShown;
}

std::chrono::system_clock::time_point Game::getLastFrameTime() const
{
    return lastFrameTime;
}

void Game::setLastFrameTime(std::chrono::system_clock::time_point newLastFrameTime)
{
    lastFrameTime = newLastFrameTime;
}

unsigned int Game::getFps() const
{
    return fps;
}

void Game::setFps(unsigned int newFps)
{
    fps = newFps;
}

unsigned int Game::getShowFpsUpdateRate() const
{
    return showFpsUpdateRate;
}

void Game::setShowFpsUpdateRate(unsigned int newShowFpsUpdateRate)
{
    showFpsUpdateRate = newShowFpsUpdateRate;
}

bool Game::getIsPaused() const
{
    return isPaused;
}

void Game::setIsPaused(bool newIsPaused)
{
    isPaused = newIsPaused;
}

const std::pair<int, int> &Game::getResolution() const
{
    return resolution;
}

void Game::setResolution(const std::pair<int, int> &newResolution)
{
    resolution = newResolution;
}

double Game::getMapViewerCameraSpeed() const
{
    return mapViewerCameraSpeed;
}

void Game::setMapViewerCameraSpeed(double newMapViewerCameraSpeed)
{
    mapViewerCameraSpeed = newMapViewerCameraSpeed;
}

const std::string &Game::getAssetsPath() const
{
    return assetsPath;
}

const std::string &Game::getDoorTransition() const
{
    return doorTransition;
}

void Game::setDoorTransition(const std::string &newDoorTransition)
{
    doorTransition = newDoorTransition;
}

const nlohmann::json &Game::getParams() const
{
    return params;
}

void Game::setParams(const nlohmann::json &newParams)
{
    params = newParams;
}

const std::string &Game::getLanguage() const
{
    return language;
}

void Game::setLanguage(const std::string &newLanguage)
{
    language = newLanguage;
}

Dialogue &Game::getCurrentDialogue()
{
    return currentDialogue;
}

void Game::setCurrentDialogue(const Dialogue &newCurrentDialogue)
{
    currentDialogue = newCurrentDialogue;
}

double Game::getMenuArrowsTime() const
{
    return menuArrowsTime;
}

void Game::setMenuArrowsTime(double newMenuArrowsTime)
{
    menuArrowsTime = newMenuArrowsTime;
}

bool Game::getRenderHitboxes() const
{
    return renderHitboxes;
}

void Game::setRenderHitboxes(bool newRenderHitboxes)
{
    renderHitboxes = newRenderHitboxes;
}

bool Game::getShowFps() const
{
    return showFps;
}

void Game::setShowFps(bool newShowFps)
{
    showFps = newShowFps;
}

bool Game::getFullscreen() const
{
    return fullscreen;
}

void Game::setFullscreen(bool newFullscreen)
{
    fullscreen = newFullscreen;
}

Save &Game::getCurrentProgress()
{
    return currentProgress;
}

void Game::setCurrentProgress(const Save &newCurrentProgress)
{
    currentProgress = newCurrentProgress;
}

const Save &Game::getLastSave() const
{
    return lastSave;
}

void Game::setLastSave(const Save &newLastSave)
{
    lastSave = newLastSave;
}

const Save &Game::getLastCheckpoint() const
{
    return lastCheckpoint;
}

void Game::setLastCheckpoint(const Save &newLastCheckpoint)
{
    lastCheckpoint = newLastCheckpoint;
}

bool Game::getInInventory() const
{
    return inInventory;
}

void Game::setInInventory(bool newInInventory)
{
    inInventory = newInInventory;
}

bool Game::getInMap() const
{
    return inMap;
}

void Game::setInMap(bool newInMap)
{
    inMap = newInMap;
}

QPoint Game::getMapCameraPosition() const
{
    return mapCameraPosition;
}

void Game::setMapCameraPosition(QPoint newMapCameraPosition)
{
    mapCameraPosition = newMapCameraPosition;
}

double Game::getMapCameraSpeed() const
{
    return mapCameraSpeed;
}

void Game::setMapCameraSpeed(double newMapCameraSpeed)
{
    mapCameraSpeed = newMapCameraSpeed;
}

const std::pair<int, int> &Game::getCameraSize() const
{
    return cameraSize;
}

void Game::setCameraSize(const std::pair<int, int> &newCameraSize)
{
    cameraSize = newCameraSize;
}

bool Game::getShowHUD() const
{
    return showHUD;
}

void Game::setShowHUD(bool newShowHUD)
{
    showHUD = newShowHUD;
}

bool Game::getShowDebugInfo() const
{
    return showDebugInfo;
}

void Game::setShowDebugInfo(bool newShowDebugInfo)
{
    showDebugInfo = newShowDebugInfo;
}

bool Game::getDebugEnabled() const
{
    return debugEnabled;
}

void Game::setDebugEnabled(bool newDebugEnabled)
{
    debugEnabled = newDebugEnabled;
}

bool Game::getFrameAdvance() const
{
    return frameAdvance;
}

void Game::setFrameAdvance(bool newFrameAdvance)
{
    frameAdvance = newFrameAdvance;
}

bool Game::getTasToolEnabled() const
{
    return tasToolEnabled;
}

void Game::setTasToolEnabled(bool newTasToolEnabled)
{
    tasToolEnabled = newTasToolEnabled;
}

bool Game::getTas() const
{
    return tas;
}

void Game::setTas(bool newTas)
{
    tas = newTas;
}

const nlohmann::json &Game::getWindowsKeyCodes() const
{
    return windowsKeyCodes;
}

void Game::setWindowsKeyCodes(const nlohmann::json &newWindowsKeyCodes)
{
    windowsKeyCodes = newWindowsKeyCodes;
}

unsigned long long Game::getCurrentInstructionFrames() const
{
    return currentInstructionFrames;
}

void Game::setCurrentInstructionFrames(unsigned long long newCurrentInstructionFrames)
{
    currentInstructionFrames = newCurrentInstructionFrames;
}

int Game::getLine() const
{
    return line;
}

void Game::setLine(int newLine)
{
    line = newLine;
}

unsigned long long Game::getLastInstructionFrames() const
{
    return lastInstructionFrames;
}

void Game::setLastInstructionFrames(unsigned long long newLastInstructionFrames)
{
    lastInstructionFrames = newLastInstructionFrames;
}

int Game::getLinesSkipped() const
{
    return linesSkipped;
}

void Game::setLinesSkipped(int newLinesSkipped)
{
    linesSkipped = newLinesSkipped;
}

bool Game::getUltraFastForward() const
{
    return ultraFastForward;
}

void Game::setUltraFastForward(bool newUltraFastForward)
{
    ultraFastForward = newUltraFastForward;
}

std::map<std::string, std::vector<Entity *> *> &Game::getRoomEntities()
{
    return roomEntities;
}

void Game::setRoomEntities(const std::map<std::string, std::vector<Entity *> *> &newRoomEntities)
{
    roomEntities = newRoomEntities;
}

std::vector<std::string> *Game::getRoomsToUnload()
{
    return &roomsToUnload;
}

void Game::setRoomsToUnload(std::vector<std::string> &newRoomsToUnload)
{
    roomsToUnload = newRoomsToUnload;
}

std::vector<std::string> *Game::getRoomsToLoad()
{
    return &roomsToLoad;
}

void Game::setRoomsToLoad(std::vector<std::string> &newRoomsToLoad)
{
    roomsToLoad = newRoomsToLoad;
}
