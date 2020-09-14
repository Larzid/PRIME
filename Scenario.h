#ifndef SCENARIO_H
#define SCENARIO_H

#include "ObjectIlk.h"

struct scenario {
    shObjId vat_bubbly_beverage;
};

extern scenario Scen;

void generate_scenario (scenario &);

#endif
