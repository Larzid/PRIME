#ifndef WEAPON_H
#define WEAPON_H

#include "Global.h"

namespace wpn {

shAttack *get_gun_attack (shCreature *c, shObject **weapon, int &hit_mod, int &rounds);

}

#endif

