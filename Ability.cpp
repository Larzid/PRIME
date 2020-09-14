#include "Global.h"
#include "Ability.h"

namespace abil {

int
Set::get (Index idx) const
{
    switch (idx) {
    case Str: return _str;
    case Con: return _con;
    case Agi: return _agi;
    case Dex: return _dex;
    case Int: return _int;
    case Psi: return _psi;
    default:
        debug.log ("Error: Trying to read ability %d.", idx);
        assert (false);
        return 6;
    }
}

void
Set::set (Index idx, int val)
{
    switch (idx) {
    case Str: _str = val; break;
    case Con: _con = val; break;
    case Agi: _agi = val; break;
    case Dex: _dex = val; break;
    case Int: _int = val; break;
    case Psi: _psi = val; break;
    default:
        debug.log ("Error: Trying to set ability %d to %d.", idx, val);
        assert (false);
        break;
    }
}

void
Set::mod (Index idx, int delta)
{
    set (idx, get (idx) + delta);
}

bool
applies (shCreatureType ct, Index idx)
{
    if (idx < Str or idx > Psi)
        return false;

    static const bool bot_droid[] = {true,  true,  true,  true,  true,  false};
    static const bool construct[] = {true,  true,  true,  true,  false, false};
    static const bool program[] =   {true,  true,  true,  true,  true,  false};

    int i = int(idx) - int(Str);
    switch (ct) {
    case kBot: case kDroid:
        return bot_droid[i];
    case kConstruct:
        return construct[i];
    case kProgram:
        return program[i];
    case kEgg: case kOoze: case kCyborg: case kAberration: case kAnimal:
    case kAlien: case kBeast: case kHumanoid: case kMutant: case kInsect:
    case kOutsider: case kVermin: case kZerg:
        return true;
    case kMaxCreatureType:
        return false;
    }
    return false;
}

const char *
name (shCreatureType ct, Index i)
{
    switch (ct) {
    case kBot: case kDroid:
        switch (i) {
        case Str: return "strength";
        case Con: return "construction";
        case Agi: return "agility";
        case Dex: return "dexterity";
        case Int: return "artificial intelligence";
        case Psi: return "psionics";
        default: break;
        }
    case kConstruct:
        switch (i) {
        case Str: return "strength";
        case Con: return "construction";
        case Agi: return "agility";
        case Dex: return "dexterity";
        case Int: return "---";
        case Psi: return "---";
        default: break;
        }
    case kProgram:
        switch (i) {
        case Str: return "strength";
        case Con: return "construction";
        case Agi: return "agility";
        case Dex: return "dexterity";
        case Int: return "intelligence";
        case Psi: return "psionics";
        default: break;
        }
    case kEgg: case kOoze: case kCyborg: case kAberration: case kAnimal:
    case kAlien: case kBeast: case kHumanoid: case kMutant: case kInsect:
    case kOutsider: case kVermin:
        switch (i) {
        case Str: return "strength";
        case Con: return "constitution";
        case Agi: return "agility";
        case Dex: return "dexterity";
        case Int: return "intelligence";
        case Psi: return "psionics";
        default: break;
        }
    case kZerg:
        switch (i) {
        case Str: return "strength";
        case Con: return "constitution";
        case Agi: return "agility";
        case Dex: return "dexterity";
        case Int: return "intelligence";
        case Psi: return "hive link";
        default: break;
        }
    case kMaxCreatureType:
        return "uncreatureness";
    }
    return "bug affinity";
}

const char *
shortn (shCreatureType ct, Index i)
{
    switch (ct) {
    case kBot: case kDroid:
        switch (i) {
        case Str: return "Str";
        case Con: return "Con";
        case Agi: return "Agi";
        case Dex: return "Dex";
        case Int: return "AI ";
        case Psi: return "Psi";
        default: break;
        }
    case kConstruct:
        switch (i) {
        case Str: return "Str";
        case Con: return "Con";
        case Agi: return "Agi";
        case Dex: return "Dex";
        case Int: return "---";
        case Psi: return "---";
        default: break;
        }
    case kProgram:
        switch (i) {
        case Str: return "Str";
        case Con: return "Con";
        case Agi: return "Agi";
        case Dex: return "Dex";
        case Int: return "Int";
        case Psi: return "Psi";
        default: break;
        }
    case kEgg: case kOoze: case kCyborg: case kAberration: case kAnimal:
    case kAlien: case kBeast: case kHumanoid: case kMutant: case kInsect:
    case kOutsider: case kVermin:
        switch (i) {
        case Str: return "Str";
        case Con: return "Con";
        case Agi: return "Agi";
        case Dex: return "Dex";
        case Int: return "Int";
        case Psi: return "Psi";
        default: break;
        }
    case kZerg:
        switch (i) {
        case Str: return "Str";
        case Con: return "Con";
        case Agi: return "Agi";
        case Dex: return "Dex";
        case Int: return "Int";
        case Psi: return "HvL";
        default: break;
        }
    case kMaxCreatureType:
        break;
    }
    return "BUG";
}

Index
random_index ()
{
    return (Index) RNG (1, 6);
}

void
Set::zero ()
{
    _str = _con = _agi = _dex = _int = _psi = 0;
}


int Tracker::Str () const { return _totl._str; }
int Tracker::Con () const { return _totl._con; }
int Tracker::Agi () const { return _totl._agi; }
int Tracker::Dex () const { return _totl._dex; }
int Tracker::Int () const { return _totl._int; }
int Tracker::Psi () const { return _totl._psi; }

int
Tracker::totl (Index idx) const
{
    return _totl.get (idx);
}

int
Tracker::temp (Index idx) const
{
    return _temp.get (idx);
}

int
Tracker::gear (Index idx) const
{
    return _gear.get (idx);
}

int
Tracker::curr (Index idx) const
{
    return _totl.get (idx) - _temp.get (idx) - _gear.get (idx);
}

int
Tracker::base (Index idx) const
{
    return _max.get (idx) - _temp.get (idx) - _gear.get (idx);
}

int
Tracker::harm (Index idx) const
{
    int val = base (idx) - curr (idx);
    return val < 0 ? 0 : val;
}

void
Tracker::restore (Index idx, unsigned int heal)
{
    _totl.mod (idx, mini (heal, harm (idx)));
}

void
Tracker::perm_mod (Index idx, int delta)
{
    _totl.mod (idx, delta);
    _max.mod (idx, delta);
}

void
Tracker::hurt_mod (Index idx, int delta)
{
    _totl.mod (idx, mini (0, delta));
}

void
Tracker::temp_mod (Index idx, int delta)
{
    _totl.mod (idx, delta);
    _max.mod (idx, delta);
    _temp.mod (idx, delta);
}

void
Tracker::init (unsigned int abil[6])
{
    FOR_ALL_ABILITIES (a) {
        _max.set (a, abil[a - abil::Str]);
        _totl.set (a, abil[a - abil::Str]);
    }
    _temp.zero ();
    _gear.zero ();
}

}
