/* Matter compiler.  This one is huge for just a single floppy disk program. */

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Distribution.h"

/* Was the Bizarro Orgasmatron decompiled. */
static bool true_orgasmatron = false;

extern int hackingRoll (shObject *computer);

#define NOFEATURE shFeature::kMaxFeatureType
static const struct shIlkPickData
{
    int score;
    const char *dispName;
    prob::Table *category;
    shFeature::Type type;
} pickData[] =
{
    {0, "Relinquish control", NULL, NOFEATURE},

    {1,  "random item",            prob::NotMoney,        NOFEATURE},

    {0, "Armor", NULL, NOFEATURE},

    {15, "armor piece",            prob::Armor,           NOFEATURE},
    {20, "jumpsuit",               prob::Jumpsuit,        NOFEATURE},
    {20, "body armor",             prob::BodyArmor,       NOFEATURE},
    {20, "headwear",               prob::Helmet,          NOFEATURE},
    {23, "cloak",                  prob::Cloak,           NOFEATURE},
    {23, "boots",                  prob::Boots,           NOFEATURE},
    {25, "eyewear",                prob::Goggles,         NOFEATURE},
    {25, "belt",                   prob::Belt,            NOFEATURE},
    {28, "power armor",            prob::PowerArmor,      NOFEATURE},
    {30, "armor of chaos",         prob::ChaosArmor,      NOFEATURE},

    {0, "Weapons", NULL, NOFEATURE},

    {15, "weapon",                 prob::Weapon,          NOFEATURE},
    {20, "ammunition",             prob::Ammunition,      NOFEATURE},
    {20, "melee weapon",           prob::MeleeWeapon,     NOFEATURE},
    {20, "gun",                    prob::Gun,             NOFEATURE},
    {23, "rifle",                  prob::LightRifle,      NOFEATURE},
    {23, "sword",                  prob::Sword,           NOFEATURE},
    {25, "grenade",                prob::Grenade,         NOFEATURE},
    {22, "energy gun",             prob::EnergyGun,       NOFEATURE},
    {22, "conventional gun",       prob::ConvGun,         NOFEATURE},
    {26, "energy handgun",         prob::HandEnergyGun,   NOFEATURE},
    {26, "energy light gun",       prob::LightEnergyGun,  NOFEATURE},
    {26, "energy heavy gun",       prob::HeavyEnergyGun,  NOFEATURE},
    {26, "conventional handgun",   prob::HandConvGun,     NOFEATURE},
    {26, "conventional light gun", prob::LightConvGun,    NOFEATURE},
    {26, "conventional heavy gun", prob::HeavyConvGun,    NOFEATURE},
    {28, "cannon",                 prob::Cannon,          NOFEATURE},

    {0, "Tools", NULL, NOFEATURE},

    {17, "tool",                   prob::Tool,            NOFEATURE},
    {21, "keycard",                prob::Keycard,         NOFEATURE},
    {24, "gizmo",                  prob::Gizmo,           NOFEATURE},
    {28, "computer",               prob::Computer,        NOFEATURE},
    {30, "power plant",            prob::PowerPlant,      NOFEATURE},

    {0,  "Miscellaneous", NULL, NOFEATURE},

    {19, "beverage",               prob::Beverage,        NOFEATURE},
    {25, "canister",               prob::Canister,        NOFEATURE},
    {27, "ray gun",                prob::RayGun,          NOFEATURE},
    {29, "bionic implant",         prob::Implant,         NOFEATURE},

    {0,  "Features", NULL, NOFEATURE},

    {20, "door",                   NULL,      shFeature::kDoorOpen},
    {23, "radiation trap",         NULL,      shFeature::kRadTrap},
    {27, "sludge vat",             NULL,      shFeature::kVat}
};
static const int pickCount = sizeof (pickData) / sizeof (struct shIlkPickData);

/* Returns obtained weight. */
static int
decompileFeature (shCreature *c, shFeature *f)
{
    if (!f)  return 0;
    switch (f->mType) {
        case shFeature::kDoorOpen:
            I->p ("The door beside you is decompiled!");
            /* This encourages dungeon vandalism, I know. */
            Level->removeFeature (f);
            I->pause ();
            /* Nooo!  I needed that door! */
            if (Level->isInShop (c->mX, c->mY)) {
                c->damagedShop (c->mX, c->mY);
            }
            return 750;
        case shFeature::kVat:
            I->p ("The sludge vat next to you is decompiled!");
            Level->removeFeature (f);
            I->pause ();
            return 1500;
        case shFeature::kAcidPit:
            I->p ("Suddenly the acid pool beneath you is gone.");
            f->mType = shFeature::kPit;
            I->pause ();
            return 175;
        case shFeature::kRadTrap:
            I->p ("The rad trap you are standing on is decompiled!");
            if (c->isHero () and Hero.mProfession == Janitor) {
                I->p ("Nice cleaning job.");
                c->beatChallenge (mini (Level->mDLevel, 12));
            }
            Level->removeFeature (f);
            I->pause ();
            return 550;
        case shFeature::kSewagePit:
            /* Sewage decompilation handled by core routine. */
            f->mType = shFeature::kPit;
            if (c->mTrapped.mDrowning) {
                c->msg ("You can now breathe freely!");
                c->mTrapped.mDrowning = 0;
            }
            return 300; /* More sewage fits in deep spot. */
        //case shFeature::kComputerTerminal:
        default: /* Feature cannot be decompiled. */
            break;
    }
    return 0;
}

/* Returns 1 if hero stands between two walls and 0 otherwise. */
static int
isDoorSpot (int x, int y)
{
    return
    /* Horizontal. */
    ((Level->isObstacle (x-1, y) and
      Level->isObstacle (x+1, y)) or
    /* Vertical. */
     (Level->isObstacle (x, y-1) and
      Level->isObstacle (x, y+1)));
}

/* Returns true if hero is on a level where doors could be added. */
static bool
isDoorLevel (shMapLevel *l)
{
    return l->mMapType == shMapLevel::kBunkerRooms or l->isTownLevel ();
}

/* Returns ilk name and kNoFeature in kind or NULL and feature type in kind. */
/* Assumes creature calling is under player control. */
static prob::Table *
pickCompilerTarget (shCreature *c, int score, shFeature::Type *kind)
{
    shMenu *pickIlk = I->newMenu ("choose object type", 0);
    shFeature *f = Level->getFeature (c->mX, c->mY);
    char letter = 'a';
    const char *headerstr = NULL;
    int header_placed = 1;
    int options = 0;
    for (int i = 0; i < pickCount; ++i) {
        int skill_or_luck =
            (score >= pickData[i].score or // Through skill.
             !RNG (pickData[i].score));    // Through luck.
        if (pickData[i].score == 0) {
        /* Do not place header unless an object needs it. */
            headerstr = pickData[i].dispName;
            header_placed = 0;
        /* Give first (random) pick and others 80% of the time
           if other conditions are fulfilled. */
        } else if ((RNG (5) or !options) and skill_or_luck) {
            /* Offer features only if place is available. */
            if ((pickData[i].type != NOFEATURE and f == NULL and
                /* Offered feature is not a door or... */
                (pickData[i].type != shFeature::kDoorOpen or
                /* ...it matches both door requirements. */
                (isDoorLevel (c->mLevel) and isDoorSpot (c->mX, c->mY))))
                /* Objects always fine. */
                or (pickData[i].type == NOFEATURE))
            {
                if (!header_placed) {
                    pickIlk->addHeader (headerstr);
                    header_placed = 1;
                }
                pickIlk->addPtrItem (letter++, pickData[i].dispName, &pickData[i]);
                if (letter > 'z') letter = 'A';
                ++options;
            }
        }
    } /* Time to choose. */
    shIlkPickData *ilkdata = NULL;
    if (options > 1) { /* Have options besides completely random. */
        pickIlk->getPtrResult ((const void **) &ilkdata, NULL);
    }
    delete pickIlk;
    /* Hero gets or wants random item? */
    if (options == 1 or !ilkdata or ilkdata->category == pickData[1].category) {
        *kind = NOFEATURE;
        return NULL;
    } /* Return chosen feature or object. */
    *kind = ilkdata->type;
    return ilkdata->category;
}

/* Returns 1 if succesful and 0 otherwise. */
static int
compileObject (shCreature *c, prob::Table *table, int score, int *weight)
{
    shMenu *pickone = I->newMenu ("select one", 0);
    shObjectVector v;
    char letter = 'a';
    int choices = (score - 5) / 5;
    if (choices <= 0) choices = 1;
    /* Prepare choices and one backup item. */
    for (int i = 0; i < choices + 1; ++i) {
        shObject *obj = NULL;
        if (!table) { /* Choose fully randomly. */
            do {
                /* Money is boring.  Floppies are unbalancing.  Do not want. */
                if (obj)
                    delete obj;
                obj = generateObject ();
            } while (obj->isA (kObjMoney) or obj->isA (kFloppyDisk));
        } else { /* Choose from selected category. */
            obj = gen_obj_type (table);
        }

        /* Should not happen but if it does lets not ruin a good game. */
        if (!obj) {
            debug.log ("Failed to generate stuff.");
            I->p ("Suddenly something goes very wrong!");
            continue;
        }

        /* Items may be described with varying accuracy. */
        unsigned int to_set = 0;
        if ((score >= 15 and RNG (3)) or !RNG (10))
            to_set |= obj::known_appearance;
        if ((score >= 20 and RNG (5)) or !RNG (10))
            to_set |= obj::known_bugginess;
        if ((score >= 20 and RNG (5)) or !RNG (10))
            to_set |= obj::known_infected;
        if ((score >= 20 and RNG (5)) or !RNG (10))
            to_set |= obj::known_enhancement;
        obj->set (to_set);

        v.add (obj);
        if (i != choices) { /* Hide backup item. */
            if (!RNG (15)) { /* Sometimes give no description at all. */
                pickone->addPtrItem (letter++, "something", obj);
            } else {
                pickone->addPtrItem (letter++, obj->inv (), obj);
            }
        }
    } /* Make your pick. */
    shObject *chosen = NULL;
    int manual_pick = 1;
    if (choices > 1) {
        pickone->getPtrResult ((const void **) &chosen, NULL);
        /* Hero is not satisfied with options? Give backup item. */
        if (!chosen) {
            chosen = v.get (choices);
            manual_pick = 0;
        }
    } else {
        chosen = v.get (0);
    } /* Discard rest. */
    delete pickone;
    for (int i = 0; i < choices + 1; ++i) {
        shObject *obj = v.get (i);
        if (obj != chosen) delete obj;
    } /* Check weight. */
    if (chosen->myIlk ()->mWeight > *weight) {
        if (choices > 1) {
            I->p ("There is not enough particles left to assemble %s.",
                manual_pick ? chosen->inv () : "last item");
        }
        delete chosen;
        return 0;
    } else {
        if (!c->intr (kBlind))
            chosen->set (obj::known_appearance);
        Level->putObject (chosen, c->mX, c->mY);
        *weight -= chosen->myIlk ()->mWeight;
    }
    return 1;
}


static int
compileFeature (shCreature *c, shFeature::Type type, int score, int *weight)
{
    const struct shFeatureIlkData
    {
        shFeature * (*spawner) ();
        const char *sthstr;
        int weight;
    } featureData[] =
    {
        {&shFeature::newDoor, "a door", 1000},
        {&shFeature::newVat, "a vat", 4000}
    };
    shMenu *pickone = I->newMenu ("select one", 0);
    shVector<shFeature *> v;
    int row = -1;
    char letter = 'a';
    int choices = (score - 5) / 5;
    if (choices <= 0) choices = 1;
    /* Prepare choices based on type. */
    switch (type) {
        case shFeature::kRadTrap: /* Nothing to choose. */
            if (*weight < 650) {
                I->p ("You lack particles to assemble it.");
                return 0;
            }
            *weight -= 650;
            Level->addTrap (c->mX, c->mY, shFeature::kRadTrap);
            return 1;
        case shFeature::kDoorOpen: row = 0; break;
        case shFeature::kVat:      row = 1; break;
        default:
            I->p ("Unimplemented feature compilation.");
            return 0;
    }
    if (*weight < featureData[row].weight) {
        int lack = featureData[row].weight - *weight;
        I->p ("You fail to assemble %s.", featureData[row].sthstr);
        I->p ("You lack %s particles.",
            lack < 75 ? "just a few" :
            lack < 200 ? "a small amount of" :
            lack < 500 ? "a moderate amount of" :
            lack < 1000 ? "a large amount of" : "a huge amount of");
        return 0;
    }
    *weight -= featureData[row].weight;
    for (int i = 0; i < choices + 1; ++i) {
        shFeature *f = (*featureData[row].spawner) ();
        f->mX = c->mX;
        f->mY = c->mY;
        v.add (f);
        if (i != choices) { /* Hide last option. */
            if ((score >= 15 and RNG (3)) or !RNG (10)) {
                pickone->addPtrItem (letter++, featureData[row].sthstr, f);
            } else {
                pickone->addPtrItem (letter++, f->getDescription (), f);
            }
        }
    }
    /* Choose among generated features. */
    shFeature *chosen = NULL;
    if (choices > 1) {
        pickone->getPtrResult ((const void **) &chosen, NULL);
        /* Hero is not satisfied with options?  Give backup feature. */
        if (!chosen) {
            chosen = v.get (choices);
        }
    } else {
        chosen = v.get (0);
    }
    /* Discard rest. */
    delete pickone;
    for (int i = 0; i < choices + 1; ++i) {
        shFeature *obj = v.get (i);
        if (obj != chosen) delete obj;
    }
    Level->addFeature (chosen);
    return 1;
}

static int
decompileEquipment (shCreature *c, shObject *computer)
{
    int weight = 0;
    /* Held things first. */
    if (c->mWeapon) {
        if (c->mWeapon == computer) {
            c->msg ("Your computer transforms into dust.");
            return -1;
        }
        if (c->mWeapon->isA (kObjTheOrgasmatron)) {
            true_orgasmatron = true;
        }
        int massgain = c->mWeapon->getMass ();
        c->msg (fmt ("%s %s decompiled.  (+%d mass)", YOUR (c->mWeapon),
                c->mWeapon->mCount > 1 ? "are" : "is", massgain));
        c->useUpSomeObjectsFromInventory (c->mWeapon, c->mWeapon->mCount);
        weight += massgain;
    }
    /* Worn stuff next. */
    shObjectVector v;
    selectObjectsByFlag (&v, c->mInventory, obj::worn);
    int count = 0, massgain = 0;
    for (int i = 0; i < v.count (); ++i) {
        shObject *obj = v.get (i);
        if (obj->isA (kImplant) and
            obj->mIlkId != kObjTorc and obj->mIlkId != kObjMechaDendrites and
            obj->mIlkId != kObjBabelFish)
            continue; /* Implants residing in brain are safe. */
        ++count;
        massgain += obj->myIlk ()->mWeight;
        c->useUpOneObjectFromInventory (obj);
    }
    if (count) {
        c->msg (fmt ("Your armor is decompiled.  (+%d mass)", massgain));
        weight += massgain;
    }
    return weight;
}

static int
maybeFillPitOrHole (shCreature *c, shFeature *f, int unglued)
{
    if (!f)  return unglued;
    const int hole2ground = 500;
    const int hole2pit = 200;

    switch (f->mType) {
    case shFeature::kFloorGrating:
    case shFeature::kBrokenGrating:
        if (unglued >= hole2ground) {
            unglued -= hole2ground;
            c->msg ("A passage below the grating underneath you is sealed.");
            Level->removeFeature (f);
        }
        break;
    case shFeature::kHole:
        if (unglued >= hole2ground) {
            unglued -= hole2ground;
            c->msg ("The hole below you is sealed.");
            Level->removeFeature (f);
        } else if (unglued >= hole2pit) {
            c->msg ("The hole below you is partially filled.");
            f->mType = shFeature::kPit;
        }
        break;
    case shFeature::kPit:
        if (unglued >= hole2pit) {
            unglued -= hole2pit;
            Level->removeFeature (f);
            if (c->mZ == -1) {
                c->msg ("You are lifted as the pit you were in is sealed with matter.");
                c->mZ = 0;
                c->mTrapped.mInPit = 0;
                c->mTrapped.mDrowning = 0;
            } else {
                c->msg ("The pit below you is filled.");
            }
        }
        break;
    default:
        break;
    }
    return unglued;
}

static void
leftoverMatter (shCreature *c, int weight)
{
    int junk_wgt = AllIlks[kObjJunk].mWeight;
    int junk_count = weight / junk_wgt;

    if (junk_count) {
        c->msg ("Excess matter is haphazardly glued into heaps of junk.");
        for (int i = 0; i < junk_count; ++i) {
            shObject *junk = new shObject (kObjJunk);
            Level->putObject (junk, c->mX, c->mY);
        }
        weight -= junk_count * junk_wgt;
    }

    shFeature *f = Level->getFeature (c->mX, c->mY);
    weight = maybeFillPitOrHole (c, f, weight);

    if (!junk_count and weight > 0) {
        c->msg ("Leftover particles disperse.");
    }
}

/* Acquirement for PRIME. */
shExecResult
doMatterCompiler (shObject *computer, shObject *disk)
{
    shCreature *c = Hero.cr ();
    if (disk->isBuggy ()) {
        c->msg ("Your computer transforms into dust.");
        c->msg ("You've heard buggy matter compiler behaves like this.");
        if (c->isHero ())
            disk->set (obj::known_type);
        return kDestroyComputer;
    }
    int weight = 0; /* Summed weight of all decompiled objects. */
    true_orgasmatron = false;
    if (c->isHero () and !disk->is (obj::known_type)) {
        c->msg ("You have found a floppy disk of matter compiler!");
        disk->set (obj::known_type);
    }
    /* Proceed to decompile stuff. */
    if (c->is (kConfused)) {
        int ret = decompileEquipment (c, computer);
        if (ret == -1)
            return kDestroyComputer;
        weight += ret;
    }
    /* Decompile web hero might be entangled in. */
    if (c->mTrapped.mWebbed) {
        int gain = 15 * c->mTrapped.mWebbed;
        c->msg (fmt ("The web holding you is decompiled!  (+%d mass)", gain));
        c->mTrapped.mWebbed = 0;
        weight += gain;
    }
    /* Decompile duct tape hero might be held by. */
    if (c->mTrapped.mWebbed) {
        int gain = c->mTrapped.mTaped;
        c->msg (fmt ("The duct tape holding you is decompiled!  (+%d mass)", gain));
        weight += gain;
        c->mTrapped.mTaped = 0;
    }
    /* Decompile sewage. */
    if (Level->mSquares[c->mX][c->mY].mTerr == kSewage) {
        const int gain = 85;
        c->msg (fmt ("The sewage around you is decompiled!  (+%d mass)", gain));
        Level->mSquares[c->mX][c->mY].mTerr = kSewerFloor;
        /* Creature might be drowning in sewage pit,
          but this is handled by decompileFeature (). */
        weight += gain;
    }

    /* Decompile objects too. */
    shObjectVector *v = Level->getObjects (c->mX, c->mY);
    if (v) { /* Nuke whole stack of items. */
        for (int i = 0; i < v->count (); ++i) {
            shObject *obj = v->get (i);
            weight += obj->myIlk ()->mWeight;
            if (obj->isA (kObjTheOrgasmatron)) {
                true_orgasmatron = true;
            }
            if (obj->is (obj::unpaid)) {
                c->usedUpItem (obj, obj->mCount, "decompile");
            }
            delete obj;
        }
        delete v;
        Level->setObjects (c->mX, c->mY, NULL);
        if (v->count () > 1) {
            c->msg ("Items on ground are decompiled.");
        } else {
            c->msg ("Something on ground is decompiled.");
        }
    }

    /* Perhaps decompile some terrain feature? */
    if (Level->getFeature (c->mX, c->mY)) {
        weight += decompileFeature (c, c->mLevel->getFeature (c->mX, c->mY));
        if (c->mState == kDead)  return kNoExpiry;
    }

    /* Unavoidable weight loss.  Some particles escape. */
    if        (disk->is (obj::infected)) {
        weight = weight * 75 / 100;
    } else if (disk->isDebugged ()) {
        weight = weight * 90 / 100;
    } else if (disk->isOptimized ()) {
        weight = weight * 95 / 100;
    }

    /* (De)compilation should not be attempted in irradiated level/room. */
    if (Level->isRadioactive (c->mX, c->mY)) {
        weight /= 2;
        c->msg ("Particles behave in unstable way.  Half of them speed away.");
        if (c->isHero ())
            I->pause ();
    }

    int objs_created = 0;
    int feature_created = 0;
    int last_ok = 1;
    if (weight) { /* Create stuff! */
        int numitems = RNG (1, 3 + disk->mBugginess);
        while (numitems-- and weight and last_ok) {
            shFeature::Type kind = NOFEATURE;
            if (true_orgasmatron) { /* Give it back. */
                shObject *orgasmatron = new shObject (kObjTheOrgasmatron);
                Level->putObject (orgasmatron, c->mX, c->mY);
                true_orgasmatron = false;
                continue;
            }
            prob::Table *table = NULL;
            if (hackingRoll (computer) >= 20) {
                table = pickCompilerTarget (c, hackingRoll (computer), &kind);
            } else {
                if (weight >= 4000 and !RNG (20)) {
                    switch (RNG (2)) {
                        case 0: kind = shFeature::kRadTrap; break;
                        case 1: kind = shFeature::kVat; break;
                    }
                }
            }
            if (kind == NOFEATURE) { /* Generate an object. */
                last_ok = compileObject (c, table, hackingRoll (computer), &weight);
                if (!last_ok) break; /* Out of particles or an error. */
                objs_created += last_ok;
            } else {
                last_ok = compileFeature (c, kind, hackingRoll (computer), &weight);
                feature_created = last_ok;
            }
        }
    }

    /* Report results. */
    if (feature_created) {
        shFeature *f = Level->getFeature (c->mX, c->mY);
        f->mTrapUnknown = 0;
        c->msg (fmt ("%s forms.", f->getShortDescription ()));
    }
    if (objs_created > 1) {
        c->msg ("Some items form at your feet.");
    } else if (objs_created == 1) {
        c->msg ("An item forms at your feet.");
    }
    if (!objs_created and !feature_created) {
        c->msg ("Matter compilation failed entirely.");
    }

    /* Form heaps of junk, fill pits or holes. */
    leftoverMatter (c, weight);

    return kNormal;
}
#undef NOFEATURE
