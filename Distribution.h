#ifndef DISTR_H
#define DISTR_H

#include <stdint.h>
#include "MonsterIlk.h"
#include "ObjectIlk.h"

namespace prob {

struct Table
{
    unsigned int chance;
    unsigned int at_least, at_most;
    char type;
    union {
        uintptr_t dummy;
        shMonId monster;
        shObjId item;
        Table *sub;
    };
};

#define END_TOKEN {0, 0, 0, 'x', 0}

#define MAX_MON_LEV 20
extern Table *MonByLevel[MAX_MON_LEV];
extern Table Mainframe;
extern Table RobotTown;
extern Table Sewers;
extern Table GammaCaves;
extern Table ShodanGroup;

extern Table Money[];
extern Table RayGun[];
extern Table Ammunition[];
extern Table Grenade[];
extern Table Skillsoft[];
extern Table AbilModImplant[];
extern Table OtherImplant[];
extern Table Implant[];
extern Table CommonDisk[];
extern Table RareDisk[];
extern Table OtherDisk[];
extern Table FloppyDisk[];
extern Table Can5[];
extern Table Can50[];
extern Table Can75[];
extern Table Can100[];
extern Table Can200[];
extern Table Can1000[];
extern Table Canister[];
extern Table Beverage[];
extern Table VeryRareTool[];
extern Table RareTool[];
extern Table UncommonTool[];
extern Table CommonTool[];
extern Table Keycard[];
extern Table Tool[];
extern Table Gizmo[];
extern Table PowerPlant[];
extern Table Computer[];
extern Table Jumpsuit[];
extern Table Belt[];
extern Table Goggles[];
extern Table Boots[];
extern Table Cloak[];
extern Table Helmet[];
extern Table BodyArmor[];
extern Table BioArmor[];
extern Table ChaosArmor[];
extern Table PowerArmor[];
extern Table PowerHelmet[];
extern Table Armor[];
extern Table HeavyConvGun[];
extern Table HeavyEnergyGun[];
extern Table LightConvGun[];
extern Table LightEnergyGun[];
extern Table HandConvGun[];
extern Table HandEnergyGun[];
extern Table ConvGun[];
extern Table EnergyGun[];
extern Table Cannon[];
extern Table LightRifle[];
extern Table HeavyRifle[];
extern Table Gun[];
extern Table CommonMeleeWeapon[];
extern Table RareMeleeWeapon[];
extern Table MeleeWeapon[];
extern Table Sword[];
extern Table Weapon[];
extern Table NotMoney[];
extern Table RandomObject[];
}
#endif
