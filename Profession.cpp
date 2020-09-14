#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Profession.h"
#include "Object.h"
#include "Creature.h"
#include "Mutant.h"
#include "Hero.h"
#include "Equipment.h"

shVector <shProfession *> Professions;
shProfession *SpaceMarine;
shProfession *Stormboy;
shProfession *Psion;
shProfession *Empath;
shProfession *Mastermind;
shProfession *Weirdboy;
shProfession *Ninja;
shProfession *Dissident;
shProfession *Janitor;
shProfession *SoftwareEngineer;
shProfession *Cracker;
shProfession *Quarterback;
shProfession *Astronaut;
shProfession *Yautja;
shProfession *XelNaga;

/* LoreHelp.cpp: */
extern const char *processLoreLine (const char *src, const char *arg = NULL);

shProfession::shProfession (
    const char *name,
    int hitdiesize,
    int numskills,
    int reflex,
    int will,
    shHeroInitFunction *f,
    shKnowledgeFunction *kf,
    const char *t1,
    const char *t2,
    const char *t3,
    const char *t4,
    const char *t5,
    const char *t6,
    const char *t7,
    const char *t8,
    const char *t9,
    const char *t10)
{
    mId = Professions.add (this);
    mName = name;
    mHitDieSize = hitdiesize;
    mNumPracticedSkills = numskills;
    mReflexSaveBonus = reflex;
    mWillSaveBonus = will;
    mInitFunction = f;
    examine_item = kf;
    mTitles[0] = t1;
    mTitles[1] = t2;
    mTitles[2] = t3;
    mTitles[3] = t4;
    mTitles[4] = t5;
    mTitles[5] = t6;
    mTitles[6] = t7;
    mTitles[7] = t8;
    mTitles[8] = t9;
    mTitles[9] = t10;
}

/* There were six metapsychic faculties in Julian May's books. */
static const int FAC = 6;

static const shObjId commonlore[] = {
    /* Everyone knows stormtroopers wear stormtrooper stuff.  Almost. */
    kObjStormtrooperHelmet,
    kObjStormtrooperSuit,
    kObjStormtrooperBoots,
    /* Same with aquamarine and mean green marines. */
    kObjAquamarinePH,
    kObjAquamarinePA,
    kObjMeanGreenPH,
    kObjMeanGreenPA,
    kObjDeepBluePA,
    /* Elves too. */
    kObjElvenJumpsuit,
    kObjElvenSpaceSuit,
    /* Standard issue weapons. */
    kObjBlasterRifle,
    kObjBlasterPistol,
    kObjPhaserPistol,
    kObjLaserRifle,
    kObjLaserPistol
};
static const int numcommonfacts = sizeof (commonlore) / sizeof (shObjId);

static const shObjId humanlore[] = {
    /* Humans and orcs recognize this stuff. */
    kObjKevlarHelmet,
    kObjKevlarJacket,
    kObjAsbestosJacket,
    kObjFlakJacket,
    kObjSpaceHelmet,
    kObjSpaceSuit,
    kObjSpaceBoots,
    kObjLaserCannon,
    kObjMiningLaser
};
static const int numhumanfacts = sizeof (humanlore) / sizeof (shObjId);

static const shObjId elflore[] = {
    kObjElvenJumpsuit,
    kObjElvenSpaceSuit,
    kObjElvenCA,
    kObjElvenEA,
    kObjElvenCH,
    kObjElvenEH,
    kObjElvenDagger,
    kObjElvenSword
};
static const int numelffacts = sizeof (elflore) / sizeof (shObjId);

static const shObjId orclore[] = {
    /* Orcs know these well. */
    kObjPowerClaw,
    kObjPeaShooter
};
static const int numorcfacts = sizeof (orclore) / sizeof (shObjId);

static const shObjId reticulanlore[] = {
    /* Reticulans recognize this stuff.  Humans usually do not. */
    kObjReticulanJumpsuit,
    kObjBioArmor,
    kObjMBioArmor0,
    kObjEBioArmor1,
    kObjEBioArmor2,
    kObjEBioArmor3,
    kObjZapGun,
    kObjZapBaton
};
static const int numreticulanfacts = sizeof (reticulanlore) / sizeof (shObjId);

static const shObjId yautjalore[] = {
    /* Predator stuff. */
    kObjPlateArmor,
    kObjBioMask,
    kObjCloakingBelt,
    kObjSmartDisc,
    kObjWristBlade,
    kObjCombiStick,
    kObjRazorWhip,
    kObjNetLauncher,
    kObjSpeargun,
    kObjPlasmaCaster,
    kObjMedicomp,
    kObjSatCom
};
static const int numyautjafacts = sizeof (yautjalore) / sizeof (shObjId);

const shObjId soldierlore[] = {
    kObjPowerFist,
    kObjLaserCannon,
    kObjPlasmaPistol,
    kObjPlasmaRifle,
    kObjPlasmaCannon,
    kObjFlare
};
static const int numsoldierfacts = sizeof (soldierlore) / sizeof (shObjId);

static void
addKnowledge (const shObjId list[], int count)
{
    for (int i = 0; i < count; ++i)
        AllIlks[list[i]].mFlags |= kIdentified;
}

static void
addStartingEquipment (shCreature *hero, const EquipmentLine *table)
{
    int start_count = hero->mInventory->count ();
    equip_by_list (hero, table);

    shObject *last_prepared = NULL;
    shObject *designated = NULL;
    for (int i = start_count; i < hero->mInventory->count (); ++i) {
        shObject *obj = hero->mInventory->get (i);

        obj->identify ();
        if (hero->isOrc () and !obj->isBugProof ()) {
            /* Orcs don't care as much. */
            obj->clear (obj::known_bugginess);
        }
        if (obj->canBeWorn () and !obj->isComputer ()) {
            hero->don (obj, 1);
        } else if (obj->isA (kWeapon) and !obj->has_subtype (grenade)) {
            if (last_prepared)
                last_prepared->clear (obj::prepared);
            if (hero->mWeapon)
                last_prepared = hero->mWeapon;
            hero->wield (obj, 1);
            if (last_prepared)  last_prepared->set (obj::prepared);
        } else if (obj->isComputer () and !designated) {
            obj->set (obj::designated);
            designated = obj;
            if (hero->mCloak and hero->mCloak->isA (kObjPlasmaCaster)) {
                if (last_prepared)
                    last_prepared->clear (obj::prepared);
                obj->set (obj::prepared);
                last_prepared = obj;
            }
        }
    }
    hero->reorganizeInventory ();
}

struct skillData
{
    shSkillCode skill;
    int access;
    int ranks;
};

static void
addSkills (shCreature *hero, const skillData spec[], int count)
{
    int faculty[FAC] = {-1, -1, -1, -1, -1, -1};
    /* Base skills: combat, adventure, metapsychic. */
    for (int i = 0; i < count; ++i) {
        hero->addSkill (spec[i].skill, spec[i].access);
        if (spec[i].ranks)
            hero->gainRank (spec[i].skill, spec[i].ranks);
        if ((spec[i].skill & kSkillTypeMask) == kMetapsychicFaculty)
            faculty[(spec[i].skill & kSkillNumberMask)] = spec[i].access;
    }
    /* Mutant power skills have access corresponding to their faculty.
       The skills are never sorted thus it is important to inser them grouped
       by faculty. */
    for (int f = 0; f < FAC; ++f) {
        int fac_acc = faculty[f];
        for (int i = kNoMutantPower + 1; i < kMaxHeroPower; ++i) {
            /* Look only at powers belonging to current faculty. */
            if ((MutantPowers[i].mSkill & kSkillNumberMask) != f)  continue;
            /* Mutant power skills are granted if one has the faculty skill. */
            if (fac_acc != -1) {
                shSkillCode code = MutantPowers[i].getCode ();
                hero->addSkill (code, fac_acc, shMutantPower (i));
            }
        }
    }
}

static void
addRandomPower (shCreature *hero, int faculty, int points)
{
    shVector<shMutantPower> v;
    shSkillCode skill = shSkillCode (faculty | kMFCoercion);
    for (int i = 0; i < kMaxHeroPower; ++i)
        /* Power must match faculty and be low level. */
        if (MutantPowers[i].mLevel < 5 and MutantPowers[i].mSkill == skill and
            /* Hero must not have this one already. */
            hero->mMutantPowers[i] == MUT_POWER_ABSENT)
            v.add (shMutantPower (i));
    if (!v.count ()) {
        I->p ("Error: cannot assign mutant power from faculty %s.",
              hero->getSkill (skill)->getName ());
        return;
    }
    /* Pick one at random. */
    shMutantPower pow = v.get (RNG (v.count ()));
    hero->getMutantPower (pow, 1);
    hero->gainRank (MutantPowers[pow].getCode (), points, pow);
}

/* Returns true if power was granted. */
static bool
addPower (shCreature *hero, shMutantPower power, int points)
{
    if (power == kNoMutantPower)
        return false;
    if (hero->mMutantPowers[power] != MUT_POWER_ABSENT)
        return false;
    hero->getMutantPower (power, 1);
    hero->gainRank (MutantPowers[power].getCode (), points, power);
    return true;
}

static void
ensureAbil (shCreature *hero, abil::Index idx, int minimum)
{
    int score = hero->mAbil.base (idx);
    if (score < minimum)
        hero->mAbil.perm_mod (idx, minimum - score);
}

static void
showIntro (shCreature *hero, const char *introLines[], int count)
{
    char *buf = GetBuf ();
    if (hero->isOrc ()) {
        snprintf (buf, SHBUFLEN, "Hai %s!", hero->mName);
    } else {
        snprintf (buf, SHBUFLEN, "Greetings, %s the %s.",
                  hero->mName, Hero.mProfession->mName);
    }
    shMenu *intro = I->newMenu (buf, shMenu::kNoPick);
    for (int i = 0; i < count; ++i) {
        const char *line = processLoreLine (introLines[i]);
        intro->addPtrItem (' ', line, NULL);
    }
    intro->finish ();
    delete intro;
}

static void
initSoftwareEngineer (shCreature *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 4;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = SoftwareEngineer;
    hero->rollAbilityScores (8, 8, 8, 12, 13, 6);

    hero->mMaxHP = SoftwareEngineer->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       2, 0},   {kHandgun,       3, 0},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 0, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         1, 0},

        {kConcentration, 2, 0},   {kOpenLock,      3, 0},
        {kRepair,        3, 0},   {kSearch,        3, 0},
        {kHacking,       4, 2},   {kSpot,          2, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);

    static const EquipmentLine stuff[] =
    {
        {kObjPeaShooter, "b=0 +0"},
        {kObjOrdinaryJumpsuit, "b=0 +0"},
        {kObjMiniComputer, "b=0 !p !i +0"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "66% !i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "33% !i !c s"},
        {kObjCoffee, "1d2 b=0"},
        {kObjDuctTape, "1d2 b=0"},
        {kObjRestrainingBolt, "b=0"},
        {kObjEnergyCell, "200"},
        {kObjMoney, "2d100"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
know_software (shObject *obj)
{
    if (obj->isA (kFloppyDisk) or obj->has_subtype (computer))
        obj->set (obj::known_bugginess);
}
/* Preparation for orcish version of software engineer. */
/*
static void
initCracker (shCreature *hero)
{
    hero->mIlkId = kMonSpaceOrc;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 4;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Cracker;
    hero->rollAbilityScores (8, 8, 8, 12, 13, 6);

    hero->mMaxHP = Cracker->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       2, 0},   {kHandgun,       3, 0},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 1, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         0, 0},

        {kConcentration, 2, 0},   {kOpenLock,      3, 0},
        {kRepair,        3, 0},   {kSearch,        3, 0},
        {kHacking,       4, 2},   {kSpot,          2, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (orclore, numorcfacts);

    static const EquipmentLine stuff[] =
    {
        {kObjPeaShooter, "+0"},
        {kObjOrdinaryJumpsuit, "+0"},
        {kObjMiniComputer, "b=0 !p !i +0"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "66% !i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "33% !i !c s"},
        {kObjCoffee, "1d2"},
        {kObjDuctTape, "1d2"},
        {kObjRestrainingBolt, NULL},
        {kObjEnergyCell, "200"},
        {kObjMoney, "2d50"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
know_piracy (shObject *obj)
{
    if (obj->isA (kFloppyDisk) or obj->has_subtype (computer))
        obj->set (obj::known_infected | obj::known_cracked | obj::known_fooproof);
}
*/

static void
initSpaceMarine (shCreature *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 5;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = SpaceMarine;
    hero->rollAbilityScores (13, 12, 9, 11, 7, 8);
    ensureAbil (hero, abil::Str, 10);

    hero->mMaxHP = SpaceMarine->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       3, 2},   {kHandgun,       4, 2},
        {kLightGun,      4, 2},   {kHeavyGun,      4, 2},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         1, 0},

        {kConcentration, 0, 0},   {kOpenLock,      0, 0},
        {kRepair,        2, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (soldierlore, numsoldierfacts);

    const EquipmentLine stuff[] =
    {
        {kObjEnergyCell, "100"},
        {kObjBullet, "99"},
        {kObjFlare, "3d2 b=0"},
        {kObjFlakJacket, "+1 b=0"},
        {kObjConvPistol, "+0 b=0"},
        {kObjPulseRifle, "+0 b=0 !t"},
        {kObjMotionTracker, "b=0"},
        {kObjMoney, "2d10"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);

    const char *welcome[] =
    {
    "Two weeks. Two weeks trapped inside that damned cargo pod with only",
    "the requisition clerkbot to keep company. And the terrors outside. You",
    "told him. YOU TOLD HIM. Don't buy those damn \"krab patties\", Johnny.",
    "You don't know what that \"krab\" is, you never know with these backwater",
    "agriworlds! He laughed on you. You told them. I wouldn't eat those damn",
    "\"krab patties\" guys, Johnny bought'em off a weird looking yellow guy",
    "in a fishbowl! They scorned you, and their laughter echoed through the",
    "mess hall. Go eat your frozen earth shitburgers, they said. You're being",
    "hypochondriac, these krab patties are awesome - but don't worry, I'll",
    "take yours. And they laughed.",
    "",
    "WELL WHO'S LAUGHING NOW WITH HIS GUTS SPRAYED ALL OVER THE DINNER TABLE,",
    "JOHNNY?! WHO'S LAUGHING WITH HIS LUNGS OOZING OUT OF HIS EXPLODED CHEST?!!",
    "",
    "Not me! I'm getting out of this graveyard ship, and calling back to command,",
    "however long it takes. I'm going to be the most frickin' condecorated marine",
    "this side of the Perdus rift. And your sorry asses will rot in that",
    "MOTHERFUCKING DEATH TRAP FOREVER!!!",
    "",
    "So long, bastards.",
    "",
    "~*heroname*"
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}


static void
initStormboy (shCreature *hero)
{
    hero->mIlkId = kMonSpaceOrc;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 13;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Stormboy;
    hero->rollAbilityScores (13, 12, 9, 11, 7, 8);
    ensureAbil (hero, abil::Str, 10);

    hero->mMaxHP = Stormboy->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       3, 2},   {kHandgun,       3, 0},
        {kLightGun,      4, 2},   {kHeavyGun,      4, 2},
        {kUnarmedCombat, 4, 2},   {kMeleeWeapon,   3, 0},
        {kSword,         1, 0},

        {kConcentration, 0, 0},   {kOpenLock,      0, 0},
        {kRepair,        2, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (orclore, numorcfacts);
    addKnowledge (soldierlore, numsoldierfacts);

    const EquipmentLine stuff[] =
    {
        {kObjEnergyCell, "50"},
        {kObjIncendiaryGrenade, "2"},
        {kObjPrussianHelmet, "+0"},
        {kObjFlakJacket, "+0"},
        {kObjSniperRifle, "+0"},
        {kObjBullet, "30"},
        {kObjMoney, "2d10"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
know_soldier (shObject *obj)
{
    if (obj->isA (kWeapon) and
        (obj->myIlk ()->mGunSkill == kHandgun or
         obj->myIlk ()->mGunSkill == kLightGun or
         obj->myIlk ()->mGunSkill == kHeavyGun) and
        obj->myIlk ()->mAmmoType != kObjEnergyCell)
    {
        obj->set (obj::known_enhancement);
        obj->set (obj::known_bugginess);
    }
}

static void
initYautja (shCreature *hero)
{
    hero->mIlkId = kMonYautja;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 11;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Yautja;
    hero->rollAbilityScores (12, 15, 8, 10, 10, 5);
    ensureAbil (hero, abil::Con, 12);

    hero->mMaxHP = Yautja->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    hero->mInnateIntrinsics.set (kJumpy, true);

    const skillData skills[] =
    {
        {kGrenade,       4, 0},   {kHandgun,       3, 0},
        {kLightGun,      3, 0},   {kHeavyGun,      3, 0},
        {kUnarmedCombat, 4, 0},   {kMeleeWeapon,   4, 0},
        {kSword,         1, 0},

        {kConcentration, 0, 0},   {kOpenLock,      0, 0},
        {kRepair,        2, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          2, 1},

        {kMFCoercion,    1,-2},   {kMFCreativity,  1,-2},
        {kMFFarsenses,   1,-2},   {kMFRedaction,   1,-2},
        {kMFTelekinesis, 1,-2},   {kMFTranslation, 1,-2}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (yautjalore, numyautjafacts);

    const EquipmentLine ranged[] =
    {
        {kObjNetLauncher, "b=0 +0"},
        {kObjWebCasket, "8d4"},
        {kObjNothing, NULL},
        {kObjPlasmaCaster, "b=0 +0"},
        {kObjEnergyCell, "200"},
        {kObjNothing, NULL},
        {kObjSpeargun, "b=0 +0"},
        {kObjSpearheadDart, "20d2"},
        {kObjNothing, NULL},
        {kObjSmartDisc, "b=0 +0"},
        {kObjRecallDisk, "b=0 !c !i"},
        {kObjNothing, NULL}
    };
    int rngwpn = RNG (4);
    addStartingEquipment (hero, &ranged[rngwpn * 3]);
    hero->gainRank (rngwpn <= 1 ? kHandgun :
                    rngwpn == 2 ? kLightGun : kGrenade, 2);

    const EquipmentLine melee[] =
    {
        {kObjWristBlade, "b=+1 +0"},
        {kObjNothing, NULL},
        {kObjRazorWhip, "b=0 +0"},
        {kObjNothing, NULL},
        {kObjCombiStick, "b=0 +0"},
        {kObjNothing, NULL}
    };
    int melwpn = RNG (3);
    addStartingEquipment (hero, &melee[melwpn * 2]);
    hero->gainRank (melwpn == 0 ? kUnarmedCombat : kMeleeWeapon, 2);

    const EquipmentLine stuff[] =
    {
        {kObjPlateArmor, "b=0 +0"},
        {kObjBioMask, "b=0 +0"},
        {kObjCloakingBelt, "b=0"},
        {kObjEnergyCell, "50"},
        {kObjGloryDevice, "b=-1"},
        {kObjSatCom, "b=0 !p !i +0"},
        {kObjMedicomp, "b=0 c:40"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
know_yautja (shObject *obj)
{
    for (int i = 0; i < numyautjafacts; ++i)
        if (obj->mIlkId == yautjalore[i])
            obj->identify ();
}

static void
initQuarterback (shCreature *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 10;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Quarterback;
    hero->rollAbilityScores (12, 10, 13, 12, 7, 10);

    hero->mMaxHP = Quarterback->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       4, 2},   {kHandgun,       2, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      2, 0},
        {kUnarmedCombat, 4, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         2, 0},

        {kConcentration, 2, 0},   {kOpenLock,      0, 0},
        {kRepair,        0, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          4, 2},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    const shObjId drinks[] = {
        kObjNanoCola,
        kObjNukaCola,
        kObjB3
    };

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (drinks, 3);

    const EquipmentLine stuff[] =
    {
        {kObjFootballPads, "b=0 +0"},
        {kObjFootballHelmet, "b=0 +0"},
        {kObjFootball, "1 +3 b=0"},
        {kObjEnergyCell, "200"},
        {kObjBeer, "6 b=0"},
        {kObjMoney, "3d100"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);

    const char *welcome[] =
    {
    "You were thrown into high security prison for beating up your country's sport",
    "and recreation minister when he declared funds for national football",
    "representations are to be cut in half.  You were there when he made the public",
    "announcement.  Everyone was enraged at him, yet seemingly nobody could do",
    "anything about his evil deeds.  Really nobody?  Your nerves snapped. That was",
    "an act on the spot. This was an act of honor! You dashed through police cordon",
    "with help of the angry mob.  Next you broke through line of security officers",
    "with aid of your trusted few football co-stars that made it that far.  Finally,",
    "you got to the minister himself.  Boy, did you squash him.  After just a few",
    "seconds of personal contact you left him with a bunch of teeth less and lying",
    "puddle of his own blood.",
    "",
    "Then the security used drastic measures to \"pacify\" the crowd.",
    "Tear gas, tazers, rubber bullets, stun grenades and clubbing.",
    "",
    "`You still don't remember?`  The police officer asks mockingly.",
    "`Well *heroname*, for you there are two options,` he continues.",
    "`The electric chair...`, he grins smugly,",
    "`...or something that generates huge viewership.  You could survive in theory.`",
    "The officer bursts into laughter proud of his own joke.",
    "",
    "A prison ship shall drop you off on a lone asteroid.  Mysterious \"they\"",
    "will be tracking your progress by a satellite as you quest for the Bizarro",
    "Orgasmatron.  So far this programme has just generated bloody deaths of ",
    "so-called \"main stars\" at hands of monsters.  On the other hand you heard",
    "there will be reticulans nearby.  Better for them *not* to be cops ...",
    "because someone certainly *will* suffer your accumulated wrath."
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}

static void
know_quarterback (shObject *obj)
{
    if (obj->isA (kWeapon) and
       !obj->has_subtype (sword) and
        obj->myIlk ()->mGunSkill == kNoSkillCode and
        obj->myIlk ()->mGunAttack == kAttDummy and
        obj->myIlk ()->mZapAttack == kAttDummy)
    {
        obj->set (obj::known_bugginess);
        obj->set (obj::known_enhancement);
    }

    if (obj->isA (kObjBeer))
        obj->set (obj::known_bugginess);
}

static void
initPsion (shCreature *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 7;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Psion;
    hero->rollAbilityScores (10, 7, 9, 10, 7, 15);
    ensureAbil (hero, abil::Psi, 14);

    hero->mMaxHP = Psion->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       2, 1},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 1, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         3, 0},

        {kConcentration, 4, 0},   {kOpenLock,      1, 0},
        {kRepair,        1, 0},   {kSearch,        3, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0},

        {kMFCoercion,    4, 0},   {kMFCreativity,  4, 0},
        {kMFFarsenses,   4, 0},   {kMFRedaction,   4, 0},
        {kMFTelekinesis, 4, 0},   {kMFTranslation, 4, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    AllIlks[kObjMutagenCanister].mFlags |= kIdentified;

    const shMutantPower preference[FAC] =
    {
        kMentalBlast,
        kOpticBlast,
        kXRayVisionPower,
        kRestoration,
        kKinesis,
        kNoMutantPower
    };

    /* Psion gets two semi-random powers. */
    const int skill_pts = 2;
    int fac1 = RNG (FAC);
    if (!addPower (hero, preference[fac1], skill_pts))
        addRandomPower (hero, fac1, skill_pts);
    /* Maximum of one power from Translation faculty. */
    int fac2 = fac1 == 5 ? RNG (FAC - 1) : RNG (FAC);
    if (!addPower (hero, preference[fac2], skill_pts))
        addRandomPower (hero, fac2, skill_pts);

    const EquipmentLine stuff[] =
    {
        {kObjLaserPistol, "+1 b=0"},
        {kObjOrdinaryJumpsuit, "+1 b=0"},
        {kObjSunglasses, "b=0"},
        {kObjEnergyCell, "200"},
        {kObjB3, "2 b=0"},
        {kObjHealingCanister, "1 b=0"},
        {kObjRadAway, "1 b=0"},
        {kObjMoney, "2d10"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
initEmpath (shCreature *hero)
{
    hero->mIlkId = kMonSpaceElf;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 15;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Empath;
    hero->rollAbilityScores (10, 7, 10, 7, 9, 15);
    ensureAbil (hero, abil::Psi, 14);

    hero->mMaxHP = Empath->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       1, 0},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 1, 0},   {kMeleeWeapon,   2, 1},
        {kSword,         3, 0},

        {kConcentration, 4, 0},   {kOpenLock,      1, 0},
        {kRepair,        1, 0},   {kSearch,        3, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0},

        {kMFCoercion,    4, 0},   {kMFCreativity,  4, 0},
        {kMFFarsenses,   4, 0},   {kMFRedaction,   4, 0},
        {kMFTelekinesis, 4, 0},   {kMFTranslation, 4, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (elflore, numelffacts);
    AllIlks[kObjMutagenCanister].mFlags |= kIdentified;

    const shMutantPower preference[FAC] =
    {
        kNoMutantPower,
        kNoMutantPower,
        kTremorsensePower,
        kHaste,
        kWarFlow,
        kNoMutantPower
    };

    const int skill_pts = 2;
    addPower (hero, kWarFlow, skill_pts);
    int fac = RNG (FAC);
    if (!addPower (hero, preference[fac], skill_pts))
        addRandomPower (hero, fac, skill_pts);

    /* Activate war flow immediately. */
    hero->mMutantPowers[kWarFlow] = MUT_POWER_ON;
    hero->mAbil.perm_mod (abil::Psi, -2);

    const EquipmentLine stuff[] =
    {
        {kObjElvenDagger, "+2 b=+1"},
        {kObjElvenJumpsuit, "+0 b=0"},
        {kObjEnergyCell, "100"},
        {kObjNanoCola, "2 b=0"},
        {kObjHealingCanister, "2 b=0"},
        {kObjRadAway, "1 b=0"},
        {kObjMoney, "2d10"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
initMastermind (shCreature *hero)
{
    hero->mIlkId = kMonTallGreyAlien;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 3;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Mastermind;
    hero->rollAbilityScores (7, 7, 9, 10, 10, 15);
    ensureAbil (hero, abil::Psi, 14);

    hero->mMaxHP = Mastermind->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       2, 1},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 1, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         3, 0},

        {kConcentration, 4, 0},   {kOpenLock,      1, 0},
        {kRepair,        1, 0},   {kSearch,        3, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0},

        {kMFCoercion,    4, 0},   {kMFCreativity,  4, 0},
        {kMFFarsenses,   4, 0},   {kMFRedaction,   4, 0},
        {kMFTelekinesis, 4, 0},   {kMFTranslation, 4, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (reticulanlore, numreticulanfacts);
    AllIlks[kObjMutagenCanister].mFlags |= kIdentified;

    const shMutantPower preference[FAC] =
    {
        kCauseFear,
        kImbue,
        kTelepathyPower,
        kNoMutantPower,
        kNoMutantPower,
        kNoMutantPower
    };

    const int skill_pts = 2;
    addPower (hero, kTelepathyPower, skill_pts);
    int fac = RNG (FAC);
    if (!addPower (hero, preference[fac], skill_pts))
        addRandomPower (hero, fac, skill_pts);

    /* Turn telepathy on immediately. */
    hero->mMutantPowers[kTelepathyPower] = MUT_POWER_ON;
    hero->mAbil.perm_mod (abil::Psi, -2);
    hero->telepathy (1);

    const EquipmentLine stuff[] =
    {
        {kObjAnalProbe, "+0 b!-1"},
        {kObjReticulanJumpsuit, "+0 b=0"},
        {kObjEnergyDome, "+0 b=0"},
        {kObjZapGun, "+1 b=+1"},
        {kObjEnergyCell, "150"},
        {kObjFullHealingCanister, "1 b=+1"},
        {kObjRadAway, "1 b=0"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
initWeirdboy (shCreature *hero)
{
    hero->mIlkId = kMonSpaceOrc;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 14;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Weirdboy;
    hero->rollAbilityScores (10, 10, 9, 7, 7, 15);
    ensureAbil (hero, abil::Psi, 14);

    hero->mMaxHP = Empath->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       1, 0},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 1, 0},   {kMeleeWeapon,   2, 1},
        {kSword,         3, 0},

        {kConcentration, 4, 0},   {kOpenLock,      1, 0},
        {kRepair,        1, 0},   {kSearch,        3, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0},

        {kMFCoercion,    4, 0},   {kMFCreativity,  4, 0},
        {kMFFarsenses,   4, 0},   {kMFRedaction,   4, 0},
        {kMFTelekinesis, 4, 0},   {kMFTranslation, 4, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (orclore, numorcfacts);
    AllIlks[kObjMutagenCanister].mFlags |= kIdentified;

    const shMutantPower preference[FAC] =
    {
        kNoMutantPower,
        kPsiVomit,
        kImbue,
        kAdrenalineControl,
        kCharge,
        kNoMutantPower
    };

    const int skill_pts = 2;
    addPower (hero, kPsiVomit, skill_pts);
    int fac = RNG (FAC);
    if (!addPower (hero, preference[fac], skill_pts))
        addRandomPower (hero, fac, skill_pts);

    const EquipmentLine stuff[] =
    {
        {kObjPrussianHelmet, "+0"},
        {kObjCopperStick, "+1"},
        {kObjOrdinaryJumpsuit, "+0"},
        {kObjEnergyCell, "100"},
        {kObjNanoCola, "2 b!-1"},
        {kObjHealingCanister, "1 b!-1"},
        {kObjRadAway, "1 b=0"},
        {kObjMoney, "2d10"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
know_psyker (shObject *obj)
{
    if (obj->is (obj::known_appearance) and
        (obj->mIlkId == kObjMutagenCanister or obj->mIlkId == kObjRadAway))
    {
        obj->set (obj::known_bugginess);
    }
}

static void
initJanitor (shCreature *hero)
{
    hero->mIlkId = kMonSpaceOrc;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 6;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Janitor;
    hero->rollAbilityScores (10, 12, 10, 12, 8, 8);

    hero->mMaxHP = Janitor->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       1, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   4, 0},
        {kSword,         2, 0},

        {kConcentration, 2, 0},   {kOpenLock,      0, 0},
        {kRepair,        4, 2},   {kSearch,        4, 2},
        {kHacking,       0, 0},   {kSpot,          4, 2},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (orclore, numorcfacts);

    const EquipmentLine stuff[] =
    {
        {kObjMop, "+1"},
        {kObjJanitorUniform, "+0"},
        {kObjSturdyBoots, "+0"},
        {kObjSuperGlue, NULL},
        {kObjMonkeyWrench, "+0 b=+1"},
        {kObjMasterKeycard, "b!-1"},
        {kObjRestrainingBolt, NULL},
        {kObjEnergyCell, "75"},
        {kObjMoney, "1d100"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);

    const char *welcome[] =
    {
    "Oi kusin Noshkop!",
    "",
    "Ya recall dat blabbing bout putting ol' Skragashuv back in biznes?",
    "",
    "Well me found one of dem Ur Magnetrons ya need for the warp",
    "gubbinz to rev up.",
    "",
    "I gab it all cleverly thott.",
    "",
    "Dere's dis Space Base, wiv all sorta loot, an a Ur Mangegubbin",
    "sittin' all alone in da bottom. An dey lookin' fer a jenitar!",
    "",
    "Dis 'janiter' is s'posed to go into dis 'ulk an get rid of any",
    "panzees, beekees, roaches sittin' dere an clean up all da mess",
    "dem making in the 'ulk.",
    "",
    "Sum one to \"clean da mess up\"! Got it!?",
    "",
    "Im sure gonna lick da place clean. I Can go anywhere I like cos",
    "I'm some sorta junitor, anything dats not bolted to da floor is",
    "mine, an nobody's messin wiv meh cos I'm a ork."
    "",
    "So you start savin' dem teef cus Im a bringin dat magnefing.",
    "",
    "Oh if you'z not up fer it, dun worry, Ill just clobber you up da",
    "head so bad you'll be seein stars by day but no bad feelins kusin.",
    "",
    "~*heroname*"
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}

static void
know_janitor (shObject *obj)
{
    if ((obj->has_subtype (keycard) and obj->isBuggy ()) or
         obj->has_subtype (gizmo))
    {
        obj->set (obj::known_bugginess);
    }

    if (obj->mIlkId == kObjMonkeyWrench)
        obj->identify ();
}

static void
initAstronaut (shCreature *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 8;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Astronaut;
    hero->rollAbilityScores (8, 10, 10, 12, 12, 10);

    hero->mMaxHP = Astronaut->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       3, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 3, 1},   {kMeleeWeapon,   1, 0},
        {kSword,         0, 0},

        {kConcentration, 2, 0},   {kOpenLock,      1, 0},
        {kRepair,        3, 1},   {kSearch,        3, 2},
        {kHacking,       2, 0},   {kSpot,          4, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    const shObjId proflore[] = {
        kObjFissionPlant,
        kObjFusionPlant,
        kObjGeigerCounter
    };
    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (proflore, 3);

    const EquipmentLine stuff[] =
    {
        {kObjRadiationSuit, "+0 b=0"},
        {kObjSpaceHelmet, "+0 b=0"},
        {kObjSpaceBoots, "+0 b=0"},
        {kObjSpaceSuit, "-3 b=0"},
        {kObjAntimatter, "b=+1"},
        {kObjHealingCanister, "b=+1"},
        {kObjWater, "b=+1"},
        {kObjSmallEnergyTank, "c:200"},
        {kObjTricorder, "b=+1"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);

    const char *welcome[] =
    {
        "You had been tasked with the first test flight of the Quantum IV",
        "hyperspace engine.  Just as you flew clear of any gravity wells, you",
        "deccelerated and began recording the experiment logs.  Main power",
        "diverted to the prototype engine, all systems green.  All systems?  No!",
        "The flux capacitor displays in red!  Without a second thought, you fasten",
        "your space helmet on, and with an adept kick you deftly propel yourself",
        "to an emergency pod, all the while screaming OH SHIT OH SHIT OH SHIT",
        "at the top of your lungs.",
        "",
        "From a couple miles away, you see the spaceship liquefy and fold into",
        "itself as it's sucked away from within by some sort of warp-hole.",
        "You broadcast what you think are your last words, full with excrement",
        "slang and considerations about the dubious parentage and upbringing",
        "of the shipyard CEO, among other colorful expletives.",
        "",
        "After passing out from a panic attack, and through the warp rift, you find",
        "yourself orbiting a derelict asteroid turned space station.  With nothing",
        "to lose except your life, you dock and set out to explore its bowels."
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}

static void
initDissident (shCreature *hero)
{
    hero->mIlkId = kMonLittleGreyAlien;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 1;
    hero->mGlyph.mTileY = 0;

	Hero.mProfession = Dissident;
	hero->rollAbilityScores (8, 6, 10, 14, 16, 8);
	hero->mMaxHP = Dissident->mHitDieSize +
		ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       2, 0},   {kHandgun,       1, 0},
        {kLightGun,      3, 1},   {kHeavyGun,      3, 0},
        {kUnarmedCombat, 0, 0},   {kMeleeWeapon,   2, 1},
        {kSword,         0, 0},

        {kConcentration, 1, 0},   {kOpenLock,      2, 0},
        {kRepair,        2, 0},   {kSearch,        2, 0},
        {kHacking,       2, 1},   {kSpot,          3, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (reticulanlore, numreticulanfacts);

    const EquipmentLine stuff[] =
    {
        {kObjBioArmor, "+0 b=+1"},
        {kObjStabilizerBelt, "b=0"},
        {kObjBioComputer, "b=0 !p !i +0"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::FloppyDisk, "!i !c s"},
        {(uintptr_t)&prob::Grenade, "3 b=0 s"},
        {kObjLaserRifle, "+0 b=0"},
        {kObjEnergyCell, "200"},
        {kObjZapBaton, "+1 b=0"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);
}

static void
know_dissident (shObject *obj)
{
    if (obj->isA (kWeapon) and
        (obj->myIlk ()->mGunSkill == kLightGun or
         obj->myIlk ()->mGunSkill == kHeavyGun) and
        obj->myIlk ()->mAmmoType == kObjEnergyCell)
    {
        obj->set (obj::known_enhancement);
        obj->set (obj::known_bugginess);
    }

    if (obj->myIlk ()->mMaterial == kFleshy and
        (obj->isA (kTool) or obj->isA (kArmor)))
    {
        obj->set (obj::known_type | obj::known_infected |
                  obj::known_enhancement | obj::known_bugginess);
    }
}

static void
initNinja (shCreature *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 0;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = Ninja;
    hero->rollAbilityScores (13, 10, 13, 10, 8, 8);

    hero->mMaxHP = Ninja->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       3, 1},   {kHandgun,       3, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      0, 0},
        {kUnarmedCombat, 2, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         4, 1},

        {kConcentration, 2, 0},   {kOpenLock,      4, 2},
        {kRepair,        0, 0},   {kSearch,        2, 0},
        {kHacking,       2, 0},   {kSpot,          4, 0},

        {kMFCoercion,    1, 0},   {kMFCreativity,  1, 0},
        {kMFFarsenses,   1, 0},   {kMFRedaction,   1, 0},
        {kMFTelekinesis, 1, 0},   {kMFTranslation, 1, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    const shObjId proflore[] = {
        kObjNunchaku,
        kObjShuriken,
        kObjBoStaff
    };
    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (proflore, 3);

    /* Up to three additional implant types known at the start. */
    for (int i = RNG (1, 3); i > 0; --i) {
        shObject *imp = gen_obj_type (prob::Implant);
        AllIlks[imp->mIlkId].mFlags |= kIdentified;
        delete imp;
    }

    const prob::Table ninja_goggles[] =
    {
        {1, 1, 1, 'i', kObjScouterGoggles},
        {1, 1, 1, 'i', kObjNVGoggles},
        {1, 1, 1, 'i', kObjXRayGoggles},
        {1, 1, 1, 'i', kObjTargeterGoggles},
        END_TOKEN
    };

    const EquipmentLine stuff[] =
    {
        {kObjFragGrenade, "1d2+2 b=0"},
        {kObjKatana, "+0 b=+1"},
        {kObjChameleonSuit, "+1 b=+1"},
        {kObjLockPick, "b=0"},
        {(uintptr_t)&ninja_goggles, "b=0 s"},
        {kObjNothing, NULL}
    };
    addStartingEquipment (hero, stuff);

    shObjId PrimaryImplant[3] =
    {
        kObjAdrenalineGenerator,
        kObjReflexCoordinator,
        kObjCerebralCoprocessor
    };

    shObject *imp = new shObject (PrimaryImplant[RNG (3)]);
    imp->setDebugged ();
    imp->mEnhancement = +2;
    imp->identify ();
    hero->addObjectToInventory (imp, true);
    hero->don (imp, true);

    shObjId SecondaryImplant[4] =
    {
        kObjHealthMonitor,
        kObjSearchSkillsoft,
        kObjSpotSkillsoft,
        kObjTissueRegenerator
    };

    imp = new shObject (SecondaryImplant[RNG (4)]);
    imp->setOptimized ();
    imp->identify ();
    hero->addObjectToInventory (imp, true);
    hero->don (imp, true);

    const char *welcome[] =
    { /* Dead Cold style purposeful arrival. */
    "You have come here bearing the body of your mentor.  It was his request",
    "that interment be performed here.",
    "",
    "Oddly, the station gave no response to your docking request.  You pull",
    "into an open shuttle bay and prepare to disembark.  Nobody came to greet",
    "you nor to extract docking fees.  The hangar is devoid of all personnel",
    "save for a bunch of service robots.  You enlist them to help with proper",
    "burial procedures, carefully overseeing that all rituals are performed",
    "properly.",
    "",
    "After this silent funeral you sit down and reminisce many events you",
    "and your master lived through together.  A certain memory refuses to",
    "leave your mind, continuing to pester you mercilessly.  Long ago driven",
    "by curiosity you managed to get your mentor badly drunk.  He raved about",
    "his exploits of youth for hours on end.  His passion would wane when he",
    "got to tell about the Bizarro Orgasmatron, an artifact of untold power he",
    "was absolutely sure existed, but never found.  Every journey to obtain it",
    "proved fruitless and made him the mockery of his peers.  The legendary",
    "elusive Orgasmatron ... you know he desired it.  That was his greatest",
    "dream.  Sadly, he never got to turn it into reality.",
    "",
    "Master said the artifact is rumored to be lost somewhere at this space",
    "station, thus his wish to at least be put to rest somewhere close to the",
    "fabled device.  It is customary to mourn your guide of life for a week",
    "staying near the site of burial and performing honorable acts to glorify",
    "the memory of the passed warrior.  You come to a conclusion: the quest",
    "for Bizarro Orgasmatron starts now.  Honoring the dead is the right thing",
    "to do and there is no better way to please the soul of your mentor.",
    "After all you will be staying here for awhile.  The Orgasmatron will be",
    "yours, or you shall not dare proudly call yourself *heroname* again."
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}

static void
know_ninja (shObject *obj)
{
    if (obj->has_subtype (sword))
        obj->set (obj::known_enhancement);

    if (obj->has_subtype (grenade))
        obj->set (obj::known_bugginess);
}

static void
initXelNaga (shCreature *hero)
{
    hero->mIlkId = kMonXelNaga;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 12;
    hero->mGlyph.mTileY = 0;

    Hero.mProfession = XelNaga;
    hero->rollAbilityScores (10, 10, 10, 10, 10, 10);

    hero->mMaxHP = XelNaga->mHitDieSize +
        ABILITY_MODIFIER (hero->mAbil.Con ());

    const skillData skills[] =
    {
        {kGrenade,       3, 0},   {kHandgun,       3, 0},
        {kLightGun,      3, 0},   {kHeavyGun,      3, 0},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         3, 0},

        {kConcentration, 3, 0},   {kOpenLock,      3, 0},
        {kRepair,        3, 0},   {kSearch,        3, 0},
        {kHacking,       3, 0}, /* kSpot is absent! */

        {kMFFarsenses,   3, 0},   {kMFTelekinesis, 3, 0},
        {kMFCreativity,  3, 0},   {kMFRedaction,   3, 0},
        {kMFCoercion,    3, 0},   {kMFTranslation, 3, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    /* No starting mutations. */
    /* No knowledge at all. */
    /* No equipment either. */
}

/* Check generated inventory for items that should not be there. */
void
shProfession::postInit (shCreature *hero)
{   /* Idea: When more such items appear there could be a table with columns:
             unwanted item, replacement1, replacement2. */
    for (int i = 0; i < hero->mInventory->count (); ++i) {
        shObject *obj = hero->mInventory->get (i);
        switch (obj->mIlkId) {
        case kObjRadGrenade:
            if (Hero.mProfession != Astronaut)
                obj->mIlkId = RNG (2) ? kObjConcussionGrenade : kObjFragGrenade;
            break;
        case kObjGammaRayGun:
            if (Hero.mProfession != Astronaut)
                obj->mIlkId = RNG (2) ? kObjHeatRayGun : kObjFreezeRayGun;
            break;
        case kObjRecallDisk:
            if (Hero.mProfession != Yautja)
                obj->mIlkId = kObjIdentifyDisk;
            break;
        default:
            break;
        }
    }
}

/*
static void
know_ray_gun_charges (shObject *obj)
{
    if (obj->isA (kRayGun))
        obj->set (obj::known_charges);
}

static void
know_reticulan_stuff (shObject *obj)
{
    for (int i = 0; i < numreticulanfacts; ++i)
        if (obj->mIlkId == reticulanlore[i])
            obj->set (obj::known_enhancement);
}

static void
know_elven_stuff (shObject *obj)
{
    for (int i = 0; i < numelffacts; ++i)
        if (obj->mIlkId == elflore[i])
            obj->set (obj::known_enhancement);
}
*/

static void
know_nothing_yet (shObject *)
{
}

void
initializeProfessions (void)
{
    SoftwareEngineer = new shProfession ("software engineer", 8, 3, 1, 2,
        initSoftwareEngineer,
        know_software,
        "Summer Intern",
        "Q/A Tester",
        "Web Designer",
        "Help Desk Jockey",
        "Jr. Programmer",
        "Sysadmin",
        "Programmer",
        "Lead Programmer",
        "VP Engineering",
        "High Programmer");
/*
    Cracker = new shProfession ("cracker", 8, 3, 1, 2,
        initCracker,
        know_piracy,
        "Savescummer",
        "File Sharer",
        "W@r3z d00d",
        "Script Kiddie",
        "h@x0r",
        "1337 h@x0r",
        "Decker",
        "Sneaker",
        "Phreaker",
        "One");
*/
    Janitor = new shProfession ("janitor", 9, 3, 2, 2, initJanitor,
        know_janitor,
        "Toilet Scrubber",
        "Mop Boy",
        "Janitor",
        "Housekeeper",
        "Custodian",
        "Maintenance Man",
        "Sanitation Freak",
        "Superintendent",
        "Property Manager",
        "Landlord");

    SpaceMarine = new shProfession ("space marine", 10, 2, 2, 1,
        initSpaceMarine,
        know_soldier,
        "Private",
        "Corporal",
        "Sergeant",
        "Cadet",
        "Lieutentant",
        "Captain",
        "Major",
        "Lt. Colonel",
        "Colonel",
        "General");

    Stormboy = new shProfession ("stormboy", 10, 2, 2, 1,
        initStormboy,
        know_soldier,
        "Yoof",
        "Cannon Fodder",
        "Meatshield",
        "Warbuddy",
        "Skarboy",
        "Mob Kaptain",
        "Nob",
        "Meganob",
        "Warboss",
        "Waagh! Lord");

    Yautja = new shProfession ("hunter", 10, 2, 2, 1,
        initYautja,
        know_yautja,
        "Unblooded",
        "Blooded",
        "Hunter",
        "Honored One",
        "Master Hunter",
        "Vanguard",
        "Elite",
        "Clan Leader",
        "Elder",
        "Adjudicator");

    Quarterback = new shProfession ("quarterback", 10, 2, 2, 1,
        initQuarterback,
        know_quarterback,
        "Towel Boy",
        "Rookie",
        "Bench Warmer",
        "Starter",
        "Jock",
        "Star Player",
        "Team Captain",
        "MVP",
        "Pro Bowler",
        "Hall Of Famer");

    Psion = new shProfession ("psion", 8, 3, 2, 2, initPsion,
        know_psyker,
        "Odd Ball",
        "Weirdo",
        "Mind Reader",
        "Spoon Bender",
        "Freakazoid",
        "Mutant",
        "Empath",
        "Psion",
        "Psyker",
        "Farseer");

    Empath = new shProfession ("empath", 8, 3, 2, 2, initEmpath,
        know_psyker,
        "Seeker",
        "Enlightened One",
        "Mind Mirror",
        "Thought Catcher",
        "Subconscious Link",
        "Psionic Vein",
        "Brainwave Warper",
        "Neuroconductor",
        "Mind Overlord",
        "Fate Weaver");

    Mastermind = new shProfession ("mastermind", 6, 3, 2, 3, initMastermind,
        know_psyker,
        "Far Herald",
        "Space Brother",
        "Cult Idol",
        "First Contact",
        "Federation Envoy",
        "Galactic Senator",
        "Coadunator",
        "Uplifter",
        "Terraformer",
        "Astronaut God");

    Weirdboy = new shProfession ("weirdboy", 8, 3, 2, 2, initWeirdboy,
        know_psyker,
        "Crazy One",
        "Mad Orc",
        "Very Mad Orc",
        "Frazzler",
        "Humming Head",
        "Brain Warper",
        "Walking Psi Bomb",
        "One Orc Storm",
        "Warptide",
        "Warp Unleasher");

    Astronaut = new shProfession ("astronaut", 8, 3, 3, 2,
        initAstronaut,
        know_nothing_yet,
        "Rocket Tester",
        "Vostok Martyr",
        "Moon Walker",
        "Deimos Stevedore",
        "Leonov Engineer",
        "Marauder Pilot",
        "Von Braun Staff",
        "LongShot Navigator",
        "Tie Fighter Ace",
        "Nostromo Survivor");

    Ninja = new shProfession ("ninja", 9, 3, 4, 1, initNinja,
        know_ninja,
        "Eavesdropper",
        "Stalker",
        "Looming Shadow",
        "Infiltrator",
        "Sabotager",
        "Terrorist",
        "Corporate Spy",
        "Black Hand",
        "Cyber Assassin",
        "Unseen Master");

    Dissident = new shProfession ("dissident", 6, 3, 2, 3,
        initDissident,
        know_dissident,
        "Mars Castaway",
        "Moon Base Staff",
        "Orbital Watch",
        "Land Agent",
        "Covert Eye",
        "Arms Dealer",
        "Belt Capo",
        "Planet Conqueror",
        "System Kingpin",
        "Sector Crimelord");

    XelNaga = new shProfession ("wanderer", 10, 4, 0, 0, initXelNaga,
        know_nothing_yet,
        "Outworlder",
        "Void Explorer",
        "Genesis Witness",
        "Chaos Shaper",
        "Nebulae Catcher",
        "Star Assembler",
        "Planet Shifter",
        "Galaxy Designer",
        "DNA Programmer",
        "Species Creator");
}


shProfession *
chooseProfession ()
{
    const void *result = NULL;
    const void *randomNoOutsider = (void *) 0x1;
    const void *random = (void *) 0x2;

    shMenu *menu = I->newMenu ("Choose your profession", shMenu::kNoHelp);
    menu->attachHelp ("profession.txt");

    menu->addHeader ("Soldier:");
    menu->addPtrItem ('m', "Human Space Marine", SpaceMarine);
    menu->addPtrItem ('S', "Space Orc Stormboy", Stormboy);
    menu->addPtrItem ('y', "Yautja Hunter", Yautja);
    menu->addHeader ("Psyker:");
    menu->addPtrItem ('p', "Human Psion", Psion);
    menu->addPtrItem ('e', "Space Elf Empath", Empath);
    menu->addPtrItem ('M', "Reticulan Mastermind", Mastermind);
    menu->addPtrItem ('w', "Space Orc Weirdboy Warphead", Weirdboy);
    menu->addHeader ("Subversive Element:");
    menu->addPtrItem ('n', "Human Ninja", Ninja);
    menu->addPtrItem ('D', "Reticulan Dissident", Dissident);
    menu->addHeader ("Peaceful citizen:");
    menu->addPtrItem ('s', "Human Software Engineer", SoftwareEngineer);
    menu->addPtrItem ('a', "Human Astronaut", Astronaut);
    menu->addPtrItem ('q', "Human Quarterback", Quarterback);
    menu->addPtrItem ('j', "Space Orc Janitor", Janitor);
    menu->addHeader ("Outsider:");
    menu->addPtrItem ('X', "Xel'Naga", XelNaga);
    menu->addHeader ("Random choice:");
    menu->addPtrItem ('r', "Random non-outsider", randomNoOutsider);
    menu->addPtrItem ('R', "Random", random);

    do {
        menu->getPtrResult (&result);
        menu->dropResults ();
        if (randomNoOutsider == result) {
            result = Professions.get (RNG (Professions.count () - 1));
        } else if (random == result) {
            result = Professions.get (RNG (Professions.count ()));
        }
    } while (!result);
    delete menu;
    return (shProfession *) result;
}
