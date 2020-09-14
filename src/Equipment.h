#ifndef EQUIP_H
#define EQUIP_H

#include "Distribution.h"
#include <stdint.h>
#include "Global.h"

struct EquipmentLine {
    union {
        uintptr_t dummy;
        shObjId id;
        prob::Table *set;
    } ref;
    const char *mod;
};

void equip_by_list (shCreature *c, const EquipmentLine *table);

void equip_monster (shCreature *c);
#endif
