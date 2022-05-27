#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QPainter>
#include <vector>
#include <iostream>
#include <windows.h>
#include <chrono>

#include "Entities/entity.h"
#include "Entities/living.h"
#include "Entities/samos.h"
#include "map.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QApplication *app);
    ~MainWindow();

    static bool renderHitboxes; // Render hitboxes as rectangles
    static bool mapViewer; // Whether the game the game was launched as a map viewer
    static int renderingMultiplier; // Textures are rendered with their size being multiplied by this value
    static QImage errorTexture;
    static QImage emptyTexture;
    static bool running;
    static double frameRate; //fps
    static double updateRate; // How many game updates in one second
    static double gameSpeed;
    static unsigned long long frameCount;
    static unsigned long long updateCount;
    static double gravity; //p.s^-2
    static nlohmann::json keyCodes;
    static Map currentMap;
    static nlohmann::json loadKeyCodes();
    static void loadGeneral();
    static void handleCollision(Entity* obj1, Entity* obj2);
    static std::map<std::string, bool> inputList;
    static std::chrono::system_clock::time_point lastFpsShown; // Time of the last frame in which the shown fps were updated
    static std::chrono::system_clock::time_point lastFrameTime; // Time of the last frame in which the shown fps were updated
    static unsigned int fps; // Fps count when 'lastFpsShown' was updated
    static bool showFps; // Whether to show fps in-game
    static unsigned int showFpsUpdateRate; // Time (in ns) between each fps update
    static bool showUI;
    static bool isPaused;
    static bool fullscreen;
    static std::pair<int,int> resolution;
    void closeEvent(QCloseEvent *event);
    virtual void paintEvent(QPaintEvent*);
    void addRenderable(Entity *entity);
    void clearRendering();
    void updatePhysics();
    void updateAnimations();
    void updateSamos(Samos* s);
    void getInputs();
    bool updateProjectile(Projectile* p);
    void updateMenu();
    static nlohmann::json params;
    static nlohmann::json loadParams();

    const std::vector<Entity *> &getRendering() const;
    void setRendering(const std::vector<Entity *> &newRendering);
    int getSelectedOption() const;
    void setSelectedOption(int newSelectedOption);

    const std::string &getMenu() const;
    void setMenu(const std::string &newMenu);

    const std::vector<std::string> &getMenuOptions() const;
    void setMenuOptions(const std::vector<std::string> &newMenuOptions);

private:
    Ui::MainWindow *ui;
    QApplication *m_qApp;
    std::vector<Entity*> rendering;
    int selectedOption = 0;
    std::string menu;
    std::vector<std::string> menuOptions;
    double menuArrowsTime = 0.0;
    bool holdingMenu = false;
};
#endif // MAINWINDOW_H
