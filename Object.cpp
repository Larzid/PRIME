#include <ctype.h>
#include <math.h>
#include "Global.h"
#include "Object.h"
#include "Interface.h"
#include "ObjectType.h"
#include "Creature.h"
#include "Hero.h"
#include "Distribution.h"
#include "Transport.h"

extern shFlavor Flavors[];

static const shColor floppyColors[] = {kRed, kGreen, kBrown, kMagenta,
    kCyan, kGray, kOrange, kLime, kYellow, kNavy,
    kPink, kAqua, kWhite};
static const int numFloppyColors = sizeof (floppyColors) / sizeof (shColor);

const char *matname[kLastMaterial] = {
    "fleshy", "paper", "cloth", "plastic", "leather", "wood", "bone",
    "iron", "brass", "tin", "bronze", "lead", "steel", "aluminum", "asbestos",
    "titanium", "kevlar", "carbon-fiber", "plasteel", "adamantium",
    "depleted uranium", "copper", "silver", "gold", "platinum", "sillicon",
    "glass", "crystal"
};

void
randomizeIlkNames ()
{
    static const int shuftable[][2] =
    {
     {kFFirstBelt, kFLastBelt},
     {kFFirstCan, kFLastCan},
     {kFFirstDisk, kFLastDisk},
     /*{kFFirstGrenade, kFLastGrenade}, <-- pointless */
     {kFFirstGizmo, kFLastGizmo},
     {kFFirstImplant, kFLastImplant},
     {kFFirstJumpsuit, kFLastJumpsuit},
     {kFFirstKey, kFLastKey},
     {kFFirstRayGun, kFLastRayGun}
    };
    const int nshuf = sizeof (shuftable) / sizeof (int[2]);

    /* Shuffle all static flavor names. */
    for (int i = 0; i < nshuf; ++i)
        shuffle (Flavors + shuftable[i][0],
                 shuftable[i][1] - shuftable[i][0] + 1,
                 sizeof (shFlavor));

    /* Generate and assign grenade names.  Prepare short names
       consisting only from third part. */

    /* Some data to cook names from.  Stylized after Xenocide. */
    static const char *adjs[] = {"heavy", "little", "shiny", "small",
        "strange", "tiny", "polished", "rugged", "smooth", "rigid"};
    const int nadjs = sizeof (adjs) / sizeof (char *);
    static const char *mats[] = {"aluminum", "carbon-fiber", "electronic",
        "lead", "metal", "plastic", "alien-alloy", "plasteel", "sillicon",
        "steel", "titanium"};
    const int nmats = sizeof (mats) / sizeof (char *);
    /* Material also dicates color. */
    static const shColor matcols[nmats] = {kAqua, kBrown, kGreen, kGray,
        kCyan, kYellow, kLime, kMagenta, kOrange, kNavy, kWhite};
    /* Also weight adjustment.  Not implemented until we do the necessary
       reseach how exactly each material should modify weight so that chosen
       numbers are at least remotely believeable. */
    static const char *shapes[] = {"ball", "box", "rounded box", "cube",
        "cuboid", "rounded cuboid", "cylinder", "device", "disc", "oval disc",
        "egg", "stick", "tetrahedron", "torus"};
    const int nshapes = sizeof (shapes) / sizeof (char *);
    /* Shapes will determine tile in the future. */
    const int tier2 = nadjs;
    const int tier3 = tier2 * nmats;

    /* Generate grenade flavor names by permuting choices. */
    const int numgren = kFLastGrenade - kFFirstGrenade + 1;
    int choices[numgren];
    for (int i = 0; i < numgren; ++i) {
        bool ok;
        int n1, n2, n3;
        do { /* Make sure names do not repeat. */
            n1 = RNG (nadjs);
            n2 = RNG (nmats);
            n3 = RNG (nshapes);
            choices[i] = n1 + tier2 * n2 + tier3 * n3;
            ok = true;
            /* Check if new choice matches with any earlier. */
            for (int j = i-1; j >= 0; --j) {
                if (choices[j] == choices[i]) { /* It does!  Retry. */
                    ok = false;
                    break;
                }
            }
        } while (!ok);
        int k = i + kFFirstGrenade; /* Index into flavor table. */
        snprintf (Flavors[k].mAppearance.mName, 40, "%s %s %s",
                  adjs[n1], mats[n2], shapes[n3]);
        Flavors[k].mAppearance.mGlyph.mColor = matcols[n2];
        snprintf (Flavors[k].mVague.mName, 40, "%s", shapes[n3]);
        /* Vague tile.  Base tiles start at column 3. */
        Flavors[k].mVague.mGlyph.mTileX = n3 + 3;
        /* Actual tile.  20th column and later are free. */
        Flavors[k].mAppearance.mGlyph.mTileX = 20 + i;
    }
}

shTileRow
wpnRow (shSkillCode skill)
{
    switch (skill) {
    case kHandgun:       return kRowHandgun;
    case kLightGun:      return kRowLightGun;
    case kHeavyGun:      return kRowHeavyGun;
    case kMeleeWeapon:   return kRowMelee;
    case kSword:         return kRowSword;
    case kUnarmedCombat: return kRowUnarmed;
    case kGrenade:       return kRowMissile;
    default:             return kRowDefault;
    }
}

shTileRow
mslRow (shObjId parent)
{
    if (AllIlks[parent].subtype == grenade)
        return kRowGrenade;
    return kRowMissile;
}

shTileRow
armRow (shObjId parent)
{
    switch (AllIlks[parent].subtype) {
    case nonspecial:  return kRowBodyArmor;
    case helmet:      return kRowHelmet;
    case goggles:     return kRowGoggles;
    case belt:        return kRowBelt;
    case boots:       return kRowBoots;
    case cloak:       return kRowCloak;
    case jumpsuit:    return kRowJumpsuit;
    case body_armor:  return kRowBodyArmor;
    default:          return kRowDefault;
    }
}

shTileRow
toolRow (shObjId parent)
{
    switch (AllIlks[parent].subtype) {
    case computer:       return kRowComputer;
    case keycard:        return kRowKeycard;
    case gizmo:          return kRowGizmo;
    case power_plant:    return kRowPowerPlant;
    default:             return kRowGenTool;
    }
}

shEnergyType
shObject::vulnerability ()
{
    if (isA (kEnergyCell) or (isA (kImplant) and mIlkId != kObjTorc)) {
        return kElectrical;
    } else if (isA (kCanister)) {
        if (isA (kObjLNO)) return kNoEnergy; /* Already frozen. */
        return kFreezing;
    }
    switch (myIlk ()->mMaterial) {
    case kFleshy:
    case kPaper:
    case kCloth:
    case kLeather:
    case kWood:
    case kPlastic:
    case kBone:
        return kBurning;

    case kIron:
    case kBrass:
    case kTin:
    case kBronze:
    case kLead:
    case kSteel:
    case kAluminum:
    case kCopper:
    case kSilver:
        return kCorrosive;

    case kSilicon:
    case kGlass:
    case kCrystal:
        return kElectrical;

    case kTitanium:
    case kGold:
    case kPlatinum:
    case kCarbonFiber:
    case kPlasteel:
    case kAdamantium:
    case kAsbestos:
    case kKevlar:
    case kDepletedUranium:
    case kLastMaterial:
        return kNoEnergy;
    }
    return kNoEnergy;
}

void initializeEquipment (void); /* Equipment.cpp */
void initializeProperties (void); /* Property.cpp */

void
initializeObjects ()
{
    initializeTools ();
    initializeWeapons ();
    initializeRayGuns ();
    initializeCanisters ();
    initializeFloppyDisks ();
    initializeImplants ();
    initializeLore ();
    initializeEquipment ();
    initializeProperties ();
    for (int i = 0; i < kObjNumIlks; ++i) {
        AllIlks[i].mId = shObjId (i);

        /* Use flavor if applicable. */
        if (AllIlks[i].mFlavor) {
            shFlavorId fid = AllIlks[i].mFlavor;
            /* Overrides name? */
            if (Flavors[fid].mAppearance.mName[0] != 0) {
                AllIlks[i].mAppearance.mName = Flavors[fid].mAppearance.mName;
                /* Where main name is empty override that. */
                if (!AllIlks[i].mReal.mName)
                    AllIlks[i].mReal.mName = Flavors[fid].mAppearance.mName;
            }
            if (Flavors[fid].mVague.mName[0] != 0)
                AllIlks[i].mVague.mName = Flavors[fid].mVague.mName;
            /* Color?  Symbol? */
            if (Flavors[fid].mAppearance.mGlyph.mColor) {
                AllIlks[i].mAppearance.mGlyph.mColor = Flavors[fid].mAppearance.mGlyph.mColor;
                /* Real color is not defined override that too. */
                if (!AllIlks[i].mReal.mGlyph.mColor)
                    AllIlks[i].mReal.mGlyph.mColor = Flavors[fid].mAppearance.mGlyph.mColor;
            }
            if (Flavors[fid].mVague.mGlyph.mColor)
                AllIlks[i].mVague.mGlyph.mColor = Flavors[fid].mVague.mGlyph.mColor;
            if (Flavors[fid].mAppearance.mGlyph.mSym != '@')
                AllIlks[i].mAppearance.mGlyph.mSym = Flavors[fid].mAppearance.mGlyph.mSym;
            if (Flavors[fid].mVague.mGlyph.mSym != '@')
                AllIlks[i].mVague.mGlyph.mSym = Flavors[fid].mVague.mGlyph.mSym;
            /* Tile? */
            if (Flavors[fid].mAppearance.mGlyph.mTileX != -1)
                AllIlks[i].mAppearance.mGlyph.mTileX = Flavors[fid].mAppearance.mGlyph.mTileX;
            if (Flavors[fid].mVague.mGlyph.mTileX != -1)
                AllIlks[i].mVague.mGlyph.mTileX = Flavors[fid].mVague.mGlyph.mTileX;
            if (Flavors[fid].mAppearance.mGlyph.mTileY != -1)
                AllIlks[i].mAppearance.mGlyph.mTileY = Flavors[fid].mAppearance.mGlyph.mTileY;
            if (Flavors[fid].mVague.mGlyph.mTileY != -1)
                AllIlks[i].mVague.mGlyph.mTileY = Flavors[fid].mVague.mGlyph.mTileY;
            /* Modifies material, price or weight? */
            if (Flavors[fid].mMaterial != kPaper)
                AllIlks[i].mMaterial = Flavors[fid].mMaterial;
            AllIlks[i].mCost += Flavors[fid].mCostMod;
            AllIlks[i].mWeight += Flavors[fid].mWeightMod;
        }

        /* If appearance description data is unspecified, it is assumed
           to be identical to real description.  Likewise, if vague
           description data is unspecified, it is assumed to be identical
           to appearance description data. */
        if (!AllIlks[i].mAppearance.mName)
            AllIlks[i].mAppearance.mName = AllIlks[i].mReal.mName;
        if (!AllIlks[i].mVague.mName)
            AllIlks[i].mVague.mName = AllIlks[i].mAppearance.mName;

        if (!AllIlks[i].mAppearance.mType)
            AllIlks[i].mAppearance.mType = AllIlks[i].mReal.mType;
        if (!AllIlks[i].mVague.mType)
            AllIlks[i].mVague.mType = AllIlks[i].mAppearance.mType;

        if (AllIlks[i].mAppearance.mGlyph.mSym == '@')
            AllIlks[i].mAppearance.mGlyph.mSym = AllIlks[i].mReal.mGlyph.mSym;
        if (AllIlks[i].mVague.mGlyph.mSym == '@')
            AllIlks[i].mVague.mGlyph.mSym = AllIlks[i].mAppearance.mGlyph.mSym;

        if (!AllIlks[i].mAppearance.mGlyph.mColor)
            AllIlks[i].mAppearance.mGlyph.mColor = AllIlks[i].mReal.mGlyph.mColor;
        if (!AllIlks[i].mVague.mGlyph.mColor)
            AllIlks[i].mVague.mGlyph.mColor = AllIlks[i].mAppearance.mGlyph.mColor;

        if (AllIlks[i].mAppearance.mGlyph.mTileY == -1)
            AllIlks[i].mAppearance.mGlyph.mTileY = AllIlks[i].mReal.mGlyph.mTileY;
        if (AllIlks[i].mVague.mGlyph.mTileY == -1)
            AllIlks[i].mVague.mGlyph.mTileY = AllIlks[i].mAppearance.mGlyph.mTileY;
        if (AllIlks[i].mAppearance.mGlyph.mTileX == -1)
            AllIlks[i].mAppearance.mGlyph.mTileX = AllIlks[i].mReal.mGlyph.mTileX;
        if (AllIlks[i].mVague.mGlyph.mTileX == -1)
            AllIlks[i].mVague.mGlyph.mTileX = AllIlks[i].mAppearance.mGlyph.mTileX;
        /* Help detect unlinked tiles.
           Anything undefined should point to `this is not a tile' tile. */
        if (AllIlks[i].mReal.mGlyph.mTileX == -1) {
            AllIlks[i].mReal.mGlyph.mTileX = 5;
            AllIlks[i].mReal.mGlyph.mTileY = kRowDefault;
        }
        if (AllIlks[i].mAppearance.mGlyph.mTileX == -1) {
            AllIlks[i].mAppearance.mGlyph.mTileX = 5;
            AllIlks[i].mAppearance.mGlyph.mTileY = kRowDefault;
        }
        if (AllIlks[i].mVague.mGlyph.mTileX == -1) {
            AllIlks[i].mVague.mGlyph.mTileX = 5;
            AllIlks[i].mVague.mGlyph.mTileY = kRowDefault;
        }
    }
    initializeParser ();
}

bool
shObjectIlk::isA (shObjId ilk)
{
    return mId == ilk;
}

bool
shObject::has_subtype (subtype_t s)
{
    return myIlk ()->subtype == s;
}

void
shObject::announce (void)
{
    I->p ("%c - %s", mLetter, inv ());
}

bool
shObjectIlk::has_mini_icon ()
{
    return mReal.mGlyph.mTileY == kRowTag1 or
           mReal.mGlyph.mTileY == kRowTag2 or
           mReal.mGlyph.mTileY == kRowCursor;
}

void
shObject::name (const char *newname)
{
    if (!newname) {
        char prompt[80];
        char buf[80];

        snprintf (prompt, 80, "%same %s:", mUserName ? "Ren" : "N", these ());
        if (!I->getStr (buf, 40, prompt)) {
            I->p ("Never mind.");
            return;
        }
        newname = buf;
    }
    if (mUserName) {
      free ((void *) mUserName);
    }
    if (1 == strlen (newname) and ' ' == *newname) {
        mUserName = NULL;
    } else {
        mUserName = strdup (newname);
    }
}

const char *
shObject::stackDesc (int cnt, const char *pronoun, const char *desc)
{
    if (cnt == -1)  cnt = mCount;
    char *buf = GetBuf ();
    if (cnt > 1)
        snprintf (buf, SHBUFLEN, "%s%d %s", pronoun, cnt, desc);
    else
        snprintf (buf, SHBUFLEN, "%s%s", pronoun, desc);
    return buf;
}

const char *
shObject::those (int cnt)
{
    const char *pronoun = "those ";
    if (cnt == 1 or (cnt == -1 and mCount == 1))
        pronoun = "that ";
    return stackDesc (cnt, pronoun, getDescription (cnt));
}

const char *
shObject::these (int cnt)
{
    const char *pronoun = "these ";
    if (cnt == 1 or (cnt == -1 and mCount == 1))
        pronoun = "this ";
    return stackDesc (cnt, pronoun, getDescription (cnt));
}

const char *
shObject::anStackDesc (int cnt, const char *desc)
{
    if (cnt == -1)  cnt = mCount;
    const char *pronoun =
        cnt > 1 ? "" :
        isUniqueKnown () ? "the " :
        isvowel (desc[0]) ? "an " : "a ";
    return stackDesc (cnt, pronoun, desc);
}

const char *shObject::an (int cnt) {
    return anStackDesc (cnt, getDescription (cnt));
}

const char *shObject::anQuick (int cnt) {
    return anStackDesc (cnt, getShortDescription (cnt));
}

const char *shObject::anVague (int cnt) {
    return anStackDesc (cnt, getVagueDescription ());
}

const char *shObject::the (int cnt) {
    return stackDesc (cnt, "the ", getDescription (cnt));
}

const char *shObject::theQuick (int cnt) {
    return stackDesc (cnt, "the ", getShortDescription (cnt));
}

const char *shObject::your (int cnt) {
    return stackDesc (cnt, "your ", getShortDescription (cnt));
}

const char *
shObject::her (shCreature *owner, int cnt)
{
    const char *pronoun = "its ";
    if (kFemale == owner->mGender)
        pronoun = "her ";
    else if (kMale == owner->mGender)
        pronoun = "his ";
    return stackDesc (cnt, pronoun, getDescription (cnt));
}

int
shObject::isA (shObjId type)
{
    return myIlk ()->isA (type);
}


int
shObject::canMerge (shObject *obj)
{
    if (!obj) {
        obj = this;
    }
    if (obj->myIlk () != myIlk () or kObjWreck == mIlkId) {
        return 0;
    }
    if ((myIlk ()->mFlags & kMergeable) and
        (obj->mFlags == mFlags) and
        (obj->mBugginess == mBugginess) and
        (obj->mDamage == mDamage) and
        (!obj->mUserName and !mUserName))
    {
        return 1;
    }
    return 0;
}


shObjectVector DeletedObjects;

/* Note: Weight issues are handled by caller. */
void
shObject::merge (shObject *obj)
{
    if (this == obj)  return;
    mCount += obj->mCount;
    mCharges += obj->mCharges;
    /* Discard unneeded containers. */
    if (isChargeable ()) {
        int capacity = myIlk ()->mMaxCharges;
        int needed = mCharges / capacity + (mCharges % capacity > 0);
        mCount = needed;
    }
    /* HACK: don't delete the object right away; we might need to use
       it to print a message or something.  (For example, the case of
       a shopkeeper giving a quote about an object picked up in a
       shop.) */
    DeletedObjects.add (obj);
}


int
shObject::sufferDamage (shAttackId id, shCreature *attacker, int x, int y)
{
    return sufferDamage (&Attacks[id], attacker, x, y);
}

/* Returns number of objects destroyed from stack. */
int
shObject::sufferDamage (shAttack *attack, shCreature *attacker, int x, int y)
{
    if (isIndestructible ())
        return 0;

    for (int j = 0; j < ATKDMG; ++j) {
        shEnergyType energy = attack->mDamage[j].mEnergy;

        if (kNoEnergy == energy)
            break;

        /* If some type of energy deals damage consider it enough. */
        int result = sufferDamage (energy, attack, attacker, x, y);
        if (result)
            return result;
    }

    return 0;
}

/* Returns number of objects destroyed from stack. */
int
shObject::sufferDamage (shEnergyType energy, shAttack *attack,
    shCreature *attacker, int x, int y)
{
    if (isIndestructible ())  return 0;
    /* Cannot get damaged by what you protect against. */
    if (myIlk ()->mResistances[energy] > 0)  return 0;
    bool annihilated = false;
    bool destroy = false;
    bool cansee = Hero.cr ()->canSee (x, y);
    bool nearby = distance (Hero.cr (), x, y) <= 30;
    bool owned = Hero.cr ()->owns (this);
    const char *theobj;

    if (owned) {
        cansee = 1;
        theobj = YOUR (this);
    } else if (mOwner) {
        theobj = her (mOwner);
    } else {
        theobj = theQuick ();
    }

    /* First consider only attack type. */
    if (energy == kNoEnergy) switch (attack->mType) {
    case shAttack::kHealingRay:
        if (isA (kObjBioArmor)) {
            if (!owned) {
                cansee = false;
            }
            int worked = 0;
            if (mEnhancement < 2) {
                ++mEnhancement;
                worked = 1;
            }
            debug.log ("healed %s %d", theobj, mEnhancement);
            if (cansee) {
                if (worked)
                    I->p ("%s feels tougher!", theobj);
                else
                    I->p ("%s seems to be enjoying something.", theobj);
            }
        }
        break;
    case shAttack::kPlague:
        if (isA (kObjBioArmor) and !isA (kObjMBioArmor1)) {
            int domsg = 1;
            int damage = 3;
            if (!owned) {
                cansee = 0;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee) {
                    if (mEnhancement > -3) {
                        I->p ("%s manages to resist somewhat.", theobj);
                    } else {
                        I->p ("%s tries to resist but fails.", theobj);
                    }
                }
                damage = 1;
                domsg = 0;
            }
            mEnhancement -= damage;
            debug.log ("plagued %s %d", theobj, mEnhancement);
            if (domsg) {
                if (mEnhancement <= -4) {
                    I->p ("%s gets horribly sick and dies!", theobj);
                } else {
                    I->p ("%s gets much weaker!", theobj);
                }
            }
            if (mEnhancement <= -4) return 1;
        }
        break;
    case shAttack::kRestorationRay:
        if (isA (kObjBioArmor)) {
            if (!owned and
                (Hero.cr ()->intr (kBlind) or x != Hero.cr ()->mX or y != Hero.cr ()->mY))
            {
                cansee = 0;
            }
            int worked = 0;
            if (isBuggy ()) {
                setDebugged ();
                ++worked;
            }
            if (mEnhancement < 0) {
                mEnhancement = 0;
                ++worked;
            }
            if (mDamage) {
                mDamage = 0;
                ++worked;
            }
            debug.log ("restored %s %d", theobj, mEnhancement);
            if (cansee) {
                switch (worked) {
                case 0: I->p ("%s seems to be enjoying something.", theobj);
                    break;
                case 1: I->p ("%s feels better!", theobj); break;
                case 2: I->p ("%s feels much better!", theobj); break;
                case 3: I->p ("%s feels great!", theobj); break;
                }
            }
        }
        break;
    default:
        break;
    }
    if (energy == kNoEnergy)  return 0;

    /* Note: If energy effect causes something to destroy and this
       is accompanied by a special message, use return to skip default
       destruction messages. */
    switch (energy) {
    case kCorrosive:
        if (!owned)
            cansee = false; /* reduce messages */
        if (isA (kEnergyCell)) {
            if (cansee) {
                I->p ("%s dissolve%s!", theobj, mCount == 1 ? "s" : "");
            }
            return mCount;
        } else if (kCorrosive == vulnerability ()) {
            if (is (obj::fooproof)) {
                if (cansee)
                    I->p ("%s is not affected.", theobj);
                return 0;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee) {
                    set (obj::known_bugginess);
                    I->p ("Somehow, %s is not affected.", theobj);
                }
                return 0;
            }
            if (mDamage < 3) {
                ++mDamage;
            } else {
                return 0; /* Avoid message spam. */
            }
            debug.log ("corroded %s %d", theobj, mDamage);
            if (cansee) {
                switch (mDamage) {
                case 1:
                    I->p ("%s corrodes!", theobj); break;
                case 2:
                    I->p ("%s corrodes some more!", theobj); break;
                default:
                    I->p ("%s is thoroughly corroded!", theobj); break;
                }
            }
        } else if (cansee)  I->p ("%s is not affected.", theobj);
        break;
    case kElectrical:
        if (kElectrical != vulnerability ())
            break;
        if (mOwner and is (obj::worn)) {
            if (owned) {
                I->p ("%s is ejected from your %s in a shower of sparks!",
                      theobj, describeImplantSite (mImplantSite));
            } else if (cansee) {
                I->p ("%s is ejected from head of %s in a shower of sparks!",
                      theobj, THE (mOwner));
            }
            mOwner->doff (this);
            shCreature *owner = mOwner;
            mOwner->removeObjectFromInventory (this);
            Level->putObject (this, owner->mX, owner->mY);
            return 0;
        }
        break;
    case kBurning:
    case kPlasma:
        if (isA (kMoney)) {
            I->p ("%s melt%s!", theobj, mCount == 1 ? "s" : "");
            return mCount;
        } else if (isA (kFloppyDisk)) {
            int numdestroyed = RNG (4) ? 1 : 2;
            if (cansee) {
                if (1 == mCount) {
                    I->p ("%s melts!", theobj);
                    numdestroyed = 1;
                } else if (1 == numdestroyed) {
                    I->p ("One of %s melts!", theobj);
                } else if (numdestroyed == mCount) {
                    I->p ("%s melt!", theobj);
                } else {
                    I->p ("Some of %s melt!", theobj);
                }
            } else if (nearby) { /* FIXME: No msg needed when underwater. */
                I->p ("You smell burning plastic.");
            }
            return numdestroyed;
        } else if (kBurning == vulnerability ()) {
            if (!owned)
                cansee = false; /* reduce messages */
            if (is (obj::fooproof)) {
                if (cansee)
                    I->p ("%s is not affected.", theobj);
                return 0;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee) {
                    set (obj::known_bugginess);
                    I->p ("Somehow, %s is not affected.", theobj);
                }
                return 0;
            }
            if (mDamage < 4)
                ++mDamage;
            debug.log ("burned %s %d", theobj, mDamage);
            if (cansee) {
                switch (mDamage) {
                case 1:
                    I->p ("%s burns!", theobj); break;
                case 2:
                    I->p ("%s burns some more!", theobj); break;
                case 3:
                    I->p ("%s is thoroughly burned!", theobj); break;
                default:
                    I->p ("%s burns completely!", theobj);
                }
            }
            if (mDamage == 4)
                return 1;
        }
        break;
    case kFreezing:
        if (isA (kCanister) and kFreezing == vulnerability ()) {
            int numdestroyed = RNG (4) ? 1 : 2;
            if (cansee) {
                if (1 == mCount ) {
                    I->p ("%s freezes and shatters!", theobj);
                } else if (1 == numdestroyed) {
                    I->p ("One of %s freezes and shatters!", theobj);
                } else if (numdestroyed == mCount) {
                    I->p ("%s freeze and shatter!", theobj);
                } else {
                    I->p ("Some of %s freeze and shatter!", theobj);
                }
            } else if (nearby) { /* FIXME: No msg when in vacuum. */
                I->p ("You hear something shatter.");
            }
            return numdestroyed;
        }
        break;
    case kMagnetic:
    {
        int oldenh = mEnhancement;
        /* Area attacks carry much more power. */
        if (attack->mEffect == shAttack::kBeam or
            attack->mEffect == shAttack::kBurst or
            attack->mEffect == shAttack::kCone or
            attack->mEffect == shAttack::kFarBurst)
        {
            /* FIXME: clerkbot should get mad if any of this is hero's fault. */
            if (isA (kObjM56Smartgun) or isA (kObjM57Smartgun)) {
                mIlkId = kObjPulseRifle;  /* Erase auto-aim system. */
                if (mOwner)  mOwner->computeSkills ();
            }
            if (has_subtype (computer)) {
                mEnhancement = -5; /* Lost OS. */
                if (is (obj::designated))
                    mOwner->computeSkills ();
            }
            if (isA (kFloppyDisk)) { /* Yes, the whole stack. */
                mIlkId = kObjBlankDisk;
                clear (obj::cracked | obj::infected);
                set (obj::known_cracked | obj::known_infected);
            }
            mBugginess = 0;
            mEnhancement = 0;
        }
        /* Introduce some variety in damage.  It would be boring if
           a hit either damaged no equipment or all for -1 enhancement.
           Similarly neutralizing bugginess for every item or none is
           undesirable. */
        mEnhancement = mEnhancement * RNG (2, 4) / 5;
        if (oldenh != mEnhancement and mOwner and is (obj::worn)) {
            if (NOT0 (mOwner, ->isHero ()))
                I->p ("You feel performance of %s %screase.", YOUR (this),
                      oldenh > mEnhancement ? "de" : "in");
            mOwner->computeIntrinsics ();
        }
        if (!RNG (3))
            setDebugged ();
        break;
    }
    case kBugging:
        if (isBugProof ())
            break;
        if (isOptimized ()) {
            setDebugged ();
        } else {
            setBuggy ();
        }
        break;
    case kDmgStr: case kDmgCon: case kDmgAgi: case kDmgDex:
        if (isA (kObjBioArmor) and !isA (kObjMBioArmor0)) {
            if (!owned and
                (Hero.cr ()->intr (kBlind) or x != Hero.cr ()->mX or y != Hero.cr ()->mY))
            {
                cansee = false;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee)  I->p ("Somehow, %s is not affected.", theobj);
                return 0;
            }
            --mEnhancement;
            debug.log ("poisoned %s %d", theobj, mEnhancement);
            if (cansee) {
                if (mEnhancement == -4) {
                    I->p ("%s quivers and dies!", theobj);
                } else {
                    I->p ("%s weakens!", theobj);
                }
            }
            if (mEnhancement == -4) return 1;
        } else if (isA (kObjGreenCPA)) {
            if (!owned)
                cansee = false;
            int worked = 0;
            if (mEnhancement < 3) {
                ++mEnhancement;
                worked = 1;
            }
            debug.log ("poisoned %s %d", theobj, mEnhancement);
            if (cansee and worked) {
                Hero.cr ()->visual (fmt ("%s glistens.", theobj)) or
                    Hero.cr ()->msg (fmt ("%s throbs.", theobj));
            }
        }
        break;
    case kDisintegrating:
        annihilated = true;
        break;
    case kConcussive:
        //if (myIlk ()->mMaterial == kLeather)  break;
        if (isComputer ()) {
            /* Computers generally do not like to be kicked around... */
            if (!RNG (10)) {
                if (cansee)  I->p ("Sparks fly from %s!", theobj);
                setBuggy ();
            }
            if (isA (kObjBioComputer) and !RNG (3) and hasSystem ()) {
                if (cansee)  I->p ("%s suffers brain damage.", theobj);
                --mEnhancement;
            }
            /* ... but this may also be beneficial. */
            kickComputer ();
        }
        destroy = myIlk ()->mMaterial == kGlass or isA (kFloppyDisk);
        break;
    case kTransporting:
    {
        if (this->mOwner)
            break;  /* Carried items are immune. */

        int i = attack->findEnergyType (kTransporting);
        assert (i != -1);
        if (i == -1)
            break;

        int dst_x, dst_y;
        if (attack->mDamage[i].mHigh == 100) {
            Level->findOpenSquare (&dst_x, &dst_y);
        } else {
            int min = attack->mDamage[i].mLow;
            int max = attack->mDamage[i].mHigh;
            trn::coord_pred predicate = &shMapLevel::is_open_sq;
            trn::coord_in_stripe (dst_x = mX, dst_y = mY, min, max, predicate);
        }

        int dst_room = Level->getRoomID (dst_x, dst_y);
        bool changed_rooms = Level->getRoomID (mX, mY) != dst_room;
        bool changed_shops = Level->mRooms[dst_room].is_shop ();

        shObjectVector *v = Level->getObjects (mX, mY);
        assert (v);

        if (changed_rooms and !changed_shops and is (obj::unpaid)) {
            attacker->usedUpItem (this, this->mCount, "transport");
            clear (obj::unpaid);
        }

        if (v)
            v->remove (this);
        Level->putObject (this, dst_x, dst_y);
        break;
    }
    default:
        break;
    }

    if (destroy) {
        if (cansee) {
            if (annihilated) {
                I->p ("%s is annihilated!", theobj);
            } else if (myIlk ()->mMaterial == kGlass) {
                I->p ("%s shatters!", theobj);
            } else {
                I->p ("%s is destroyed!", theobj);
            }
        } else if (nearby and myIlk ()->mMaterial == kGlass) {
            I->p ("You hear something shatter.");
        }
        return 1;
    }
    return 0;
}


int
shObject::getPsiModifier ()
{
    if (isA (kObjHarmonyDome) and !isBuggy ()) {
        /* Negate all psi aura disruption caused by cranial implants. */
        int psimod = 0;
        for (int i = 0; i <= shObjectIlk::kCerebellum; ++i) {
            if (mOwner->mImplants[i] and
                mOwner->mImplants[i]->myIlk ()->mPsiModifier < 0)
            {
                psimod -= mOwner->mImplants[i]->myIlk ()->mPsiModifier;
            }
        }
        return psimod;
    } else if (isA (kObjPsiAmp) or isA (kObjSBioArmor4)) {
        return mEnhancement + myIlk ()->mPsiModifier;
    } else {
        return myIlk ()->mPsiModifier;
    }
}


int
shObject::getArmorBonus ()
{
    if (kArmor != myIlk ()->mReal.mType) {
        return 0;
    } else {
        return maxi (0, myIlk ()->mArmorBonus - mDamage + mEnhancement);
    }
}


int
shObject::getThreatRange ()
{
    if (isA (kWeapon)) {
        return 20;
    } else { /* impossible to critical hit with improvised weapon */
        return 999;
    }
}

int
shObject::getCriticalMultiplier ()
{
    /* impossible to critical hit with improvised weapon */
    return isA (kWeapon) ? 2 : 1;
}

static shObjectVector *
createEquipment (shObjectVector *inv, const prob::Table *table)
{
    /* Weighted choice from sublist. */
    if (table->type == 's') {
        int count = RNG (table->at_least, table->at_most);

        for (int i = 0; i < count; ++i) {
            prob::Table *ptr = table->sub;
            int weight_sum = 0;
            while (ptr->chance != 0) {
                weight_sum += ptr->chance;
                ++ptr;
            }

            ptr = table->sub;
            int choice = RNG (weight_sum);
            while (choice >= 0) {
                choice -= ptr->chance;
                ++ptr;
            }
            inv = createEquipment (inv, --ptr);
        }
        return inv;

    /* Create specific object(s). */
    } else if (table->type == 'i') {
        shObject *obj = new shObject (table->item);
        obj->mCount = RNG (table->at_least, table->at_most);
        inv->add (obj);
        return inv;

    /* Create object set.  Execute each entry from sublist. */
    } else if (table->type == 'I') {
        prob::Table *set = table->sub;
        /* Chance is not treated like weight -- it is a percentage
           chance of executing this row. */
        while (set->chance != 0)
            if (RNG (100) < (int)set->chance) {
                shObject *obj = new shObject (table->item);
                obj->mCount = RNG (table->at_least, table->at_most);
                inv->add (obj);
            }

        return inv;
    }

    assert (false);
    return inv;
}

shObject *
generateObject ()
{
    shObjectVector v;
    static prob::Table dummy = {100, 1, 1, 's', 0};
    dummy.sub = prob::RandomObject;
    createEquipment (&v, &dummy);
    return v.get (0);
}

shObject *
gen_obj_type (prob::Table *table)
{
    shObjectVector v;
    static prob::Table dummy = {100, 1, 1, 's', 0};
    dummy.sub = table;
    createEquipment (&v, &dummy);
    return v.get (0);
}


static int InventorySortOrder [kMaxObjectType] =
{
    13, 0, 5, 8, 6, 11, 3, 1, 2, 12, 7, -1
};

/* used for sorting
   returns: -1, 0 or 1 depending on whether obj1 belongs before, with, or
            after obj2 in an inventory list.  sorts on type
*/
int
compareObjects (shObject **obj1, shObject **obj2)
{
    int t1 = InventorySortOrder[(*obj1)->apparent ()->mType];
    int t2 = InventorySortOrder[(*obj2)->apparent ()->mType];
    if (t1 < t2) return -1;
    if (t1 > t2) return 1;
    return 0;
}


void
purgeDeletedObjects ()
{
    while (DeletedObjects.count ()) {
        delete DeletedObjects.removeByIndex(0);
    }
}

/* Returns object that should be shown at the top of the stack. */
shObject *   /* This means: find most interesting thing to player. */
findBestObject (shObjectVector *objs, bool markSeen)
{   /* Finds object with the lowest objecttype. */
    shObject *bestobj = NULL;
    int besttype = kMaxObjectType;

    for (int i = 0; i < objs->count (); ++i) {
        shObject *obj = objs->get (i);
        if (markSeen)  obj->set (obj::known_appearance);
        if (obj->apparent ()->mType < besttype) {
            besttype = obj->apparent ()->mType;
            bestobj = obj;
        }
    }
    return bestobj;
}

using namespace obj;

shObject::shObject (shObjId id)
{
    extern int getChargesForRayGun (shObjId raygun);
    mIlkId = id;
    mFlags = 0;
    mLetter = 0;
    mUserName = NULL;
    mOwner = NULL;
    mLastEnergyBill = MAXTIME;

    /* Set count. */
    if (myIlk ()->mFlags & kMergeable) {
        /* Fuel tank has 50 capacity.  One is enough. */
        if (mIlkId == kObjFlamerFuel)  mCount = 1;
        /* Ammunition.  Bullets, shells, slugs and like. */
        else if (isA (kProjectile))  mCount = NDX (5, 6);
        /* Shurikens, daggers and grenades. */
        else if (myIlk ()->mFlags & kMissile)  mCount = RNG (1, 4);
        else if (id == kObjEnergyCell)  mCount = NDX (8, 10);
        /* Canisters can merge but are usually generated single. */
        else  mCount = 1;
    } else { /* Guns, armor pieces, tools etc. */
        mCount = 1;
    }

    /* Set bugginess. */
    if (kBugProof & myIlk ()->mFlags) {
        mBugginess = 0;
        set (known_bugginess);
    } else if (kUsuallyBuggy & myIlk ()->mFlags and RNG (9)) {
        mBugginess = -1;
    } else {
        int tmp;
        if (isA (kImplant) or isA (kFloppyDisk)) tmp = RNG (6);
        else tmp = RNG (8);
        mBugginess = (1 == tmp) ? 1 : (0 == tmp) ? -1 : 0;
    } /* Special cases for some items. */
    if (kObjSpamDisk == mIlkId and RNG (4)) {
        mBugginess = -1;
    } else if (kObjBugDetectionDisk == mIlkId and RNG (2)) {
        mBugginess = 1;
    }

    /* Set crackedness. */
    if (isA (kFloppyDisk)) {
        if (!RNG (100) and
            mIlkId != kObjHackingDisk and mIlkId != kObjMatterCompiler)
        {
            set (cracked);
        }
    } else if (has_subtype (keycard)) {
        if (!RNG (100) and !isA (kObjMasterKeycard))
            set (cracked);
    }

    /* Set fooproof knowledge.  Currently only relevant for computers. */
    /*
    if (has_subtype (computer))
        set (known_fooproof);
    else
    */
        set (known_fooproof);

    /* Set enhancement.  Requires bugginess determined beforehand. */
    if (!isEnhanceable ()) {
        mEnhancement = 0;
    } else if (isA (kArmor)) {
        int max = myIlk ()->mMaxEnhancement;
        if (isOptimized ()) {
            mEnhancement = (RNG (max) * RNG (max) + max - 1) / max;
        } else if (isBuggy ()) {
            mEnhancement = RNG (max) * RNG (max) / -max;
        } else {
            mEnhancement = RNG (6) ? RNG (max) * RNG (max) / max
                                   : RNG (max) * RNG (max) / -max;
        }
    } else if (isA (kWeapon)) {
        int tmp = RNG (20); /* -1 or +1 weapons are rare. */
        mEnhancement = (1 == tmp) ? 1 : (0 == tmp) ? -1 : 0;
    } else if (isA (kImplant)) {
        if (isOptimized ()) {
            mEnhancement = RNG (1, 5);
        } else if (isBuggy ()) {
            mEnhancement = -RNG (1, 5);
        } else {
            mEnhancement = RNG (6) ? RNG (0, 2) + RNG (0, 3) : -RNG (0, 5);
        }
    } else if (has_subtype (computer)) {
        if (!RNG (10)) {
            mEnhancement = -5; /* No OS! */
        } else if (!RNG (8)) {
            mEnhancement = RNG(-4, +4); /* Hardware independent OS. */
        } else switch (mBugginess) { /* Some OS. */
            case -1: mEnhancement = RNG(-4, -1); break;
            case  0: mEnhancement = RNG(-1, +1); break;
            case +1: mEnhancement = RNG(+1, +4); break;
        }
    } else { /* Other enhanceable items start out without it. */
        mEnhancement = 0;
    }

    /* Set charges. */
    if (mIlkId == kObjEmptyRayGun) {
        mCharges = 0;
    } else if (isA (kRayGun)) {
        mCharges = getChargesForRayGun (mIlkId);
    } else if (has_subtype (power_plant)) {
        mCharges = RNG (1, myIlk ()->mMaxCharges);
        set (known_charges);
    } else if (has_subtype (energy_tank)) {
        mCharges = RNG (0, myIlk ()->mMaxCharges / 5);
        set (known_charges);
    } else if (mIlkId == kObjKhaydarin) {
        mCharges = NDX (2, 2);
    } else if (mIlkId == kObjPlasmaCaster) {
        mCharges = !RNG (10) ? 1 : 0;
    } else if (mIlkId == kObjCryolator) {
        /* No less than 50.  Let the player have some fun. */
        mCharges = RNG (50, myIlk ()->mMaxCharges);
    } else if (mIlkId == kObjShockCapacitor) {
        mCharges = RNG (4);
    } else if (mIlkId == kObjFlamerFuel) {
        mCharges = RNG (myIlk ()->mMaxCharges / 2, myIlk ()->mMaxCharges);
        set (known_charges);
    } else {
        mCharges = RNG (0, myIlk ()->mMaxCharges);
    }

    /* Set damage or infectedness. */
    mDamage = 0; /* Currently no items start damaged. */
    if (mIlkId == kObjBlankDisk) {
        if (isIlkKnown ())  {
            set (known_infected | known_cracked);
        }
    } else if (!isInfectable ()) {
        set (known_infected);
    } else if (mIlkId == kObjComputerVirus or
               (mIlkId != kObjAntivirusDisk and
                mIlkId != kObjResidentAntivirusDisk and
                !RNG (20)))
    {
        set (infected);
    }

    /* Set toggledness. */
    if (isSelectiveFireWeapon ()) {
        set (toggled); /* Guns start out in burst mode. */
    } else if (isA (kObjBioMask)) {
        if (RNG (2))
            set (toggled); /* Random mode. */
    }

    /* Remaining properties. */
    if (!isEnhanceable ())
        set (known_enhancement);
    if (!isChargeable ())
        set (known_charges);
    if (!isCrackable ())
        set (known_cracked);

    if (!strcmp (myIlk ()->mVague.mName, myIlk ()->mAppearance.mName) and
        !isA (kObjTorc) and !isA (kObjLightSaber) and !isA (kObjFlashlight))
    {
        set (known_appearance);
    }

    /* Some items come in random colors. */
    if (isA (kObjLightSaber)) {
        mColor = RNG (3); /* Red, blue and green. */
    } else if (isA (kFloppyDisk)) {
        mColor = RNG (numFloppyColors);
    } else {
        mColor = 0;
    }
}

shGlyph
shObject::getGlyph ()
{ /* Put specific items first as to not allow categories to override them. */
    shGlyph g;
    if (isA (kObjTorc)) {
        g = myIlk ()->mAppearance.mGlyph;
        if (is (known_appearance)) {
            switch (mBugginess) {
            case -1: g.mColor = kGray; break;
            case  0: g.mColor = kWhite; break;
            case +1: g.mColor = kYellow; break;
            }
            g.mTileX += mBugginess;
        }
    } else if (isA (kObjLightSaber)) {
        if (is (known_appearance) and is (known_type)) {
            g = myIlk ()->mReal.mGlyph;
            g.mTileX += 7 + mColor;
            if      (mColor == 0) g.mColor = kOrange;
            else if (mColor == 1) g.mColor = kLime;
            else if (mColor == 2) g.mColor = kNavy;
        } else if (is (known_type)) {
            g = myIlk ()->mReal.mGlyph;
        } else {
            g = myIlk ()->mAppearance.mGlyph;
        }
    } else if (isA (kObjBloodSword)) {
        g = apparent ()->mGlyph;
        if (is (known_type) and is (known_enhancement)) {
            /*           +---------------------------------+
            mEnhancement | -5 -4 -3 -2 -1 0 +1 +2 +3 +4 +5 |
                     mod | -2 -1 -1 -1 -1 0 +1 +1 +1 +1 +2 |
                         +---------------------------------+ */
            int val = mEnhancement < 0 ? -mEnhancement : mEnhancement;
            int sgn = mEnhancement ? mEnhancement / val : 0;
            int mod = sgn * (!(val % 5) + !!mEnhancement);
            g.mTileX += mod;
        }
    } else if (isA (kFloppyDisk)) {
        g = myIlk ()->mVague.mGlyph;
        if (is (known_appearance)) {
            g.mColor = floppyColors[mColor];
            g.mTileX = 3+mColor;
        } else {
            g.mColor = kGray;
        }
    } else if (myIlk ()->has_mini_icon ()) {
        if (is (known_appearance)) {
            g = myIlk ()->mAppearance.mGlyph;
        } else {
            g = myIlk ()->mVague.mGlyph;
        }
    } else {
        g = apparent ()->mGlyph;
    }
    return g;
}

void
shObject::maybeName ()
{
    if (!isIlkKnown () and !myIlk ()->mUserName and is (known_appearance))
        nameIlk ();
}

void
shObject::nameIlk ()
{
    if (!is (known_appearance)) {
        I->p ("You would never recognize another one.");
        return;
    }

    const char *desc;
    {   /* Retrieve description without previously given user name (if any). */
        const char *tmp = myIlk ()->mUserName;
        myIlk ()->mUserName = NULL;
        desc = getDescription ();
        myIlk ()->mUserName = tmp;
    }

    char prompt[80], buf[80];

    snprintf (prompt, 80, "Call %s:", desc);
    if (!I->getStr (buf, 40, prompt)) {
        I->p ("Never mind.");
        return;
    }
    if (myIlk ()->mUserName) {
      free ((void *) myIlk ()->mUserName);
      myIlk ()->mUserName = NULL;
    }
    if (1 == strlen (buf) and ' ' == *buf) {
        return;
    } else {
        myIlk ()->mUserName = strdup (buf);
    }
}

const char *
shObject::getVagueDescription ()
{
    return myIlk ()->mVague.mName;
}

const char *
shObject::getShortDescription (int cnt)
{
    if (cnt == -1)  cnt = mCount;
    int l = 0;
    char *buf = GetBuf ();
    const char *name = apparent ()->mName;

    if (isA (kObjLightSaber) and is (known_appearance) and is (known_type)) {
        const char *color;
        switch (mColor) {
        case 0: color = "red"; break;
        case 1: color = "green"; break;
        case 2: color = "blue"; break;
        default: color = "funny"; break;
        }
        l += snprintf (buf + l, SHBUFLEN - l, "%s ", color);
    }
    l += snprintf (buf + l, SHBUFLEN - l, "%s", name);
    if (cnt > 1) {
        makePlural (buf, SHBUFLEN);
        l = strlen (buf); /* Pluralisation alters length. */
    }
    if (myIlk ()->mUserName) {
        l += snprintf (buf + l, SHBUFLEN - l, " called %s", myIlk ()->mUserName);
    }
    return buf;
}

const char *
shObject::getDescription (int cnt)
{
    if (cnt == -1)  cnt = mCount;
    int l = 0;
    const char *fooproof_str = NULL;
    const char *dmg;
    char *buf = GetBuf ();

    if (is (known_bugginess) and !isBugProof ()) {
        if (mIlkId != kObjTorc) {
            l += snprintf (buf + l, SHBUFLEN - l, "%s ",
                           isBuggy () ? "buggy" :
                           isOptimized () ? "optimized" : "debugged");
        } else {
            l += snprintf (buf + l, SHBUFLEN - l, "%s ",
                           isBuggy () ? "gray" :
                           isOptimized () ? "golden" : "silver");
        }
    }

    switch (vulnerability ()) {
    case kCorrosive: fooproof_str = "acidproof"; dmg = "corroded"; break;
    case kBurning: fooproof_str = "fireproof"; dmg = "burnt"; break;
    default: dmg = "damaged";
    }
    if (has_subtype (computer))
        fooproof_str = "protected";

    switch (mDamage) {
    case 1:
        l += snprintf (buf + l, SHBUFLEN - l, "%s ", dmg); break;
    case 2:
        l += snprintf (buf + l, SHBUFLEN - l, "very %s ", dmg); break;
    case 3:
        l += snprintf (buf + l, SHBUFLEN - l, "extremely %s ", dmg); break;
    default: break;
    }

    if (is (fooproof) and is (known_fooproof)) {
        l += snprintf (buf + l, SHBUFLEN - l, "%s ", fooproof_str);
    }

    if (isInfectable () and is (known_infected) and
        /* Protected and clean together are redundant. */
        (!is (fooproof) or !is (known_fooproof)))
    {
        l += snprintf (buf + l, SHBUFLEN - l,
                       is (infected) ? "infected " : "clean ");
    }

    if (is (cracked | known_cracked)) {
        l += snprintf (buf + l, SHBUFLEN - l, "cracked ");
    } else if (isA (kFloppyDisk) and is (known_cracked) and !is (cracked)) {
        l += snprintf (buf + l, SHBUFLEN - l, "legal ");
    }

    if (isEnhanceable () and is (known_enhancement)) {
        l += snprintf (buf + l, SHBUFLEN - l, "%s%d ",
                       mEnhancement < 0 ? "" : "+", mEnhancement);
    }

    if (kObjWreck == mIlkId) {
        l += snprintf (buf + l, SHBUFLEN - l, "disabled %s", MonIlks[mWreckIlk].mName);
        if (cnt > 1) {
            makePlural (buf, SHBUFLEN);
            l = strlen (buf); /* Pluralisation alters length. */
        }
    } else {
        const char *shortd = getShortDescription (cnt);
        l += snprintf (buf +l, SHBUFLEN - l, "%s", shortd);
    }
    if (mUserName) {
        l += snprintf (buf + l, SHBUFLEN - l, " named %s", mUserName);
    }

    if (isChargeable () and is (known_charges) and !isA (kObjEmptyRayGun)) {
        if (has_subtype (energy_tank))
            l += snprintf (buf + l, SHBUFLEN - 1, " (%d/%d)", mCharges,
                           myIlk ()->mMaxCharges);
        else if (isA (kObjFlamerFuel))
            l += snprintf (buf + l, SHBUFLEN - 1, " (%d unit%s)", mCharges,
                           mCharges == 1 ? "" : "s");
        else
            l += snprintf (buf + l, SHBUFLEN - 1, " (%d charg%s)", mCharges,
                           mCharges == 1 ? "e" : "es");
    }
    buf[SHBUFLEN-1] = 0;
    return buf;
}

const char *
shObject::inv ()
{
    char *buf = GetBuf ();
    int l;

    if (mCount > 1) {
        l = snprintf (buf, SHBUFLEN, "%d %s", mCount, getDescription ());
    } else {
        const char *tmp = getDescription ();
        l = snprintf (buf, SHBUFLEN, "%s %s",
                      isUniqueKnown () ? "the" :
                      isvowel (tmp[0]) ? "an" : "a", tmp);
    }
    if (is (worn)) {
        if (isA (kImplant)) {
            l += snprintf (buf + l, SHBUFLEN - l, " (implanted in %s)",
                           describeImplantSite (mImplantSite));
        } else {
            l += snprintf (buf + l, SHBUFLEN - l, " (worn");
            if (isA (kObjBioMask) and is (known_type)) {
                l += snprintf (buf + l, SHBUFLEN - l, ", %s",
                               is (toggled) ? "EM field mode" : "thermal mode");
            }
            l += snprintf (buf + l, SHBUFLEN - l, ")");
        }
    } else if (is (wielded)) {
        l += snprintf (buf + l, SHBUFLEN - l, " (wielded");
        if (isSelectiveFireWeapon () and is (known_type)) {
            if (myIlk ()->mAmmoType == kObjEnergyCell) {
                l += snprintf (buf + l, SHBUFLEN - l, ", %s",
                               is (toggled) ? "high power" : "low power");
            } else {
                l += snprintf (buf + l, SHBUFLEN - l, ", %s",
                               is (toggled) ? "burst mode" : "single fire");
            }
        }
        l += snprintf (buf + l, SHBUFLEN - l, ")");
    }
    if (is (prepared)) {
        l += snprintf (buf + l, SHBUFLEN - l, " (prepared)");
    }
    if (is (designated)) {
        l += snprintf (buf + l, SHBUFLEN - l, " (designated)");
    }
    if (is (active)) {
        l += snprintf (buf + l, SHBUFLEN - l, " (activated");
        if (isA (kObjGeigerCounter)) {
            if (Hero.getStoryFlag ("geiger counter message")) {
                l += snprintf (buf + l, SHBUFLEN - 1, ", clicking");
            }
        }
        l += snprintf (buf + l, SHBUFLEN - l, ")");
    }
    if (is (unpaid)) {
        l += snprintf (buf + l, SHBUFLEN - l, " (unpaid, $%d)",
                       mCount * myIlk ()->mCost);
    } else if (Hero.cr ()->isInShop () and Level->getShopKeeper (Hero.cr ()->mX, Hero.cr ()->mY) and
               !isA (kObjMoney))
    {
        l += snprintf (buf + l, SHBUFLEN - l, " (worth $%d)",
                       mCount * myIlk ()->mCost);
    }
    l += snprintf (buf + l, SHBUFLEN - 1, " {%d}", getMass ());
    buf[SHBUFLEN-1] = 0;
    return buf;
}

shObject *
shObject::split (int count)
{
    shObject *result;

    assert (count < mCount);

    mCount -= count;
    result = new shObject ();
    result->mIlkId = mIlkId;
    result->mCount = count;
    result->mBugginess = mBugginess;
    result->mFlags = mFlags;
    result->clear (designated | prepared);
    if (isChargeable ()) {
        /* Split charges leaving full objects in original place. */
        result->mCharges = mCharges;
        result->mCharges -= myIlk ()->mMaxCharges * mCount;
        mCharges -= result->mCharges;
    }
    if (isA (kObjWreck)) {
        result->mWreckIlk = mWreckIlk;
    } else {
        result->mColor = mColor;
    }
    return result;
}

shDescription *
shObject::apparent ()
{
    if (is (known_type))
        return &myIlk ()->mReal;
    if (is (known_appearance))
        return &myIlk ()->mAppearance;
    return &myIlk ()->mVague;
}

void
shObject::identify ()
{
    set (known_exact);
    setIlkKnown ();
}

bool
shObject::isEquipped ()
{
    return mFlags & (worn | wielded | designated);
}

bool
shObject::isDamaged () {
    return mDamage;
}

bool
shObject::canBeWorn ()
{
    return looksLikeA (kArmor) or looksLikeA (kImplant) or
           (is (known_type) and
            (isComputer () or isA (kObjPlasmaCaster)));
}

bool
shObject::isKnownRadioactive ()
{   /* It is radioactive and ... */
    return (isRadioactive () and
    /* .. you know it. */
        ((isIlkKnown () and (!isA (kObjXRayGoggles) or is (known_bugginess))) or
    /* ... you see it. */
        Hero.cr ()->usesPower (kGammaSight))) or
    /* It is likely to be radioactive and you suspect it. */
        (isA (kObjMutagenCanister) and is (known_type) and
         (!is (known_bugginess) or !isOptimized ()) and
         (!is (known_appearance) or myIlk ()->mMaterial != kLead));
}

/* Two radioactivity functions.  They should be updated together. */
bool
shObject::isRadioactive ()
{
    return getIlkFlag (kRadioactive) or
        (isA (kObjXRayGoggles) and isBuggy ()) or
        (isA (kObjMutagenCanister) and !isOptimized () and
            myIlk ()->mMaterial != kLead);
}

bool
shObject::isWeldedWeapon ()
{
    return false;
}

/* TODO: This is getting complicated and has potential for more.
   Object ilks could have flags telling when apply command is available. */
bool
shObject::isUseable ()
{
    return myIlk ()->mUseFunc and
        (isA (kTool) or isA (kCanister) or isA (kRayGun) or
         isA (kObjLightSaber) or
         (has_subtype (grenade) and !is (known_type)) or
         (isA (kArmor) and is (known_type) and is (worn)) or
         (isA (kWeapon) and is (known_type)) or
         (isA (kImplant) and is (known_type) and
          (is (worn) or isA (kObjMechaDendrites))));
}

bool
shObject::isUnique ()
{
    return getIlkFlag (kUnique);
}

bool
shObject::isUniqueKnown ()
{
    return getIlkFlag (kUnique) and is (known_type);
}

bool
shObject::isIndestructible ()
{
    return getIlkFlag (kIndestructible);
}

bool
shObject::isInfectable ()
{
    return isA (kFloppyDisk) or isComputer ();
}

bool
shObject::isBugProof ()
{
    return getIlkFlag (kBugProof);
}

bool
shObject::isFooproofable ()
{
    return vulnerability () != kNoEnergy and (isA (kArmor) or isA (kWeapon));
}

bool
shObject::isEnhanceable ()
{
    return myIlk ()->mMaxEnhancement;
}

bool
shObject::isChargeable ()
{
    return myIlk ()->mMaxCharges;
}

bool
shObject::isCrackable ()
{
    return isA (kFloppyDisk) or has_subtype (keycard) or
           isA (kObjRemoteControl);
}

bool
shObject::isSealedArmor ()
{
    return isA (kArmor) and getIlkFlag (kSealed);
}

bool
shObject::isPoweredArmor ()
{
    return isA (kArmor) and getIlkFlag (kPowered);
}

bool
shObject::isMeleeWeapon ()
{
    return myIlk ()->mMeleeAttack;
}

bool
shObject::isThrownWeapon ()
{
    return ((isA (kWeapon) or is (known_type)) and myIlk ()->mMissileAttack) or
           (isA (kObjProximityMine) and !is (known_type));
}

bool
shObject::isAimedWeapon ()
{
    return myIlk ()->mGunAttack or myIlk ()->mZapAttack;
}

bool
shObject::isSelectiveFireWeapon ()
{
    return isA (kWeapon) and getIlkFlag (kSelectiveFire);
}

bool
shObject::isKnownWeapon ()
{
    return isA (kWeapon) or (is (known_type) and
        (myIlk ()->mMeleeAttack or myIlk ()->mGunAttack or
         myIlk ()->mZapAttack));
}

bool
shObject::canBeDrunk () {
    return !!myIlk ()->mQuaffFunc;
}

void
shObject::setBuggy ()
{
    if (isBugProof ())  return;
    mBugginess = -1;
}

void
shObject::setDebugged ()
{
    if (isBugProof ())  return;

    bool wasbuggy = isBuggy ();
    mBugginess = 0;
    bool heroitem = wasbuggy and is (worn) and NOT0 (mOwner,->isHero ());
    if (!heroitem)
        return;
    if (has_subtype (skillsoft)) {
        I->p ("%s starts working!", YOUR (this));
        set (known_type | known_bugginess);
        announce ();
    }
}

void
shObject::setOptimized ()
{
    if (isBugProof ())  return;
    mBugginess = +1;
}

bool
shObject::is (unsigned int f) {
    return (mFlags & f) == f;
}

void
shObject::set (unsigned int f) {
    /* For torcs appearance or bugginess are as one. */
    if (mIlkId == kObjTorc and f & (known_appearance | known_bugginess))
        f |= known_appearance | known_bugginess;

    if (mIlkId == kObjComputerVirus and f & known_type)
        f |= known_infected;

    if ((f & known_type and mFlags & known_appearance) or
        (f & known_appearance and mFlags & known_type))
    {
        setIlkKnown ();
    }

    if (f & known_appearance and isIlkKnown ())
        f |= known_type;

    if (f & known_charges and isA (kRayGun) and !mCharges) {
        mIlkId = kObjEmptyRayGun;
        f |= known_type;
    }

    if (f & cracked and
        (mIlkId == kObjHackingDisk or mIlkId == kObjMatterCompiler))
    {
        f &= ~cracked;
    }

    if (f & active)
        mLastEnergyBill = Clock;

    mFlags |= f;
}

void
shObject::clear (unsigned int f) {
    /* For torcs appearance and bugginess are as one. */
    if (mIlkId == kObjTorc and
        ((f & (known_appearance | known_bugginess)) !=
              (known_appearance | known_bugginess)))
    {
        f &= ~(known_appearance | known_bugginess);
    }

    if (mIlkId == kObjComputerVirus and f & infected)
        mIlkId = kObjBlankDisk;

    if (f & known_bugginess and isBugProof ())
        f &= ~known_bugginess;

    if (f & active)
        mLastEnergyBill = MAXTIME;

    mFlags &= ~f;
}

void
shObject::toggle (unsigned int f) {
    unsigned int to_set = f & ~mFlags;
    unsigned int to_clear = f & mFlags;

    set (to_set);
    clear (to_clear);
}
