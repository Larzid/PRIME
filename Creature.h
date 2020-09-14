#ifndef CREATURE_H
#define CREATURE_H

#define ABILITY_MODIFIER(score) ((score) / 2 - 5)
#define ABILITY_BONUS(score) ((score) < 12 ? 0 : (score) / 2 - 5)
#define ABILITY_PENALTY(score) ((score) > 9 ? 0 : (score) / 2 - 5)
#define ABILITY_MAX 25

#include "Map.h"
#include "Profession.h"
#include "Ability.h"
#include "Services.h"
#include "CrType.h"

enum shActionState
{
    kActing,   /* actively taking turns */
    kWaiting,  /* don't process, on another level */
    kDead      /* don't process, will be cleaned up soon */
};


enum shGender {
    kFemale =    0x1,
    kMale =      0x2
};

enum shCondition {
    kSpeedy =        0x1,       /* from temporary speed boost item */
    kSlowed =        0x2,       /* also from temporary item */
    kConfused =      0x4,       /* mental blast, canister of beer, etc. */
/*  unused           0x8,
    unused           0x10, */
    kHosed =         0x20,      /* traffic */
    kViolated =      0x40,      /* anal probe */
    kXenosHunter =   0x80,      /* wears marine stuff */
/*  unused           0x100, */
    kSickened =      0x200,     /* virus attack */
    kParalyzed =     0x400,     /* stasis ray, some melee attacks, etc. */
    kFrozen =        0x800,     /* Cryolator effect. */
/*  unused           0x1000, */
    kStunned =       0x2000,    /* stun grenades, etc */
    kAsleep =        0x4000,    /* Hypnosis, narcoleptor. */
    kPlagued =       0x8000,    /* Defiler plague. */

    kNoExp =         0x10000,   /* Do not give experience upon death. */
/*  unused           0x20000,
    unused           0x40000,
    unused           0x80000,*/

    kUnableCompute = 0x100000,  /* The Voice said not to read the Nam-Shub. */
    kUnableUsePsi =  0x200000,  /* The Voice said no mutant power usage. */
    kUnableAttack =  0x400000,  /* The Voice said not to show belligerence. */
    kGenerous =      0x800000,  /* The Voice said to lower prices. */

    kFleeing =       0x1000000, /* but not necessarily frightened - for mons */
    kFrightened =    0x2000000,
/*  unused           0x4000000,
    unused           0x8000000,*/

    kBurdened =      0x10000000,
    kStrained =      0x30000000,
    kOvertaxed =     0x70000000,
    kOverloaded =    0xf0000000
};
const unsigned int kEncumbrance = kOverloaded;


#define SKILL_KEY_ABILITY(_code) \
    ((abil::Index) (((_code) & kSkillAbilityMask) >> 8))

enum shFeat {
    kNoFeat =           0x000,
    kMimicMoney =       0x001,
    kMimicObject =      0x002,
    kMimicFeature =     0x004,
    kHideUnderObject =  0x008,
    kExplodes =         0x010,
    kSessile =          0x020,
    kNoTreasure =       0x040,
    kUniqueMonster =    0x080,
    kWarpish =          0x100,
    kBurrow =           0x200,
    kMaxFeat
};

int sportingD20 ();

/* What are kSlain and kKilled for:
 * kSlain means whacked, cut or shot to death.  Aliens dying that way splash
 * acid everywhere.  Smart bombs, smart missiles and other explosive bots go
 * boom.
 * kKilled means death comes through some non-violent way.  Aliens do not splash
 * acid, smart bombs leave proximity mine item.
 */
enum shCauseOfDeath {
    kSlain,
    kKilled,
    kAnnihilated,  /* Took disintegration damage. */
    kSuddenDecompression,
    kSuicide,
    kDrowned,
    kMisc, /* Miscellaneous stupid stuff, use killer text. */
    kMiscNoYouDie, /* Use killer text, do not print "You die". */
    kQuitGame,
    kWonGame
};

/* timeout keys */

enum shTimeOutType {
    NOTIMEOUT,
    ASLEEP,
    BLINDED,
    CAFFEINE,
    CONFUSED,
    FLEEING,
    FRIGHTENED,
    FROZEN,
    HOSED,
    PARALYZED,
    PLAGUED,
    PREGNANCY,
    SICKENED,
    SLOWED,
    SPEEDY,
    STUNNED,
    TELEPATHY,
    TRAPPED,
    VIOLATED,
    VOICE_GENEROSITY,
    VOICE_NAMSHUB,
    VOICE_MUTANT,
    VOICE_VIOLENCE,
    XRAYVISION
};

struct shCreature
{
    enum Strategy {
        kWander,      /* wander around looking for a fight */
        kLurk,        /* stay put, if Hero approaches, fight and pursue */
        kSitStill,    /* stay put, attack any adjacent creature */
        kHide,        /* same as sitstill, but pursue if hero finds us */
        kHatch,       /* alien egg strategy */
        kPet,         /* pet strategy */
        kShopKeep,
        kAngryShopKeep,
        kGuard,
        kDoctor,
        kMelnorme
    };

    enum Tactic {
        kNewEnemy,  /* respond to new threat */
        kReady,     /* ready for next tactic */
        kFight,     /* attempt close combat */
        kShoot,     /* attempt ranged combat */
        kMoveTo     /* move to a location */
    };

    enum Disposition {
        kHelpful,
        kFriendly,
        kIndifferent,
        kUnfriendly,
        kHostile
    };

    struct TimeOut {
        shTimeOutType mKey;
        shTime mWhen;

        TimeOut () { };
        TimeOut (shTimeOutType key, shTime when) {
            mKey = key;
            mWhen = when;
        }
    };

    class shMapLevel *mLevel;
    shMonId mIlkId;
    int mX, mY, mZ; /* Z: -1, 0, 1  for in pit, on ground, flying */

    char mName[30];
    int mGender;

    int mAP;        /* action points */

    int mCLevel;    // character level

    int mAC;        // armor class

    int mHP, mMaxHP;   // hit points
    shTime mLastRegen; //time of last HP regen

    abil::Tracker mAbil;
    shIntrinsicData mInnateIntrinsics; /* Permanent + from mutant powers. */
    shIntrinsicData mIntrinsics;       /* Innate + from equipment. */

    int mRad;                /* radiation exposure */
    int mXP;
    int mSkillPoints;
    int mPsionicStormFatigue;

    int mSpeed;
    int mSpeedBoost; /* e.g. psionic speed boost */
    int mBurdenMod; /* Speed penalty from carrying too much. */

    int mNaturalArmorBonus;
    int mReflexSaveBonus; /* when dodging area effects */
    int mWillSaveBonus;   /* when saving against psychic attacks */

    /* resistances represented as damage reduction */
    int mInnateResistances[kMaxEnergyType]; /* permanent */
    int mResistances[kMaxEnergyType];      /* innate + that from equip */

    int mFeats;
    char mMutantPowers[kMaxAllPower];
    int mHidden;    /* hide score (negative iff Hero has spotted, but
                       monster doesn't know) */
    enum { kMmNothing, kMmObject, kMmFeature, kMmMonster } mMimic;
    shTime mSpotAttempted;   /* last time hero tried to spot creature */
    int mConditions;
    int mCarryingCapacity;   /* all weights are in grams */
    /* Following three variables store effect of equipment or mutant powers
       on psionic actions, hit chance and damage respectively. */
    int mPsiModifier;
    int mToHitModifier;
    int mDamageModifier;
    unsigned int mImpregnation; /* Progress of chestburster pregnancy. */

    int mWeight; /* Total weight of carried stuff. */

    struct {
        int mInPit;          /* how badly stuck in pit */
        int mDrowning;       /* air left in lungs */
        int mWebbed;         /* sticky webs impeding movement */
        int mTaped;          /* getting immobilized with duct tape */
    } mTrapped;

    shTime mLastMoveTime;    /* time of last movement */
    shDirection mDir;        /* direction of current movement */
    int mLastX, mLastY;      /* previous x,y coordinates */
    shActionState mState;
    shGlyph mGlyph;
#define TRACKLEN 5
    shCoord mTrack[TRACKLEN]; /* mTrack[0] == our last position */

    /* Monster decision code. */
    Strategy mStrategy;
    Tactic mTactic;
    Disposition mDisposition;

    /* AI information. */
    int mTame; /* 0 = wild, 1 = loyal */
    bool mDisturbed; /* True if took damage at least once or saw the hero. */
    int mEnemyX;  /* set if monster is threatened by enemy other than hero */
    int mEnemyY;  /* (right now only used for pets) */
    int mDestX;
    int mDestY;
    shDirection mPlannedMoves[100];
    int mPlannedMoveIndex;
    shTime mSpellTimer;

    /* State for special behaviors. */
    union {
        struct {
            int mShopId;
            int mHomeX;  /* coordinates of the "home spot" in front of */
            int mHomeY;  /* the shop door */
            int mBill;   /* amount owed for destroyed items */
        } mShopKeeper;
        struct {
            int mSX;       /* coordinates of guarded rectangle */
            int mSY;
            int mEX;
            int mEY;
            int mToll;     /* toll: -1 for none, 0 for already paid */
            int mChallengeIssued;
        } mGuard;
        struct {
            int mHatchChance;
        } mAlienEgg;
        struct {
            int mHomeX;
            int mHomeY;
            int mRoomID;
            int mPermute[kMedMaxService];
        } mDoctor;
        struct {
            int mKnowledgeExhausted;
            int mSX;     /* Coordinates of current room. */
            int mSY;
            int mEX;
            int mEY;
            int mPermute[kMelnMaxService];
        } mMelnorme;
    };

    union {
        shObjectIlk *mMimickedObject;
        shFeature::Type mMimickedFeature;
        shMonsterIlk *mMimickedMonster;
    };

    /* Equipment. */
    shObject *mWeapon;       /* must be the first ptr field (see save ()) */

    shObject *mJumpsuit;     /* worn under body armor */
    shObject *mBodyArmor;
    shObject *mHelmet;
    shObject *mBoots;
    shObject *mGoggles;
    shObject *mBelt;
    shObject *mCloak;

    shObject *mImplants[shObjectIlk::kMaxSite];

    shVector <TimeOut *> mTimeOuts;
    shVector <shSkill *> mSkills;
    shObjectVector *mInventory;
    shMapLevel *mLastLevel;  /* previous level */
    shCauseOfDeath mHowDead; /* Set when mState is set to kDead. */

    shCreature ();
    ~shCreature ();

    static shCreature *monster (shMonId id);
    void becomeAMonster (shMonId id);

    void saveState (int fd);
    void loadState (int fd);

    shMonsterIlk *myIlk ();
    bool isA (shMonId id);
    bool isHero ();

    /* Grammar. */
    const char *the ();
    const char *an ();
    const char *your ();
    const char *are ();
    const char *herself ();
    const char *her ();
    const char *her (const char *thing);
    const char *namedher ();

    int interrupt ();

    int reflexSave (int DC);
    int willSave (int DC);

    //RETURNS: 1 if attack kills us; o/w 0
    int sufferAbilityDamage (abil::Index idx, int amount, bool perm = true);

    /* Taking damage and dying. */
    int suffer_toxin (shAttack *attack, shCreature *attacker);
    //RETURNS: 1 if attack kills us; o/w 0
    int sufferDamage (shAttack *attack, shCreature *attacker = NULL,
        shObject *weapon = NULL,
        int bonus = 0, int multiplier = 1, int divisor = 1);
    int sufferDamage (shAttackId attid, shCreature *atkr = NULL,
        shObject *weapon = NULL, int bonus = 0, int mult = 1, int divd = 1)
    {
        return sufferDamage (&Attacks[attid], atkr, weapon, bonus, mult, divd);
    }
    void takeRads (int amt);
    //RETURNS: 1 if the creature really dies; o/w 0
    int die (shCauseOfDeath how, shCreature *killer,
        shObject *implement, shAttackId attack, const char *killstr = NULL);
    int die (shCauseOfDeath how, shCreature *killer,
        shObject *implement, shAttack *attack, const char *killstr = NULL);
    int die (shCauseOfDeath how, const char *killstr = NULL) {
        return die (how, NULL, NULL, NULL, killstr);
    }
    int die (shCauseOfDeath how, shCreature *killer) {
        return die (how, killer, NULL, NULL, NULL);
    }
    void pDeathMessage (const char *monname, shCauseOfDeath how,
                               int presenttense = 0);

    /* Experience. */
    void beatChallenge (int challenge);
    void earnXP (unsigned int experience);
    void levelUp ();

    void showLore ();


    /* Inventory management. */
    void reorganizeInventory ();
    int listInventory ();
    void adjust (shObject *obj);
    //RETURNS: 1 if successful
    int addObjectToInventory (shObject *obj, bool quiet = false);
    void removeObjectFromInventory (shObject *obj);
    shObject *removeOneObjectFromInventory (shObject *obj);
    shObject *removeSomeObjectsFromInventory (shObject *obj, int cnt);
    void useUpOneObjectFromInventory (shObject *obj);
    //RETURNS: number of objects used
    int useUpSomeObjectsFromInventory (shObject *obj, int cnt);
    int owns (shObject *obj);
    int hasInstalled (shObjId);
    shObject *quickPickItem (shObjectVector *v, const char *action,
        int flags, int *count = NULL);

    //RETURNS: 1 if successful, 0 o/w
    int wield (shObject *obj, int quiet = 0);
    //RETURNS: 1 if successful, 0 o/w
    int unwield (shObject *obj);
    int expendAmmo (shObject *weapon, int cnt = 0);
    int hasAmmo (shObject *weapon);

    /* Creature type and genus. */
    bool isAlive ();
    bool isProgram ();
    bool isRobot ();
    bool isInsect ();
    bool isAlien ();
    bool radiates ();
    inline bool isSlime () { return mGlyph.mSym == 's'; }
    inline bool isOrc () { return mGlyph.mSym == 'O'; }
    inline bool isElf () { return mGlyph.mSym == 'E'; }
    bool isXenos ();

    /* Heretics = humans using psychic powers not for the Imperium. */
    inline int isHeretic () {
        return isA (kMonBeneGesserit) or isA (kMonSorceress) or
               isA (kMonMutantHuman);
    }

    /* Health and abilities. */
    void doDiagnostics (int quality, int present = 1, FILE *file = NULL);
    void gainAbility (bool controlled, int num);
    int needsRestoration ();
    int checkRadiation ();

    /* Inventory and equipment. */
    void drop (shObject *obj);
    int don (shObject *obj, int quiet = 0);
    int doff (shObject *obj, int quiet = 0);
    void damageEquipment (shAttack *attack, shEnergyType energy);
    void damageEquipment (shAttackId attid, shEnergyType energy)
    {
        damageEquipment (&Attacks[attid], energy);
    }

    /* Skills. */
    void addSkill (shSkillCode c, int access, shMutantPower power = kNoMutantPower);
    shSkill *getSkill (shSkillCode c, shMutantPower power = kNoMutantPower);

    int getSkillModifier (shSkillCode c, shMutantPower power = kNoMutantPower);
    void gainRank (shSkillCode c, int howmany = 1,
                   shMutantPower power = kNoMutantPower);

    int getWeaponSkillModifier (shObjectIlk *ilk, shAttack *atk = NULL);
    void editSkills ();

    /* Energy. */
    int countEnergy (int *tankamount = NULL);
    int loseEnergy (int amount);
    void gainEnergy (int amount);

    /* Cash. */
    int countMoney ();
    int loseMoney (int amount);
    int gainMoney (int amount);

    /* Item use. */
    int quaffCanister (shObject *can);
    int loadRayGun (shObject *gun);
    int useKey (shObject *key, shFeature *door);

    /* Intrinsics or feats. */
    int intr (shIntrinsic);
    void setIntr (shIntrinsic, int);
    bool hasMind ();
    bool feat (shFeat);

    /* Timed effects. */
    shTime setTimeOut (shTimeOutType key, shTime howlong, int additive = 1);
    TimeOut *getTimeOut (shTimeOutType key);
    inline void clearTimeOut (shTimeOutType key)
    {
        setTimeOut (key, 0, 0);
    }
    int checkTimeOuts ();

    /* Temporary or internal conditions. */
    bool isHelpless (void);
    int is (shCondition cond);
    void inflict (shCondition cond, int howlong);
    void cure (shCondition cond);
    void setBlind () { mIntrinsics.set (kBlind, true); }
    void resetBlind () { mIntrinsics.set (kBlind, false); }
    void makeBlinded (int howlong);
    void checkConcentration ();
    int isTrapped ();
    void abortion (bool quiet = false);
    int isMoving ();
    int getEncumbrance () { return mConditions & kEncumbrance; }
    int instantUpkeep ();
    void upkeep ();

    /* Location. */
    bool isUnderwater ();
    bool isInShop ();
    int isInPit ();
    int isAdjacent (int x, int y) { return areAdjacent (mX, mY, x, y); }

    /* Changing whereabouts. */
    void enterShop ();
    void leaveShop ();
    int transport (int x, int y, int control = 0,
        bool imprecise = false, bool force = false);
    void oldLocation (int newX, int newY);
    void newLocation ();
    void enterHospital ();
    void leaveHospital ();
    void followHeroToNewLevel ();

    /* Operations on creature that affect only internals. */
    void free_from_traps ();

    /* Miscellaneous or uncategorized. */
    const char *radLevels ();
    void postMortem ();
    int hpWarningThreshold () { return mini (mMaxHP-1, maxi (mMaxHP/6, 5)); }
    void sterilize ();
    int tryToTranslate (shCreature *c);

    bool isFriendly (shCreature *c) {
        return (c->isHero () or c->isPet ()) == (isHero () or isPet ());
    }
    int getShockCapacity (void);
    void absorbShock (int damage, shObjectVector *v);
    int utilizeWreck (shObject *obj);
    void computeIntrinsics (bool quiet = false);
    void computeBurden (bool quiet = false);
    void computeSkills ();
    void computeAC ();
    bool sewerSmells ();

    int getAC (int flatfooted = 0);
    int getTouchAC (int flatfooted = 0);

    shThingSize getSize ();
    int getSizeACMod (shThingSize size);

    int isImmuneToCriticalHits () { return isHero (); }
    int getResistance (shEnergyType etype) {
        return mResistances[etype] > 100 ? 1000 : mResistances[etype];
    }
    int kick (shDirection dir);

    int numHands ();

    int getPsionicDC (int powerlevel);


    /* Interaction with environment and space base features. */
    void shootLock (shObject *weapon, shAttack *attack, shFeature *door);
    //RETURNS: 1 if successful, 0 o/w
    int openDoor (int x, int y);
    int closeDoor (int x, int y);
    void quaffFromVat (shFeature *vat);
    void quaffFromAcidPit ();

    /* Hiding. */
    int canHideUnder (shObject *o) {
        return mFeats & kHideUnderObject and o->myIlk ()->mSize > getSize ();
    }
    int canHideUnder (shFeature *f) {
        return mFeats & kHideUnderObject and f->isHidingSpot ();
    }
    int revealSelf ();
    bool hasCamouflage ();

    /* Passing as someone else. */
    int looksLikeJanitor ();

    /* Shopping, trade or other commercial endeavors. */
    void damagedShop (int x, int y);
    void dropIntoOwnedVat (shObject *obj);
    void quaffFromOwnedVat (shFeature *vat);
    void movedVat ();
    void stolenVat ();
    void pickedUpItem (shObject *obj);
    void billableOffense (shObject *obj, const char *gerund, int fee);
    void employedItem (shObject *obj, int fee);
    void usedUpItem (shObject *obj, int cnt, const char *action);
    void maybeSellItem (shObject *obj);
    void payShopkeeper ();
    void payDoctor (shCreature *doctor);
    void payMelnorme (shCreature *trader);
    void bribe (shCreature *lawyer);

    /* Senses and perception. */
    bool canSmell (shCreature *c);
    bool canSenseLife (shCreature *c);
    int canSee (int x, int y);
    int canSee (shCreature *c);
    bool seeByXRay (int x, int y);
    bool canHearThoughts (shCreature *c);
    bool canTrackMotion (shCreature *c);
    bool canTrackMotion (shFeature *f);
    bool canFeelSteps (shCreature *c);
    bool isAwareOf (shCreature *c);
    int spotStuff (bool force);
    void doSearch ();
    void sensePeril ();
    void innate_knowledge (shObject *);

    /* Decision code and actions. */
    void newEnemy (shCreature *c);
    void makeAngry ();
    int tryToFreeSelf ();
    int readyWeapon ();

    /* Disposition and tameness. */
    int isHostile () { return kHostile == mDisposition; }
    bool isPet () { return mTame; }
    void makePet ();
    int attemptRestraining (shObject *bolt);

    /* Combat functions; see Fight.cpp */
    int meleeAttack (shObject *weapon, shDirection dir);
    int meleeAttack (shObject *weapon, int x, int y);
    int meleeAttack (shAttack *attack, shObject *weapon, int x, int y);
    int attackRoll (shAttack *attack, shObject *weapon,
                    int attackmod, int AC, shCreature *target);
    int rangedAttackHits (shAttack *attack, shObject *weapon, int attackmod,
                          shCreature *target, int *dbonus);
    int resolveRangedAttack (shAttack *attack, shObject *weapon,
                             int attackmod, shCreature *target,
                             shObject *stack = NULL);
    int shootWeapon (shObject *weapon, shDirection dir,
                     shAttack *attack = NULL);
    int resolveMeleeAttack (shAttack *attack, shObject *weapon,
                            shCreature *target);
    void projectile (shObject *obj, shObject *stack,
                     int x, int y, shDirection dir,
                     shAttack *attack, int basehit, int range);
    int throwObject (shObject *obj, shObject *stack, shDirection dir);
    int misfire (shObject *weapon, shAttack *attack);
    int reflectAttack (shAttack *attack, shDirection *dir);

    /* Can effects: */
    int healing (int hp, int hpmaxdice);
    int fullHealing (int hp, int hpmaxdice);

    /* Mutant powers: */
    int useMutantPower ();
    int stopMutantPower (shMutantPower id);
    shMutantPower getMutantPower (shMutantPower power = kNoMutantPower,
                                  int silent = 0);
    int getMutantPowerChance (int power);
    void lose_mutant_power (shMutantPower power, bool silent = false);

    int telepathy (int on);
    int tremorsense (int on);
    int opticBlast (shDirection dir);
    int shootWeb (shDirection dir);
    int hypnosis (shDirection dir);
    int xRayVision (int on);
    int mentalBlast (int x, int y);
    int regeneration ();
    int restoration (int howmuch);
    int adrenalineControl (int on);
    int haste (int on);
    int teleportation ();
    int psionicStorm (int x, int y);
    int theVoice (int x, int y);
    int magneticMayhem (int x, int y);
    int flight (int on);
    int kinesis (int x, int y, shDirection dir);
    void getShoved (shCreature *attacker, shDirection dir, int range);
    int charge (shDirection dir);
    int ceaseAndDesist ();
    bool terrify (int howlong, int power);
    bool usesPower (shMutantPower power);

    /* Initialization crap: */
    void rollAbilityScores (
        unsigned int strbas, unsigned int conbas, unsigned int agibas,
        unsigned int dexbas, unsigned int intbas, unsigned int psibas);
    void rollHitPoints (int hitdice, int diesize);

    /* Taking action: */
    void playerControl ();
    void monsterControl ();
    int objectVerbCommand (shObject *obj);

    /* Receiving messages.         Condition to print str: */
    bool msg (const char *str);    /* Is hero. */
    bool visual (const char *str); /* Is not blind hero. */
    bool hear (const char *str);   /* Is not deaf hero. */
    bool appear (const char *str); /* Is not hero and hero sees it. */
    bool see (const char *str);    /* Hero sees it. */
 private:
    shObject *getKeyForDoor (shFeature *f);
    int doWear (shObject *obj);
    int doTakeOff (shObject *obj);
    void specialAdjustments (void); /* Used when creating. */
    void pregnancy (); /* Called by upkeep. */
    int doPlayerMove (shDirection dir);

    /* Flags for evaluating nearby surroundings by monsters. */
    enum SquareFlags {
        kHero =       0x1,
        kMonster =    0x2,
        kEnemy =      0x4,
        kLinedUp =    0x10,
        kDoor =       0x100,
        kWall =       0x200,
        kTrap =       0x400,
        kFreeMoney =  0x1000,
        kFreeWeapon = 0x2000,
        kFreeArmor =  0x4000,
        kFreeEnergy = 0x8000,
        kFreeWreck =  0x10000,
        kFreeItem =   0x1f000,
        kHidingSpot = 0x20000
    };

    /* Helpers for monster decision code. */
    int checkThreats ();
    void findPetGoal ();
    void findTreasure ();

    int likesMoney ();
    int likesWeapons ();
    int needsWeapon ();

    int doComputerMove (shDirection dir);
    int setupPath ();
    int doQuickMoveTo (shDirection dir = kNoDirection);
    int doMoveTo ();
    int clearObstacle (int x, int y);
    int drinkBeer ();

    bool willYouPay (void); /* Used by doGuard. */

    int findSquares (int flags, shCoord *coords, int *info);

    /* The two functions below return kAttDummy on no attack was chosen or
       available.  On success attack identifier is returned. */
    shAttackId pickMeleeAttack ();
    shAttackId pickRangedAttack ();

    /* Actions for monster decision code. */
    int doAttack (shCreature *target, int *elapsed);
    void doRangedAttack (shAttack *attack, shDirection dir);
    bool disregardAttack (shAttack *atk);
    int monUseMutantPower ();
    int mimicSomething ();
    int meleeAttack (shObject *weapon, shAttack *attack, int x, int y);

    /* Strategies: */
    int doStrategy (); /* Chooser. */

    int doSitStill ();
    int doHide ();
    int doLurk ();
    int doHatch ();
    int doWander ();
    int doPet ();
    int doShopKeep ();
    int doAngryShopKeep ();
    int doGuard ();
    int doDoctor ();
    int doMelnorme ();

    int doRetaliate ();
};

#endif
