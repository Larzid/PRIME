#ifndef PROPERTY_H
#define PROPERTY_H
#include "Intrinsic.h"

namespace cond {
enum shTrigger {
    carry        = 0x0000,
    buggy        = 0x0001,
    debugged     = 0x0002,
    optimized    = 0x0004,
    nonbuggy     = 0x0008,
    toggled      = 0x0010,
    not_toggled  = 0x0020,
    active       = 0x0040,
    inactive     = 0x0080,
    wielded      = 0x0100,
    worn         = 0x0200,
    has_charges  = 0x0400,
    uncovered    = 0x0800,
    on_bare_skin = 0x1000,
    mult_chg     = 0x2000,
    mult_enhc    = 0x4000,
    sword_skill  = 0x8000
};
}

struct shProperties
{
    int cond;
    shIntrinsic intr;
    int value;
};

#endif
