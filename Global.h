#ifndef GLOBAL_H
#define GLOBAL_H

#define ZAPM_VERSION "0.8.2"
#define PRIME_VERSION "2.4"

#ifdef DJGPP
#include <stdarg.h>
#include <stdio.h>
int snprintf (char *str, size_t size, const char *format, ...);
int vsnprintf (char *str, size_t size, const char *format, va_list ap);
#endif

#include <assert.h>
#ifdef _WIN32
#define DATADIR "user"
#define USERDIR "user"
#define SCOREDIR "user"
#else
#include "config.h"
#endif
#include "FeatureToggle.h"

typedef signed int shTime;    //ms
#define MAXTIME 99999999 /*so sloppy*/

#define LONGTURN  2000
#define SLOWTURN  1500
#define DIAGTURN  1000 //TODO: remove
#define FULLTURN  1000
#define QUICKTURN 750
#define HALFTURN  500

struct shInterface;
struct shMenu;
struct shIntrinsicData;
class shCreature;
struct shMonsterIlk;
class shMonster;
struct shObjectIlk;
struct shObject;
struct shHero;
class shMapLevel;
struct shFeature;
namespace abil { struct Set; }
namespace prob { struct Table; }

struct shCoord { char mX; char mY; };

extern shObjectIlk AllIlks[];
extern shMonsterIlk MonIlks[];

enum shObjectType
{
    kUninitialized = 0,
    kMoney,
    kImplant,
    kFloppyDisk,
    kCanister,
    kTool,
    kArmor,
    kWeapon,
    kProjectile,
    kOther,
    kRayGun,
    kEnergyCell,
    kMaxObjectType
};

const char objectTypeHeader[kMaxObjectType][20] =
{
    "UNINITIALIZED",
    "Money",
    "Bionic Implants",
    "Floppy Disks",
    "Canisters",
    "Tools",
    "Armor",
    "Weapons",
    "Ammunition",
    "Other",
    "Ray Guns",
    "Energy Cells"
};

/* Game options. */
struct shFlags {
    char mKeySet[40];
    int mFadeLog;
    int mShowLOS;
    int mAutopickup;
    int mAutopickupTypes[kMaxObjectType];
    //float mDelayMult;
};

enum shDirection {
    kNorth = 0,
    kNorthEast = 1,
    kEast = 2,
    kSouthEast = 3,
    kSouth = 4,
    kSouthWest = 5,
    kWest = 6,
    kNorthWest = 7,
    kUp = 8,
    kDown = 9,
    kOrigin = 10,
    kNoDirection = 11
};

extern shTime Clock;         // global clock
extern shTime MonsterClock;


enum shColor {
    kBlack = 0,
    kBlue,
    kGreen,
    kCyan,
    kRed,
    kMagenta,
    kBrown,
    kGray,
    kDarkGray,       /* unused color */
    kNavy,           /* bright blue */
    kLime,           /* bright green */
    kAqua,           /* bright cyan */
    kOrange,         /* bright red */
    kPink,           /* bright magenta */
    kYellow,
    kWhite
};

struct shGlyph {
    char mSym;
    shColor mColor, mBkgd;
    int mTileX, mTileY;
};


enum shSkillCode {
    kNoSkillCode =        0x0000,

    kSkillAbilityMask =   0x0f00,
    kSkillTypeMask =      0x00f0,
    kSkillNumberMask =    0x000f,

    kWeaponSkill =        0x0000,
    kAdventureSkill =     0x0010,
    kMetapsychicFaculty = 0x0020,
    kMutantPower =        0x0030,

    kStrSkill =           0x0100, /* Must be kStr << 8 */
    kUnarmedCombat =      0x0101,
    kHeavyGun =           0x0102,

    kConSkill =           0x0200, /* Must be kCon << 8 */
    kPowerArmor =         0x0201,
    kConcentration =      0x0210,

    kAgiSkill =           0x0300, /* Must be kAgi << 8 */
    kMeleeWeapon =        0x0301,
    kSword =              0x0302,

    kDexSkill =           0x0400, /* Must be kDex << 8 */
    kGrenade =            0x0401,
    kHandgun =            0x0402,
    kLightGun =           0x0403,
    kOpenLock =           0x0410,
    kRepair =             0x0411,

    kIntSkill =           0x0500, /* Must be kInt << 8 */
    kSearch =             0x0510,
    kHacking =            0x0511,
    kSpot =               0x0512,

    kPsiSkill =           0x0600, /* Must be kPsi << 8 */
    kMFCoercion =         0x0620,
    kMFCreativity =       0x0621,
    kMFFarsenses =        0x0622,
    kMFRedaction =        0x0623,
    kMFTelekinesis =      0x0624,
    kMFTranslation =      0x0625,

    kUninitializedSkill = 0xffff
};

#include "Util.h"

void exitPRIME (const int code); /* defined in Game.cpp */

extern shHero Hero;
extern shMapLevel *Level;  /* points to the current level */
extern shVector <shMapLevel *> Maze;  /* all the levels */
extern shFlags Flags;
extern int GameOver;

#define HERO_NAME_LENGTH 14
#endif
