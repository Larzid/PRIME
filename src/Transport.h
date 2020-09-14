#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "Global.h"
#include "PointerStore.h"

namespace trn {

struct WarpTrace
{
    shMapLevel *level;
    int time;
    int src_x, src_y;
    int dst_x, dst_y;
    char *who;
};

struct WarpMonitor
{
    WarpMonitor ();
    ~WarpMonitor ();

    /* Clear structure.  Allocate if needed. */
    void reset ();
    /* Remember creature 'c' teleporting from (src_x,src_y) to (dst_x,dst_y). */
    void trace (shCreature *c, int src_x, int src_y, int dst_x, int dst_y);

    /* Those two routines work in tandem.  Call 'first' to get the freshest transport
       trace on a level.  Then call 'next' with the argument repeatedly to iterate
       through list of all traces until 'next' returns NULL.  Note that 'first' may
       return NULL too. */
    WarpTrace *first ();
    WarpTrace *next (WarpTrace *);

    void save_state (FILE *, sav::PointerStore *);
    bool load_state (FILE *, sav::PointerStore *);

private:
    unsigned int capacity;
    int last_trace;
    WarpTrace *traces;
};

extern WarpMonitor *wmon;

typedef bool (shMapLevel::*coord_pred)(int, int);

/* For a given pair of coordinates it finds a spot in a "stripe" described by
   minimum and maximum distance from point of origin.  Variables 'x' and 'y'
   indicate start point and are used to return result.

   Parameter 'pred' accepts a function for filtering possible coordinates.

   Example for min = 3, max = 4

        XXXXXXXXX
        XXXXXXXXX   o - point at (x,y)
        XX     XX   X - potentially valid result
        XX     XX
        XX  o  XX
        XX     XX
        XX     XX
        XXXXXXXXX
        XXXXXXXXX
*/
bool coord_in_stripe (int &x, int &y, int min, int max, coord_pred pred);

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


/* Recreates default WarpMonitor instance. */
void initialize ();

}
#endif
