#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

#include <Entities/living.h>

bool MainWindow::running = true;
double MainWindow::frameRate = 60.0;
double MainWindow::gravity = 500.0;

MainWindow::MainWindow(QApplication *app)
    : ui(new Ui::MainWindow)
    , m_qApp(app)
    //, rendering()
{
    ui->setupUi(this);
    // Set size of the window
    setFixedSize(1000, 500);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    running = false;
}

void MainWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    for (Entity *entity : rendering) {
        painter.drawImage(QRect(entity->getX(), entity->getY(), entity->getTexture()->width() * renderingMultiplier, entity->getTexture()->height() * renderingMultiplier),
                          *entity->getTexture());
    }
    painter.end();
}

void MainWindow::addRenderable(Entity *entity)
{
    rendering.push_back(entity);
}

void MainWindow::clearRendering()
{
    rendering = {};
}

void MainWindow::updatePhysics()
{
    for (Entity* ent : rendering) {
        if (ent->getIsAffectedByGravity())
            ent->setVY(ent->getVY() + MainWindow::gravity / MainWindow::frameRate);
        ent->setVX(ent->getVX() * (1.0 - 0.0000245 * ent->getVX() / MainWindow::frameRate));
        ent->updateV(MainWindow::frameRate);
    } //{Null, Terrain, Samos, Monster, Area, DynamicObj, NPC, Projectile};

    for (std::vector<Entity*>::iterator i = rendering.begin(); i != rendering.end(); i++) {
        for (std::vector<Entity*>::iterator j = i+1; j!= rendering.end(); j++) {
            if (Entity::checkCollision(*i,*j)) {
                Entity::handleCollision(*i, *j);
            }
        }
    }
}

void MainWindow::updateAnimations()
{
    for (Entity* entity : rendering) {
        if (entity->getState() != entity->getLastFrameState()) {
            entity->updateAnimation();
        }
        entity->setAnimation(entity->getAnimation() + 1);
        if (entity->getCurrentAnimation().size() <= entity->getAnimation())
            entity->setAnimation(0);
        entity->updateTexture();
    }
}

const std::vector<Entity *> &MainWindow::getRendering() const
{
    return rendering;
}

void MainWindow::setRendering(const std::vector<Entity *> &newRendering)
{
    rendering = newRendering;
}
