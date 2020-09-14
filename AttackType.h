#ifndef ATTACKTYPE_H
#define ATTACKTYPE_H

#include "Attack.h"
#include "Global.h"
#include "ObjectIlk.h"

/* After adding any energy type here remember to add respective
   res_type field to resistances table in Items.txt and Monsters.txt. */
enum shEnergyType {
    kNoEnergy,

    /* The following ignore armor. */
    kIrresistible,   /* What it says on the tin.  Use judiciously. */
    kDisintegrating, /* Antimatter. */
    kPsychic,        /* Psi attacks. */

    /* Armor taken in normal way. */
    kBullet,         /* Guns and cannons. */
    kConcussive,     /* Smashing or bashing. */
    kExplosive,      /* Pure force of explosion. */
    kHackNSlash,     /* Cutting and hacking. */
    kLaser,          /* Many energy weapons. */
    kParticle,       /* For energy guns like blaster. */
    kShrapnel,       /* Shotguns, frag grenades, exploding canisters. */

    /* Armor is only half effective. */
    kBurning,        /* Flamethrowers, napalm. */
    kCorrosive,      /* Acid. */
    kElectrical,     /* Zappers, electric batons. */
    kFreezing,       /* Cryolator, freeze ray. */
    kPlasma,         /* Plasma weapons.  High destructive potential. */

    /* Armor is only one third effective. */
    kMagnetic,       /* Gauss ray gun, plasma weapons side effect. */
    kRadiological,   /* Gamma Caves, rad traps, rad grenades, gamma ray guns. */

    /* Special energies not harming hit points. */
    kAccelerating,   /* Canister of speed. */
    kBlinding,       /* Side effect of laser weapons. */
    kBugging,        /* Decreases item statuses down to buggy. */
    kConfusing,      /* Mental blasts, drinking beer. */
    kDecelerating,
    kDigging,        /* Mining laser. */
    kMesmerizing,    /* Hypnotic gaze, narcolepsy. */
    kParalyzing,     /* No movement at all! */
    kSickening,      /* Suppresses regeneration. */
    kSpecial,        /* For quantifying special effects; does no direct harm. */
    kStunning,       /* Stun grenades, powerful blows. */
    kTransporting,   /* Teleport equivalent. */
    kViolating,      /* You get sore butt by getting hit with this. */
    kWebbing,        /* Partial immobilization. */

    /* Ability modifying "energies". */
    kDrnStr,
    kDrnCon,
    kDrnAgi,         /* Temporary lowering. */
    kDrnDex,
    kDrnInt,
    kDrnPsi,
    kDmgStr,
    kDmgCon,
    kDmgAgi,         /* Damage recoverable with restoration. */
    kDmgDex,
    kDmgInt,
    kDmgPsi,

    kMaxEnergyType
};

enum shAttackFlags {
    kMelee =           0x1,  /* mop, chainsaw */
    kMissile =         0x2,  /* shuriken, grenade */
    kAimed =           0x4,  /* blaster, chaingun */
    kTouchAttack =     0x10  /* AC is 10 + bonus from agility */
};

#define ATKDMG 3

struct shAttack
{
    enum Type {        /* primarily used for descriptive purposes */
        kNoAttack,
        kAccelerationRay,
        kAcidBath,     /* Acid pit trap. */
        kAcidSplash,   /* Acid blood. */
        kAnalProbe,
        kAttach,       /* e.g. attaching a restraining bolt */
        kAugmentationRay,
        kBite,
        kBlast,        /* explosion */
        kBolt,         /* energy bolt, e.g. pea shooter, blaster bolt */
        kBreatheBugs,
        kBreatheFire,
        kBreatheTime,
        kBreatheTraffic,
        kBreatheViruses,
        kClaw,
        kClub,
        kChoke,
        kCombi,        /* combi-stick */
        kCreditDraining, /* creeping credits :-) */
        kCrush,
        kCryolator,    /* Freezes creatures low on hit points. */
        kCut,
        kDecelerationRay,
        kDecontaminationRay,
        kDisc,         /* Smart-Disc */
        kDisintegrationRay,
        kDrill,
        kEnsnare,      /* Webbing grenades. */
        kExplode,      /* Attack calls 'die (kSuicide)' shortly after. */
        kExtractBrain, /* Mi-go specialty, also used by docbot. */
        kFaceHug,      /* Facehugger obviously. */
        kFlare,        /* Lights area. */
        kFlash,        /* Radiation grenade. */
        kFreezeRay,
        kGammaRay,
        kGaussRay,
        kGun,          /* Common for bullet based guns. */
        kHalf,         /* Changes damage amount to 50% of target's HP. */
        kHeadButt,
        kHealingRay,
        kHeatRay,
        kImpact,       /* football, improvised thrown weapon */
        kIncendiary,
        kKick,
        kLaserBeam,
        kLight,        /* blinding flash */
        kLegalThreat,
        kLightningBolt,
        kMentalBlast,
        kOilSpill,     /* Some bots can spill oil to blind. */
        kOpticBlast,
        kPea,          /* Pea shooter.  Like kGun but different animation. */
        kPlague,       /* Defiler vomit. */
        kPlasmaGlob,   /* Plasma guns. */
        kPoisonRay,
        kPrick,        /* Gom Jabbar needle */
        kPsionicStorm,
        kPunch,
        kQuill,
        kRail,         /* Railgun slug. */
        kRake,         /* Rear claws of catbot.  Xel'Naga use it too. */
        kRestorationRay,
        kSaw,          /* Chainsaw and one of docbot's melee attacks. */
        kScorch,       /* Burning in melee for NNTP daemon. */
        kShot,         /* Shotgun pellets. */
        kSlash,
        kSlime,
        kSmash,
        kSpear,        /* Speargun's spearhead darts. */
        kSpit,         /* For hydralisk. */
        kSplash,       /* Spilling a liquid on something or someone. */
        kStab,
        kStasisRay,
        kSting,
        kStomp,
        kSword,        /* Slicing message for swords. */
        kTailSlap,
        kTouch,
        kTractor,      /* Reel things close to attack source. */
        kTransporterRay,
        kVomit,        /* Psi vomit mutant power. */
        kWaterRay,     /* Squirt ray gun. */
        kWeb,          /* Web mutant power, net launcher pistol. */
        kWhip,
        kZap,
        kMaxAttackType
    };


    enum Effect {
        kSingle,    /* single target */
        kSmartDisc, /* for Yautja Smart-Discs */
        kCone,      /* a spreading area-effect attack */
        kExtend,    /* a straight line area-effect attack */
        kBeam,      /* as above but with random 1-6 bonus to range */
        kBurst,     /* a disc that doesn't penetrate walls */
        kFarBurst,  /* kBurst centered at target hit by kSingle */
        kOther
    };

    Type mType;
    Effect mEffect;
    int mFlags;
    int mRadius;             /* blast radius in squares */
    int mRange;              /* in squares for beams (+d6 will be added),
                                o/w in feet */
    int mHitMod;             /* difficulty of hitting with this attack */
    int mAttackTime;         /* AP spent recovering after attack */
    struct {
        shEnergyType mEnergy;
        int mLow, mHigh; /* damage ranges */
        int mChance;     /* percentage chance of causing this damage */
    } mDamage[ATKDMG];

    int bypassesShield ();
    int isSingleAttack () { return kSingle == mEffect; }
    int isAimedAttack () { return kAimed & mFlags; }
    int isMeleeAttack () { return kMelee & mFlags; }
    int isMissileAttack () { return kMissile & mFlags; }
    int isTouchAttack () { return kTouchAttack & mFlags; }
    int isAbsorbedForFree (int index);
    int isLightGenerating (); /* drawn over dark squares */
    int dealsEnergy (shEnergyType type);
    bool isPure (shEnergyType type);
    bool isPure ();
    int findEnergyType (shEnergyType type);

    int getThreatRange (shObject *weapon);
    int getCriticalMultiplier (shObject *weapon);

    const char *noun ();
    const char *damStr ();
    static bool isLethal (shEnergyType);
    static bool isSpecial (shEnergyType e) { return e >= kAccelerating; }
    static bool is_toxin (shEnergyType type);
};

const char *energyDescription (shEnergyType t);

extern shAttack Attacks[];
#endif
