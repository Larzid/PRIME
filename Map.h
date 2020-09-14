#ifndef MAP_H
#define MAP_H

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "AttackType.h"
#include "MonsterIlk.h"
#include "Distribution.h"
#include "Effect.h"

#define MAPMAXCOLUMNS 64
#define MAPMAXROWS    20
#define MAPMAXROOMS   80

enum shTerrainType {
/* Impassable and occlusive terrain first: */
    kStone = 0,
    kVWall,
    kHWall,
    kCavernWall1,
    kCavernWall2,
    kSewerWall1,
    kSewerWall2,
    kVirtualWall1,
    kVirtualWall2,
    kNWCorner,
    kNECorner,
    kSWCorner,
    kSECorner,
    kNTee,
    kSTee,
    kWTee,
    kETee,
/* Impassable but unocclusive: */
    kGlassPanel,

/* Passable terrain: */
    kFloor,
    kFirstFloor = kFloor,
    kCavernFloor,
    kSewerFloor,
    kVirtualFloor,
    kBrokenLightAbove, /* Turns to kFloor after light is repaired. */
    kSewage,           /* hiding spot */
    kLastFloor = kSewage,
    kVoid,
    kMaxTerrainType
};

#define leftQuarterTurn(_direction) (shDirection) (((_direction) + 6) % 8)
#define rightQuarterTurn(_direction) (shDirection) (((_direction) + 2) % 8)


#define leftTurn(_direction) (shDirection) (((_direction) + 7) % 8)
#define rightTurn(_direction) (shDirection) (((_direction) + 1) % 8)
#define uTurn(_direction) (shDirection) (((_direction) + 4) % 8)
#define isHorizontal(_direction) (2 == (_direction) % 4)
#define isVerictal(_direction) (0 == (_direction) % 4)
#define isDiagonal(_direction) (1 == (_direction) % 2 and (_direction) < 8)

shDirection vectorDirection (int x1, int y1, int x2, int y2);
shDirection vectorDirection (int x1, int y1);
shDirection linedUpDirection (int x1, int y1, int x2, int y2);
shDirection linedUpDirection (shCreature *e1, shCreature *e2);

const char *stringDirection (shDirection d);

//RETURNS: distance in feet between points (squares are 5x5)
int distance (int x1, int y1, int x2, int y2);
int distance (shCreature *c1, shCreature *c2);
int distance (shCreature *c1, int x1, int y1);

int rlDistance (int x1, int y1, int x2, int y2);

int areAdjacent (int x1, int y1, int x2, int y2);
int areAdjacent (shCreature *c1, shCreature *c2);

struct shTerrainSymbol
{
    char mGlyph;
    char mColor;
    char *desc;
};


struct shFeature
{
    enum Type {
        /* impassable features first */
        kDoorHiddenVert,
        kDoorHiddenHoriz,
        kDoorClosed,
        kMovingHWall,       /* max occlusive */
        kMachinery,         /* max impassable */

        /* passable features */

        kStairsUp,
        kStairsDown,
        kFloorGrating,
        kBrokenGrating,
        kVat,               /* min hiding spot */
        kComputerTerminal,  /* max hiding spot */

        kPit,     /* min trap type */
        kAcidPit,
        kSewagePit,
        kRadTrap,
        kACMESign,
        kTrapDoor,
        kHole,
        kWeb,
        kPortal,  /* max trap type */

        kPortableHole,
        kDoorOpen,
        kMaxFeatureType
    };

    enum DoorFlags {
        kInverted = 0x1, /* If automatic, the logic will be inverted. */
        kAutomatic = 0x2,
        kBerserk = 0x4,
        kLocked = 0x8,
        kLockBroken = 0x10,
        kMagneticallySealed = 0x20,
        kLockRetina = 0x40,
        kLockKeycard = 0x80,
        kAnyLock = 0xC0,
        kLock1 = 0x180,
        kLock2 = 0x280,
        kLock3 = 0x480,
        kLock4 = 0x880,
        kLockMaster = 0x1080, /* Only master keycard will open this one. */
        kColorCode = 0x1f00,
        kAlarmed = 0x2000,
        kHorizontal = 0x4000, /* For displaying. */
        kCanFeel = 0x40f6     /* Features of door that can be felt blindly. */
    };

    Type mType;
    int mTrapUnknown;    /* is the hero unaware of this trap? */
    int mTrapMonUnknown; /* are the monsters unaware of this trap? */
    int mSportingChance; /* bonus for searching, kicking doors down,
                            incremented with each attempt */
    shTime mSpotAttempted;

    int mX;
    int mY;
    union {
        struct {  /* used for stairs, portals */
            int mLevel;
            int mX;
            int mY;
        } mDest;
        struct {
            int mFlags;
            int mMoveTime; /* time of last automatic door movement */
        } mDoor;
        struct {
            int mHealthy; /* 0 for really gross, higher is cleaner */
            int mRadioactive;
            int mAnalyzed; /* 1 for known ingredients */
        } mVat;
        struct {
            int mBeginY;
            int mEndY;
            int mRoomId;
        } mMovingHWall;
    };
    //char mHP;

    //constructor:
    shFeature ()
    {
        mTrapUnknown = 0;
        mTrapMonUnknown = 0;
        mSportingChance = 0;
        mSpotAttempted = 0;
    }

    int
    isObstacle () {
        return mType <= kMachinery;
    }


    int
    isOcclusive () {
        return mType <= kDoorClosed;
    }

    bool
    isPit () {
        return mType == kPit or mType == kAcidPit or mType == kSewagePit;
    }

    bool
    isHole () {
        return mType == kHole or mType == kTrapDoor or mType == kBrokenGrating;
    }

    bool
    isDoor () {
        return mType == kDoorHiddenVert or mType == kDoorClosed or
               mType == kDoorHiddenHoriz or mType == kDoorOpen;
    }
    int isOpenDoor () { return kDoorOpen == mType; }
    int isClosedDoor () { return kDoorClosed == mType; }
    int isHorizontalDoor () { return isDoor () and mDoor.mFlags & kHorizontal; }
    int isAutomaticDoor () { return isDoor () and mDoor.mFlags & kAutomatic; }
    int isInvertedDoor () { return isDoor () and mDoor.mFlags & kInverted; }
    int isBerserkDoor () { return isDoor () and mDoor.mFlags & kBerserk; }
    int isLockedDoor () { return isDoor () and mDoor.mFlags & kLocked; }
    int isLockBrokenDoor () { return isDoor () and mDoor.mFlags & kLockBroken; }
    int isRetinaDoor () { return isDoor () and mDoor.mFlags & kLockRetina; }
    int isCodeLockDoor () { return isDoor () and mDoor.mFlags & kLockKeycard; }
    int isLockDoor () { return isDoor () and mDoor.mFlags & kAnyLock; }
    int isAlarmedDoor () { return isDoor () and mDoor.mFlags & kAlarmed; }
    int isMagneticallySealed () {
        return isDoor () and mDoor.mFlags & kMagneticallySealed;
    }
    int isHiddenDoor () {
        return kDoorHiddenVert == mType or kDoorHiddenHoriz == mType;
    }
    shObjectIlk *keyNeededForDoor ();
    void lockDoor () { mDoor.mFlags |= kLocked; }
    void unlockDoor () { mDoor.mFlags &= ~kLocked; }

    int isStairs () { return mType == kStairsUp or mType == kStairsDown; }

    int
    isTrap ()
    {
        return isBerserkDoor () or (mType >= kPit and mType <= kPortal);
    }

    int
    isHidingSpot ()
    {
        return (mType >= kVat and mType <= kComputerTerminal);
    }

    void splash (void);
    shObject *dropObject (shCreature *actor, shObject *obj);

    const char *getDoorLockName ();
    const char *getDescription ();
    const char *getShortDescription ();
    const char *the () {
        char *buf = GetBuf ();
        snprintf (buf, SHBUFLEN, "the %s", getShortDescription ());
        return buf;
    }
    const char *an () {
        char *buf = GetBuf ();
        const char *tmp = getShortDescription ();
        snprintf (buf, SHBUFLEN, "%s %s",
                  isvowel (tmp[0]) ? "an" : "a", tmp);
        return buf;
    }

    static shFeature *newDoor (void);
    static shFeature *newVat (void);
    static bool teleportVat (shFeature *vat, shCreature *actor, int newX, int newY);
    static bool teleportAcid (shFeature *acidpit, shCreature *actor, int newX, int newY);
    bool quaffFromVat (shCreature *);
};


struct shSquare
{
    enum shSquareFlags
    {
        kHallway =      0x1,
        kRadioactive =  0x2,
        kStairsOK =     0x4,  /* Good place for stairs. */
        kNoLanding =    0x8,  /* Disallow landing here with transportation. */

        kDarkNW =      0x10,
        kDarkNE =      0x20,
        kDarkSW =      0x40,
        kDarkSE =      0x80,
        kDark =        0xf0,
        kLit =        0x100   /* Illuminated by some light source. */
    };

    shTerrainType mTerr;
    int mFlags;
    unsigned char mRoomId;

    const char *the ();
    const char *an ();
};


struct shRoom {
    enum Type {
        kNotRoom = 0,
        kNormal,
        kCavern,
        kGeneralStore,
        kHardwareStore,
        kSoftwareStore,
        kArmorStore,
        kWeaponStore,
        kImplantStore,
        kCanisterStore,
        kNest,     /* alien nest */
        kHospital,
        kSewer,
        kGarbageCompactor
    };

    Type mType;

    bool is_shop ();
};


class shMapLevel
{
public:
    enum MapFlags {
        kHeroHasVisited = 0x1,
        kHasShop =        0x2,
        kNoTransport =    0x4,
        kNoDig =          0x8,
        kHasMelnorme =    0x10
    };
    enum MapType {
        kBunkerRooms,
        kTown,
        kRabbit,
        kRadiationCave,
        kMainframe,
        kSewer,
        kSewerPlant,
        kTest
    };

public:
//private:
    int mDLevel;       /* depth level (difficulty) */
    MapType mMapType;
    char mName[12];
    int mRows;
    int mColumns;
    int mType;
    int mFlags;
    int mNumRooms;
    int mCompactorState;
    int mCompactorHacked;
    struct {
        int mCompactor;
    } mTimeOuts;

    shRoom mRooms[MAPMAXROOMS]; /* room 0 is the non-room */
    shSquare mSquares[MAPMAXCOLUMNS][MAPMAXROWS];
    struct shMemory {
        shObjId mObj;
        shMonId mMon;
        shTerrainType mTerr;
        int mDoor;
        shFeature::Type mFeat;
    } mRemembered[MAPMAXCOLUMNS][MAPMAXROWS];
    shObjectVector *mObjects [MAPMAXCOLUMNS][MAPMAXROWS];
    shCreature *mCreatures[MAPMAXCOLUMNS][MAPMAXROWS];
    unsigned char mVisibility[MAPMAXCOLUMNS][MAPMAXROWS];
    eff::type mEffects[MAPMAXCOLUMNS][MAPMAXROWS];
    shVector <shCreature *> mCrList;
    shVector <shFeature *> mFeatures;
    shVector <shFeature *> mExits;
    shVector <Effect_on_map> mEffectList;
private:
    void reset ();
public:
    shMapLevel () : mCrList (), mFeatures (), mExits (), mEffectList ()
    {
        reset ();
    }
    shMapLevel (int level, MapType type);

    void saveState (int fd);
    void loadState (int fd);

    inline shSquare *
    getSquare (int x, int y)
    {
        return &mSquares[x][y];
    }

    void set_special_effect (int x, int y, eff::type ef);

    shFeature *getFeature (int x, int y);
    shFeature *getKnownFeature (int x, int y);

    inline shCreature *
    getCreature (int x, int y)
    {
        return mCreatures[x][y];
    }

    inline const char *
    the (int x, int y)
    {
        shFeature *f = getFeature (x, y);
        if (f) {
            return f->the ();
        } else {
            return getSquare (x, y) -> the ();
        }
    }

    inline const char *
    an (int x, int y)
    {
        shFeature *f = getFeature (x, y);
        if (f) {
            return f->an ();
        } else {
            return getSquare (x, y) -> an ();
        }
    }




    shCreature *removeCreature (shCreature *c);

    //RETURNS: 1 if the square is still on the map
    int moveForward (shDirection, int *x, int *y, int *z = NULL);

    //RETURNS non-zero iff the square x,y is on the map
    inline int
    isInBounds (int x, int y)
    {
        return x >= 0 and x < mColumns and y >= 0 and y < mRows;
    }

    //RETURNS: non-zero iff the square at x,y can be walked upon
    inline int
    isFloor (int x, int y)
    {
        return (kFirstFloor <= mSquares[x][y].mTerr and
                mSquares[x][y].mTerr <= kLastFloor);
    }

    inline int
    appearsToBeFloor (int x, int y)
    {
        if (isFloor (x, y)) {
            shFeature *f = getFeature (x, y);
            if (f and (shFeature::kDoorHiddenVert == f->mType
                      or shFeature:: kDoorHiddenHoriz == f->mType))
            {
                return 0;
            } else {
                return 1;
            }
        } else {
            return 0;
        }
    }

    inline int
    isWatery (int x, int y)
    {
        return kSewage == mSquares[x][y].mTerr;
    }

    inline int
    isHidingSpot (int x, int y)
    {
        if (kSewage == mSquares[x][y].mTerr) {
            return 1;
        }
        shFeature *f = getFeature (x, y);
        if (f)
            return f->isHidingSpot ();
        return 0;
    }


    inline int
    getRoomID (int x, int y)
    {
        return mSquares[x][y].mRoomId;
    }

    shRoom *
    getRoom (int x, int y)
    {
        return &mRooms[mSquares[x][y].mRoomId];
    }

    /* Finds room dimensions by scanning whole level. */
    void
    getRoomDimensions (int id, int *sx, int *sy, int *ex, int *ey)
    {
        if (id == 0) return; /* Non-room. */
        *sx = MAPMAXCOLUMNS-1;  *sy = MAPMAXROWS-1;  *ex = 0;  *ey = 0;
        /* Find room dimensions. */
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            for (int y = 0; y < MAPMAXROWS; ++y) {
                if (Level->getRoomID (x, y) == id) {
                    if (x < *sx) *sx = x;
                    if (x > *ex) *ex = x;
                    if (y < *sy) *sy = y;
                    if (y > *ey) *ey = y;
                }
            }
        }
    }

    /* Is the square (x,y) lit from the viewpoint of (vx, vy). */
    bool isLit (int x, int y, int vx, int vy);
    void setLit (int x, int y, int nw = 1, int ne = 1, int sw = 1, int se = 1);

    int
    isInDoorWay (int x, int y)
    {
        shFeature *f = getFeature (x, y);
        return f and f->isDoor ();
    }


    int
    isInRoom (int x, int y)
    {
        return isFloor (x, y) and !(mSquares[x][y].mFlags & shSquare::kHallway);
    }


    int
    stairsOK (int x, int y)
    {
        return isFloor (x, y) and (mSquares[x][y].mFlags & shSquare::kStairsOK);
    }


    int
    landingOK (int x, int y)
    {
        return isFloor (x, y) and
            !(mSquares[x][y].mFlags & shSquare::kNoLanding);
    }


    const char *shopAd (int room);
    bool isInShop (int x, int y);

    int
    isInHospital (int x, int y)
    {
        return shRoom::kHospital == mRooms[mSquares[x][y].mRoomId].mType;
    }

    int
    isInGarbageCompactor (int x, int y)
    {
        return shRoom::kGarbageCompactor == mRooms[mSquares[x][y].mRoomId].mType;
    }


    shCreature *getShopKeeper (int x, int y);
    shCreature *getDoctor (int x, int y);
    shCreature *getGuard ();
    shCreature *getMelnorme ();

    //RETURNS: non-zero iff there is a creature on the square x,y
    inline int
    isOccupied (int x, int y)
    {
        return NULL == mCreatures[x][y] ? 0 : 1;
    }


    //RETURNS: the objects on the square x,y
    inline shObjectVector *
    getObjects (int x, int y)
    {
        return mObjects[x][y];
    }


    inline shObject *
    removeObject (int x, int y, shObjId ilk)
    {
        shObject *res = findObject (x, y, ilk);
        if (res) {
            if (res->mCount > 1) {
                return res->split (1);
            }
            mObjects[x][y]->remove (res);
            if (0 == mObjects[x][y]->count ()) {
                delete mObjects[x][y];
                mObjects[x][y] = NULL;
            }
        }
        return res;
    }


    //RETURNS: count of objects on the square x,y
    inline int
    countObjects (int x, int y)
    {
        return NULL == mObjects[x][y] ? 0 : mObjects[x][y]->count ();
    }


    shObject *findObject (int x, int y, shObjId ilk);


    inline void
    setObjects (int x, int y, shObjectVector *v)
    {
        mObjects [x][y] = v;
    }


    //RETURNS: non-zero iff the square at x,y contains impassable terrain
    inline int
    isObstacle (int x, int y)
    {
        shFeature *f = getFeature (x,y);
        if (f and f->isObstacle ()) {
            return 1;
        } else {
            return mSquares[x][y].mTerr <= kGlassPanel ? 1 : 0;
        }
    }

    //RETURNS: non-zero iff the square at x,y blocks line of sight
    inline int
    isOcclusive (int x, int y)
    {
        shFeature *f = getFeature (x, y);
        if (f and f->isOcclusive ()) {
            return 1;
        } else {
            return mSquares[x][y].mTerr <= kETee ? 1 : 0;
        }
    }

    //RETURNS: non-zero iff the square x,y is in the Hero's LOS.  This does
    //         not take into account lighting, telepathy, blindness, etc.
    inline int
    isInLOS (int x, int y)
    {
        return mVisibility[x][y];
    }

    int existsLOS (int x1, int y1, int x2, int y2);

    //RETURNS: number representing how radioactive is the square at x,y
    int isRadioactive (int x, int y);

    shMapLevel *getLevelBelow ();
    bool isBottomLevel () { return NULL == getLevelBelow (); }
    bool isSpaceBase () { return kBunkerRooms == mMapType; }
    bool isTownLevel () { return kTown == mMapType; }
    bool isMainframe () { return kMainframe == mMapType; }
    bool isPermaRadioactive () { return kRadiationCave == mMapType; }

    void setVisible (int x, int y, unsigned char value) { mVisibility[x][y] = value; }

    shMemory getMemory (int x, int y) { return mRemembered[x][y]; }
    inline void remember (int x, int y, shMonId m) {
        mRemembered[x][y].mMon = m;
    }
    inline void remember (int x, int y, shObjId o) {
        mRemembered[x][y].mObj = o;
    }
    inline void remember (int x, int y, shFeature *f) {
        mRemembered[x][y].mFeat = f->mType;
        if (f->isDoor ()) {
            mRemembered[x][y].mDoor = f->mDoor.mFlags;
        }
    }
    inline void remember (int x, int y, shTerrainType t) {
        mRemembered[x][y].mTerr = t;
    }
    inline void forgetCreature (int x, int y) {
        mRemembered[x][y].mMon = kMonEarthling;
    }
    inline void forgetObject (int x, int y) {
        mRemembered[x][y].mObj = kObjNothing;
    }
    inline void forgetFeature (int x, int y) {
        mRemembered[x][y].mFeat = shFeature::kMaxFeatureType;
        mRemembered[x][y].mDoor = 0;
    }
    inline void forgetTerrain (int x, int y) {
        mRemembered[x][y].mTerr = kMaxTerrainType;
    }

    void computeVisibility (shCreature *eyes);

    /* Doors. */
    void doorRemoved(shFeature *door);
    void doorCreated(shFeature *door);

    /* Square finding. */
    int findSquare (int *x, int *y);
    int findOpenSquare (int *x, int *y);
    int findUnoccupiedSquare (int *x, int *y);
    int findAdjacentUnoccupiedSquare (int *x, int *y);
    bool find_close_free_sq (int &, int &);
    int findNearbyUnoccupiedSquare (int *x, int *y);
    void findSuitableStairSquare (int *x, int *y);
    int findLandingSquare (int *x, int *y);
    bool findSpawnSquare (int &x, int &y);
    int countAdjacentCreatures (int x, int y);

    /* Square predicates. */
    bool is_ok_stair_sq (const int, const int);
    bool is_open_sq (const int, const int);
    bool is_free_sq (const int, const int);
    bool is_free_safe_sq (const int, const int);

    int rememberedCreature (int x, int y);

    void reveal (int partial);

    void debugDraw ();
    void draw ();
    void feelSq (int x, int y);
    void drawSq (int x, int y);
    int drawSqSpecialEffect (int x, int y);


    int setSpecialEffect (int x, int y, eff::type ef);
    void clearSpecialEffect (int index);
    void clearSpecialEffects ();
    int effectsInEffect ();

    int attackEffect (shAttack *atk, shObject *weapon, int x, int y,
                      shDirection dir, shCreature *attacker,
                      int attackmod = 0, int dbonus = 0);

    inline int attackEffect (shAttackId attid, shObject *wpn, int x, int y,
        shDirection dir, shCreature *atkr, int amod = 0, int dbns = 0)
    {
        return attackEffect (&Attacks[attid], wpn, x, y, dir, atkr, amod, dbns);
    }

    int areaEffectFeature (shAttack *atk, shObject *weapon, int x, int y,
          shDirection dir, shCreature *attacker);
    int areaEffectCreature (shAttack *atk, shObject *weapon, int x, int y,
          shDirection dir, shCreature *attacker,
                            int attackmod, int dbonus = 0);
    void areaEffectObjects (shAttack *atk, shObject *weapon, int x, int y,
                            shCreature *attacker);


    int warpCreature (shCreature *c, shMapLevel *newlevel);
    int checkTraps (int x, int y, int savedcmod = 0);
    void checkDoors (int x, int y);
    int moveCreature (shCreature *c, int x, int y, bool fast = false);
    int putCreature (shCreature *c, int x, int y);
    int spawnMonsters ();
    int spawnTeam (const int x, const int y, const prob::Table *table);
    int spawnNTeams (int teams, const prob::Table *table);
    int spawnGroup (int x, int y, int num, shMonId mon);

    void makeNoise (int x, int y, int radius);
    void alertMonsters (int x, int y, int radius, int destx, int desty);
    int attractWarpMonsters (int x, int y);
    void doorAlarm (shFeature *door);

    shObject *putObject (shObject *obj, int x, int y);

    void removeFeature (shFeature *f);
    void addDoor (int x, int y, int horiz, int open = -1, int lock = -1,
                  int secret = -1, int alarmed = -1, int magsealed = 0,
                  int retina = 0);
    void addDownStairs (int x, int y,
                        shMapLevel *destlev, int destx, int desty);

    void addVat (int x, int y);
    void addMuck (int x, int y, shFeature::Type type);
    void addFeature (shFeature *f);
    void darkRoom (int sx, int sy, int ex, int ey);
    shFeature *addTrap (int x, int y, shFeature::Type type);
    shFeature *addMovingHWall (int x, int y, int sy, int ey);
    shFeature *addMachinery (int x, int y);

    void magDoors (int action);  /* 1 lock, -1 unlock */
    void moveWalls (int action);
    int pushCreature (shCreature *c, shDirection dir);

    int noTransport () { return mFlags & kNoTransport; }
    int noDig () { return mFlags & kNoDig; }
    void dig (int x, int y);


    static void buildMaze ();

    const char *getDescription ();

private:

    static shMapLevel *buildBranch (MapType type, int depth, int dlevel,
                                    shMapLevel **end, int ascending = 0);


    int isClear (int x1, int y1, int x2, int y2);
    int enoughClearance (int x, int y, shDirection d, int m, int n);
    int fiveByFiveClearance (int x, int y);
    int buildRoomOrElRoom (int sx, int sy, int height);
    int buildTwistyCorridor (int x, int y, shDirection d);
    void buildSnuggledRooms ();
    void buildTwistyRooms ();
    void wallCorridors ();

    void buildDenseLevel ();

    int testSquares (int x1, int y1, int x2, int y2, shTerrainType what);

    void decorateRoom (int sx, int sy, int ex, int ey);
    int makeShop (int sx, int sy, int ex, int ey, int kind = -1);
    int makeHospital (int sx, int sy, int ex, int ey);
    int makeGarbageCompactor (int sx, int sy, int ex, int ey);
    int makeNest (int sx, int sy, int ex, int ey);
    void mundaneRoom (int sx, int sy, int ex, int ey);

    int buildBunkerRoom (int sx, int sy, int ex, int ey);
    int buildBunkerRooms ();
    void buildTown ();
    int buildCaveRoom (int x, int y, int size);
    void buildCaveTunnel (int x1, int y1, int x2, int y2);
    int buildCave ();
    void buildRabbitLevel ();

    void buildArena ();

    void buildMainframe ();
    int buildMainframeRoom (int x, int y, int width, int height);
    int buildMainframeJunction (void *user, int col, int row,
                                int x, int y, int *widths, int *heights);
    void buildMainframeHelper (void *user, int x, int y, int depth);


    void buildSewerPlant ();
    int buildSewer ();
    int buildSewerRoom (void *user, int col, int row);
    void buildSewerHelper (void *user, int x, int y, int depth);
    int floodMuck (int sx, int sy, shTerrainType type, int amount);

    void layCorridor (int x1, int y1, int x2, int y2);
    void layRoom (int x1, int y1, int x2, int y2);
    void fillRect (int sx, int sy, int ex, int ey, shTerrainType t);
    void flagRect (int sx, int sy, int ex, int ey,
                   shSquare::shSquareFlags flag, int value);
    void setRoomId (int x1, int y1, int x2, int y2, int id);

};

#endif
