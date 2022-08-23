#include "editorpreview.h"
#include "editorwindow.h"

#include <QApplication>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTreeView>
#include <fstream>
#include <set>

#define JSON_DIAGNOSTICS 1 // Json extended error messages
#include "../ATOTAM/nlohmann/json.hpp"
#include "../ATOTAM/map.h"

QStandardItemModel* setupObjectList() {
    QStandardItemModel* model = new QStandardItemModel;
    nlohmann::json j = Entity::values["names"];
    std::vector<Entity*> entities;
    std::set<std::string> entTypes;

    // Entity creation
    for (auto j : j.items())
        if (!j.value()["type"].is_null()) {
            std::string name = j.key();
            std::string type = j.value()["type"];
            if (type == "Projectile" || type == "Samos")
                continue;
            entTypes.emplace(type);
            Entity* e = nullptr;

            // Entity instantiation
            if (type == "Terrain")
                e = new Terrain(0, 0, name);
            else if (type == "Area") {
                if (name.substr(name.size() - 4, 4) == "Door")
                    e = new Door(0, 0, name);
                else
                    e = new Area(0, 0, name);
            } else if (type == "NPC")
                e = new NPC(0, 0, "Right", name);
            else if (type == "Monster")
                e = new Monster(0, 0, "Right", name);

            e->setState(j.value()["defaultState"]);
            e->setCurrentAnimation(e->updateAnimation());
            e->updateTexture();

            entities.push_back(e);
        }

    for (const std::string &t : entTypes) {
        QStandardItem* type = new QStandardItem;

        type->setDragEnabled(false);
        type->setDropEnabled(false);
        type->setText(QString::fromStdString(t));
        type->setCheckable(false);
        type->setSelectable(false);
        type->setEditable(false);

        model->appendRow(type);
        for (Entity* e : entities) {
            if (e->getEntType() != t)
                continue;
            QStandardItem* item = new QStandardItem;

            item->setDragEnabled(true);
            item->setDropEnabled(true);
            item->setText(QString::fromStdString(e->getName()));
            item->setCheckable(false);
            item->setEditable(false);
            item->setIcon(QIcon(QPixmap::fromImage(*e->getTexture(), Qt::ColorOnly)));

            type->appendRow(item);
        }
    }

    return model;
}

void setupEditorWindow(nlohmann::json editorJson, std::string assetsPath)
{
    // Map preview
    Map editedMap = Map::loadMap(editorJson["lastLaunch"]["map"]["id"], assetsPath);
    std::ifstream file(assetsPath + "/params.json");
    nlohmann::json params;
    file >> params;
    EditorPreview* preview = new EditorPreview(&editedMap, new QImage(QString::fromStdString(assetsPath + "/textures/error.png"))
                                               , new QImage(QString::fromStdString(assetsPath + "/textures/empty.png")), 2
                                               , editorJson, params["frameRate"]);
    preview->setAssetsPath(assetsPath);

    // Window
    EditorWindow* editorWindow = new EditorWindow(preview);
    editorWindow->setWindowTitle("ATOTAM Map Editor");
    editorWindow->setMinimumSize(QSize(editorJson["values"]["minimumWindowSize"][0], editorJson["values"]["minimumWindowSize"][1]));
    // Set window screen
    /*QList<QScreen*> screens = m_qApp->screens();
    if (screens.size() > 1) {
        editorWindow->setScreen(screens[1]);
        editorWindow->setGeometry(screens[1]->geometry().center().x() - static_cast<int>(editorJson["lastLaunch"]["window"]["size"][0])/2,
                screens[1]->geometry().center().y() - static_cast<int>(editorJson["lastLaunch"]["window"]["size"][1])/2, 800, 600);
    }*/

    // Widgets inside the window
    //QTreeView* objectList = new QTreeView;
    //objectList->setHeaderHidden(true);
    //objectList->setFixedSize(200, 1080);
    //objectList->setModel(setupObjectList());

    // Align widgets
    QSplitter* splitter = new QSplitter;
    //splitter->addWidget(objectList);
    splitter->addWidget(preview);

    editorWindow->setCentralWidget(splitter);
    if (editorJson["lastLaunch"]["window"]["maximized"])
        editorWindow->showMaximized();
    else
        editorWindow->show();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::string assetsPath = "../ATOTAM/assets";
    std::ifstream file(assetsPath + "/map_editor.json");
    nlohmann::json editorJson;
    file >> editorJson;
    Entity::values = Entity::loadValues(assetsPath);
    setupEditorWindow(editorJson, assetsPath);

    return a.exec();
}
