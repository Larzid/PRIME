#include "Transport.h"
#include "Creature.h"
#include "Monster.h"
#include "Map.h"
#include "Hero.h"
#include <string.h>

static bool
transport_blocked (shCreature *c)
{
    if (c->mLevel->noTransport ()) {
        if (c->isHero ())
            I->p ("You %s for a moment.",
                Hero.cr ()->intr (kBlind) ? "shudder" : "flicker");

        return true;
    }
    return false;
}

/* A transport may place the creature off the destination in some cases. */
static void
perturb_transport (shCreature *c, int &x, int &y)
{
    if (c->isA (kMonShodan) and !(c->is (kConfused) or c->is (kStunned)))
        return;

    int tries = 50;
    int nx, ny;
    do {
        nx = x + RNG (15) - 7;
        ny = y + RNG (9) - 4;
        Level->find_close_free_sq (nx, ny);
    } while (nx == x and ny == y and tries--);

    (x = nx), (y = ny);

    if (!c->isHero ())
        return;

    if (c->mLevel->isMainframe ())
        I->p ("It's hard to transport accurately in here.");
    else
        I->p ("You fail to control the transport precisely.");
}

static bool
can_be_controlled (shCreature *c, int control)
{
    if (!c->isHero ())
        return false;

    control += c->mIntrinsics.get (kWarpControl);
    return RNG (100) < control;
}

namespace trn {

WarpTrace traces[MAXTRACES];
int last_trace;

void
initialize ()
{
    for (int i = 0; i < MAXTRACES; ++i) {
        traces[i].level = NULL;
        traces[i].who = NULL;
    }
    last_trace = 0;
}

void
register_trace (shCreature *c, int src_x, int src_y, int dst_x, int dst_y)
{
    traces[last_trace].src_x = src_x;
    traces[last_trace].src_y = src_y;
    traces[last_trace].dst_x = dst_x;
    traces[last_trace].dst_y = dst_y;
    traces[last_trace].level = c->mLevel;
    if (traces[last_trace].who)
        free (traces[last_trace].who);
    traces[last_trace].who = strdup (c->myIlk ()->mName);
    traces[last_trace].time = Clock;
    last_trace = (last_trace + 1) % MAXTRACES;
}

bool
BOFH (shCreature *c, int x, int y)
{
    c->free_from_traps ();

    int tries = 10, tmp_x = x, tmp_y = y;
    do {
        if (c->mLevel->moveCreature (c, tmp_x, tmp_y) == 0)
            return true;

        (tmp_x = x), (tmp_y = y);
        c->mLevel->find_close_free_sq (tmp_x, tmp_y);
    } while (tries--);

    return false;
}

bool
controlled (shCreature *c, bool imprecise)
{
    if (transport_blocked (c))
        return false;

    int x = -1, y = -1;
    if (!c->isHero () or
        !I->getSquare ("Transport to what location?", &x, &y, -1))
    {
        return false;
    }

    if (imprecise or c->mLevel->isMainframe ())
        perturb_transport (c, x, y);

    if (c->mLevel->moveCreature (c, x, y) == 0)
        return true;

    if (c->mLevel->findAdjacentUnoccupiedSquare (&x, &y) == 0)
        if (c->mLevel->moveCreature (c, x, y) == 0)
            return true;

    return false;
}

bool coord_in_stripe (int &ox, int &oy, int min, int max, coord_pred pred)
{
    /* Define outer and inner region. */
    int out_sx = maxi (0, ox - max);
    int out_sy = maxi (0, oy - max);
    int out_ex = mini (Level->mColumns - 1, ox + max);
    int out_ey = mini (Level->mRows    - 1, oy + max);
    int inn_sx = maxi (0, ox - min);
    int inn_sy = maxi (0, oy - min);
    int inn_ex = mini (Level->mColumns - 1, ox + min);
    int inn_ey = mini (Level->mRows    - 1, oy + min);

    /* Count valid squares. */
    int max_coords;
    {
        int field_outer = (out_ex - out_sx + 1) * (out_ey - out_sy + 1);
        int field_inner = (inn_ex - inn_sx + 1) * (inn_ey - inn_sy + 1);
        max_coords = field_outer - field_inner;
    }

    if (max_coords <= 0)
        return false;

    /* Get coordinates of all valid squares. */
    struct pair {
        int x, y;
    };

    pair *coord = (pair *) calloc (max_coords, sizeof (pair));
    int num_coords = 0;

    for (int x = out_sx; x <= out_ex; ++x)
        for (int y = out_sy; y <= out_ey; ++y)
            if ((x < inn_sx or x > inn_ex) or
                (y < inn_sy or y > inn_ey))
            {
                coord[num_coords].x = x;
                coord[num_coords++].y = y;
            }

    /* Test coordinates until good one is found or they are depleted. */
    while (num_coords) {
        int chosen = RNG (num_coords);
        int x = coord[chosen].x, y = coord[chosen].y;

        if ((Level->*pred) (x, y)) {
            (ox = x), (oy = y);
            free (coord);
            return true;
        }

        --num_coords;
        coord[chosen].x = coord[num_coords].x;
        coord[chosen].y = coord[num_coords].y;
    }

    free (coord);
    return false;
}

bool
ranged (shCreature *c, int min, int max, int control_mod)
{
    if (transport_blocked (c))
        return false;

    if (can_be_controlled (c, control_mod) and controlled (c, false))
        return true;

    /* TODO: if max = 100 then use random hit algorithm - it is faster. */
    int x = c->mX, y = c->mY;
    if (coord_in_stripe (x, y, min, max, &shMapLevel::is_free_safe_sq))
        return c->mLevel->moveCreature (c, x, y) == 0;

    return false;
}

}
