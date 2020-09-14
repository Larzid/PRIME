#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "Global.h"

namespace trn {

#define MAXTRACES 4
struct WarpTrace
{
    shMapLevel *level;
    int time;
    int src_x, src_y;
    int dst_x, dst_y;
    char *who;
};
extern WarpTrace traces[MAXTRACES];
extern int last_trace;

typedef bool (shMapLevel::*coord_pred)(int, int);

bool coord_in_stripe (int &x, int &y, int min, int max, coord_pred pred);

/* Clears table and trace count. */
void initialize ();

/* Chooses destination randomly on map if creature fails to control it. */
/* TODO: Implement this and replace shCreature::transport with trn::random. */
bool random (shCreature *c, int control_mod, bool imprecise);

/* Tries to place affected creature withing <min_range, max_range> squares
   from its original position on map.  May be controlled in which case
   range limitations do not apply. */
bool ranged (shCreature *c, int min_range, int max_range, int control_mod);

/* Asks for destination square if creature is hero.  When imprecise, chosen
   coordinates will be randomly shifted by small amount. */
bool controlled (shCreature *c, bool imprecise);

/* Always controlled, disregards usual rules. */
bool BOFH (shCreature *c, int x, int y);

/* Saves warp trace of transport. */
void register_trace (shCreature *c, int src_x, int src_y, int dst_x, int dst_y);

}

#endif
