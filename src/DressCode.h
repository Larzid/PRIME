#ifndef DRESSCODE_H
#define DRESSCODE_H

#include "Global.h"

namespace dress {

enum DressCode
{
    Janitor,
    Ninja,
    Stormtrooper,
    Troubleshooter,
    Num_DressCode
};

}

bool looks_like (shCreature *, dress::DressCode);

#endif
