#ifndef PHYSICS_H
#define PHYSICS_H

#include <Entities/entity.h>
#include <Entities/projectile.h>

#include "Entities/area.h"
#include "Entities/dynamicobj.h"
#include "Entities/entity.h"
#include "Entities/living.h"
#include "Entities/monster.h"
#include "Entities/npc.h"
#include "Entities/samos.h"
#include "Entities/terrain.h"
#include "Entities/door.h"
#include "map.h"

class Physics
{
public:
    static double gravity; //p.s^-2
    static double frameRate; //fps

    static std::tuple<std::string, std::vector<Entity*>, std::vector<Entity*>> updatePhysics(Samos *s, std::vector<Terrain*> *ts, std::vector<DynamicObj*> *ds, std::vector<Monster*> *ms, std::vector<Area*> *as, std::vector<NPC*> *ns, std::vector<Projectile*> *ps, Map currentMap);
    static std::vector<Entity*> handleCollision(Entity* obj1, Entity* obj2);
    static bool updateProjectile(Projectile* p);
    static bool canChangeBox(Entity *e, CollisionBox *b, std::vector<Terrain*> *ts, std::vector<DynamicObj*> *ds);
    static std::vector<Entity*> updateSamos(Samos *s, std::vector<Terrain*> *ts, std::vector<DynamicObj*> *ds, std::map<std::string, bool> inputList, std::map<std::string, double> inputTime);
    static void updateCamera(Samos *s, QPoint camera, Map currentMap);
};

#endif // PHYSICS_H
