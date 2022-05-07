#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QPainter>
#include <vector>
#include <iostream>
#include <windows.h>

#include "Entities/entity.h"
#include "Entities/living.h"
#include "Entities/samos.h"

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
    static int renderingMultiplier; // Textures are rendered with their size being multiplied by this value
    static QImage errorTexture;
    static bool running;
    static double frameRate; //fps
    static double gameSpeed;
    static unsigned long long frameCount;
    static double gravity; //p.s^-2
    static nlohmann::json keyCodes;
    static nlohmann::json loadKeyCodes();
    static void loadGeneral();
    static std::map<std::string, bool> inputList;
    static void getInputs();
    static void updateSamos(Samos* s);

    void closeEvent(QCloseEvent *event);
    virtual void paintEvent(QPaintEvent*);
    void addRenderable(Entity *entity);
    void clearRendering();
    void updatePhysics();
    void updateAnimations();

    const std::vector<Entity *> &getRendering() const;
    void setRendering(const std::vector<Entity *> &newRendering);
private:
    Ui::MainWindow *ui;
    QApplication *m_qApp;
    std::vector<Entity*> rendering;
};
#endif // MAINWINDOW_H
