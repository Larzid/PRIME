#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdio.h>
#include "gen/ObjectIlk.h"

struct _scen_internal;

struct Scenario
{
    void generate ();
    void clear ();
    void save_state (FILE *);
    bool load_state (FILE *);

    shObjId bubbly_beverage ();

private:
    _scen_internal *m;
};

extern Scenario Scen;

#endif
