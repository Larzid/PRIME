#ifndef ABILITY_H
#define ABILITY_H

#include "CrType.h"

namespace abil {

enum Index {
    No  = -1,
    Str = 1,
    Con = 2,
    Agi = 3,
    Dex = 4,
    Int = 5,
    Psi = 6
};
#define NUM_ABIL abil::Psi

struct Set {
    int _str, _con, _agi, _dex, _int, _psi;

    int get (Index idx) const;
    void set (Index idx, int val);
    void mod (Index idx, int delta);
    void zero ();
};

#define FOR_ALL_ABILITIES(x) \
    for (abil::Index x = abil::Str; \
         x <= abil::Psi; \
         x = (abil::Index) ((int)(x)+1))

struct Tracker {
    int Str () const;
    int Con () const;
    int Agi () const;
    int Dex () const;
    int Int () const;
    int Psi () const;

    int totl (Index idx) const;
    int curr (Index idx) const;
    int base (Index idx) const;
    int harm (Index idx) const;
    int temp (Index idx) const;
    int gear (Index idx) const;
    void restore (Index idx, unsigned int heal);
    void perm_mod (Index idx, int delta);
    void hurt_mod (Index idx, int delta);
    void temp_mod (Index idx, int delta);
    void init (unsigned int abil[6]);

    struct Set _totl, _max, _gear, _temp;
};

bool applies (shCreatureType, Index);
const char *name (shCreatureType, Index);
const char *shortn (shCreatureType, Index);
Index random_index ();

}
#endif
