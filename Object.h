#ifndef OBJECT_H
#define OBJECT_H
#include <stdlib.h>

#include "Global.h"
#include "ObjectType.h"
#include "Util.h"
#include "MonsterIlk.h"


struct shObject;
typedef shVector <shObject *> shObjectVector;

int selectObjects (shObjectVector *dest, shObjectVector *src, shObjId ilk);
int selectObjects (shObjectVector *dest, shObjectVector *src, subtype_t st);
int selectObjects (shObjectVector *dest, shObjectVector *src,
                   shObjectType type);
int selectObjectsByFlag (shObjectVector *dest, shObjectVector *src,
                         unsigned int flag, bool invert = false);
int deselectObjectsByFlag (shObjectVector *dest, shObjectVector *src,
                           unsigned int flag);
int selectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                             bool (shObject::*idfunc) (), bool invert = false);
int deselectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                               bool (shObject::*idfunc) ());
int selectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                             bool (*objfunc) (shObject *), bool invert = false);
int deselectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                               bool (*objfunc) (shObject *));


/* shOperatingSystem values are stored in computer enhancement value. */
enum shOperatingSystem {
    kNoSys          = -5, /* computer inoperable */
    kAbysmalSys     = -4,
    kTerribleSys    = -3,
    kBadSys         = -2,
    kPoorSys        = -1,
    kAverageSys     =  0,
    kFineSys        = +1,
    kGoodSys        = +2,
    kExcellentSys   = +3,
    kSuperbSys      = +4,
    kUniqueSys      = +5
};

enum shCompInterface {
    kNotAComputer,
    kIVisualOnly,
    kIVisualVoice,
    kIUniversal
};

namespace obj {

enum Flag {
    known_type =        0x1,    // item true name revealed
    known_bugginess =   0x2,
    known_enhancement = 0x4,    // so called plus
    known_charges =     0x8,    // # of charges
    known_appearance =  0x10,   // disk labels, canister types
    known_infected =    0x20,   // disk/computer infection
    known_cracked =     0x40,   // hacked software
    known_fooproof =    0x80,   // only computer antivirus might be unknown
    known_exact =       0xff,   // sum of all above
    fooproof =          0x100,  // acidproof, fireproof, etc.
                                // also computers protected by antivirus
    cracked =           0x200,  // floppy disks and keycards
    infected =          0x400,  // software contains virus
    thrown =            0x800,  // was thrown by player
    worn =              0x1000,
    wielded =           0x2000,
    active =            0x4000, // turned on, for example
    unpaid =            0x8000, // belongs to shopkeeper
    toggled =           0x10000,
    designated =        0x20000,// primary item for some action
    prepared =          0x40000 // quick swap weapon or item
};

}

struct shObject
{
    /* NOTE: don't forget to update constructor AND split () method
       when adding new fields to this class!
       Oh yeah and saveObject() and loadObject() too!
    */
    shObjId mIlkId;
    shObjectIlk *myIlk () { return &AllIlks[mIlkId]; }
    shDescription *apparent (void);

    int mCount;               // for mergeable objects
    int mCharges;             // for chargeable objects (maxhp for wreck)
    int mDamage;              // corrosion / fire damage
    int mEnhancement;         // operating system for computers
    char mLetter;             // letter in Hero's inventory display
    int mBugginess;           // optimized +1 / debugged 0 / buggy -1
    shTime mLastEnergyBill;   // last time energy was billed to the item
    int mFlags;               // definitions in obj::flags

    enum Location {
        kNowhere,
        kFloor,
        kInventory
    } mLocation;
    int mX;
    int mY;
    shCreature *mOwner;       /* Creature with obj in inventory (use kUnpaid
                                 for obj owned by shopkeeper). */

    const char *mUserName;    /* Name given by user with name command. */
    union {
        shMonId mWreckIlk;
        shObjectIlk::Site mImplantSite;
        int mColor;           /* For customizing floppies and lightsabers. */
    };

    //constructor:
    shObject () {
        mIlkId = kObjNothing; mCount = 0; mEnhancement = 0; mCharges = 0;
        mLetter = 0; mBugginess = 0; mFlags = 0; mOwner = NULL;
        mDamage = 0; mLastEnergyBill = MAXTIME; mUserName = NULL;
    }
    shObject (shObjId id);

    void saveState (int fd);
    void loadState (int fd);

    const char *getDescription (int cnt = -1);
    const char *getShortDescription (int cnt = -1);
    const char *getVagueDescription ();
    int isA (shObjId ilk);
    bool has_subtype (subtype_t);
    inline int isA (shObjectType type) { return myIlk ()->mReal.mType == type; }
    inline int looksLikeA (shObjectType type) { return apparent ()->mType == type; }
    int getIlkFlag (int id) { return myIlk ()->mFlags & id; }
    shGlyph getGlyph ();
    int getMass () { return mCount * myIlk ()->mWeight; }
    int getCost () { return mCount * myIlk ()->mCost; }
    void name (const char *newname = NULL);
    void nameIlk ();
    void maybeName ();

    bool is (unsigned int);
    void set (unsigned int);
    void clear (unsigned int);
    void toggle (unsigned int);

    bool isUnique ();
    bool isUniqueKnown ();
    bool isFooproofable ();
    bool isEnhanceable ();
    bool isChargeable ();
    bool isRadioactive ();
    bool isKnownRadioactive ();
    bool isIndestructible ();
    bool isInfectable ();
    bool isCrackable ();
    bool isBugProof ();
    bool isEquipped ();
    bool isDamaged ();
    bool isSealedArmor ();
    bool isPoweredArmor ();
    bool isMeleeWeapon ();
    bool isThrownWeapon ();
    bool isAimedWeapon ();
    bool isWeldedWeapon ();
    bool isSelectiveFireWeapon ();
    bool isKnownWeapon ();
    bool canBeWorn ();
    bool canBeDrunk ();
    bool isUseable ();

    /* Bugginess: */
    inline bool isBuggy () { return -1 == mBugginess; }
    inline bool isDebugged () { return 0 == mBugginess; }
    inline bool isOptimized () { return 1 == mBugginess; }
    void setBuggy ();
    void setOptimized ();
    void setDebugged ();

    /* Computer properties section: */
    bool isComputer (); /* For OS floppy disk. */
    int hackingModifier ();
    bool hasSystem ();
    bool systemRunPOST (); /* Power-On Self Test */
    bool systemCheckDisk (shObject *disk);
    bool isUpdateTime ();
    void systemUpdateMessage ();
    void kickComputer ();
    int executeProgram (shObject *media);
    int execTime ();

    void identify ();
    shEnergyType vulnerability ();

    int isAmmo (shObject *weapon);

    void impact (shCreature *c, shDirection dir,
                 shCreature *thrower = NULL, shObject *stack = NULL);
    void impact (int x, int y, shDirection dir,
                 shCreature *thrower = NULL, shObject *stack = NULL);
    void impact (shFeature *c, shDirection dir,
                 shCreature *thrower = NULL, shObject *stack = NULL);
    void breakAt (int x, int y);

    int sufferDamage (shAttack *attack, shCreature *attacker, int x, int y);
    int sufferDamage (shAttackId id, shCreature *attacker, int x, int y);
    int sufferDamage (shEnergyType energy, shAttack *attack,
                      shCreature *attacker, int x, int y);

    const abil::Set *getAbilityModifiers ();
    void applyConferredIntrinsics (shCreature *target);
    void applyConferredResistances (shCreature *target);
    void applyConferredSkills (shCreature *target);

    void draw (shInterface *I);
    void showLore (void);
    int canMerge (shObject *obj = NULL);
    void merge (shObject *obj); /* obj will be deleted */
    shObject *split (int count);

    const char *those (int cnt = -1);
    const char *these (int cnt = -1);
    const char *the (int cnt = -1);
    const char *theQuick (int cnt = -1);
    const char *an (int cnt = -1);
    const char *anQuick (int cnt = -1);
    const char *anVague (int cnt = -1);
    const char *your (int cnt = -1);
    const char *her (shCreature *owner, int cnt = -1);
    const char *inv (void);

    /* Prints "j - debugged +2 sniper rifle". */
    void announce (void);

    /* any object can be used as a weapon, so we supply these routines: */
    int getThreatRange ();
    int getCriticalMultiplier ();

    int getArmorBonus ();
    int getPsiModifier ();

    int isXenosHunterGear ()
    {
        return mIlkId == kObjAquamarinePH or mIlkId == kObjMeanGreenPH or
               mIlkId == kObjAquamarinePA or mIlkId == kObjMeanGreenPA or
               mIlkId == kObjAdamantineCloak;
    }

    inline int isIlkKnown () { return getIlkFlag (kIdentified); }
    inline void setIlkKnown ()
    {
        if (!isIlkKnown ()) {
            if (myIlk ()->mUserName) {
                free ((void *) myIlk ()->mUserName);
                myIlk ()->mUserName = NULL;
            }
        }
        myIlk ()->mFlags |= kIdentified;
    }
private:
    const char *stackDesc (int cnt, const char *pronoun, const char *desc);
    const char *anStackDesc (int cnt, const char *desc);
};


int compareObjects (shObject **obj1, shObject **obj2);
int quaffCanister (shObject *can);

// infected == 1 means tell looks only
void identifyObjects (int howmany, int infected);

shObject *findBestObject (shObjectVector *objs, bool markSeen = false);
shObject *createWreck (shCreature *m);

void makePlural (char *buf, int len);
shObject *createObject (const char *desc);

shObject *generateObject (void);
shObject *gen_obj_type (prob::Table *table);

void initializeWeapons ();
void initializeTools ();
void initializeRayGuns ();
void initializeCanisters ();
void initializeFloppyDisks ();
void initializeImplants ();
void initializeObjects ();
void initializeLore ();
void initializeParser ();

void purgeDeletedObjects ();

#endif
