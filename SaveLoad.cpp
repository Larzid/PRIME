#include "Global.h"
#include "Object.h"
#include "Interface.h"
#include "Creature.h"
#include "Hero.h"
#include "Transport.h"
#include "Scenario.h"
#include "Services.h"
#include "Flavor.h"
extern shFlavor Flavors[kFNumFlavors];

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif


static void
safeRead (int fd, void *ptr, size_t len)
{
    while (len) {
        int result = read (fd, ptr, len);
        if (result <= 0) {
            abort ();
        } else {
            len -= result;
        }
    }
}


static void
safeWrite (int fd, void *ptr, size_t len)
{
    while (len) {
        int result = write (fd, ptr, len);
        if (result <= 0) {
            abort ();
        } else {
            len -= result;
        }
    }
}

#define SWRITE(_fd, _ptr, _len) \
    safeWrite ((_fd), (void *) (_ptr), (_len))
#define SREAD(_fd, _ptr, _len) \
    safeRead ((_fd), (void *) (_ptr), (_len))

#define SWRITEBLOCK(_fd, _p1, _p2) \
    SWRITE ((_fd), (_p1), ((char *) _p2) - ((char *) _p1));
#define SREADBLOCK(_fd, _p1, _p2) \
    SREAD ((_fd), (_p1), ((char *) _p2) - ((char *) _p1));


typedef shVector <void **> TargetList;

struct  UpdatePair
{
    int mId;
    TargetList mTargets;

    UpdatePair (int id) { mId = id; }
};

static shVector <void *> PtrList;   /* maps pointers to integer identifiers */

shVector <UpdatePair *> UpdateList; /* keeps track of pointers for linking */


static void
saveMagic (int fd, unsigned int key)
{
    unsigned int magic[2] = { 0xc0debabe, key };
    SWRITE (fd, &magic, sizeof(magic));
}

static void
loadMagic (int fd, unsigned int key)
{
    unsigned int magic[2];
    SREAD (fd, &magic, sizeof(magic));
    if (0xc0debabe != magic[0]
        or     key != magic[1])
    {
        abort ();
    }
}

static void
magicVersionString (char *buf)
{
    snprintf (buf, 12, "PRIME       ");
    snprintf (&buf[6], 12, "%s", PRIME_VERSION);
}

/* Magic symbols. */
#define CREBEGIN (unsigned) 1044730435 // 'CRE>'
#define CREEND   (unsigned) 1163019068 // '<CRE'
#define MAPBEGIN (unsigned) 1045446989 // 'MAP>'
#define MAPEND   (unsigned) 1346456892 // '<MAP'
#define OBJBEGIN (unsigned) 1045054031 // 'OBJ>'
#define OBJEND   (unsigned) 1245859644 // '<OBJ'

static void
saveHeader (int fd)
{
    char magic[12];
    magicVersionString (magic);
    SWRITE (fd, magic, 12);
}

static int
loadHeader (int fd)
{
    char magic[12];
    char buf[12];
    char *m;
    char *b;

    /* compare only the first two numbers in the version string */

    magicVersionString (magic);
    m = strchr (magic, '.');
    if (m) m = strchr (m + 1, '.');
    if (m) *m = '\0';

    SREAD (fd, buf, 12);
    b = strchr (buf, '.');
    if (b) b = strchr (b + 1, '.');
    if (b) *b = '\0';

    if (strncmp (magic, buf, 12)) {
        I->p ("Couldn't load save file due to version incompatibility.");
        return -1;
    }
    return 0;
}

static void
savePointer (int fd, void *ptr)
{
    saveMagic (fd, 1);

    if (NULL == ptr) {
        int id = -1;
        SWRITE (fd, &id, sizeof (int));
    } else {
        int id = PtrList.find (ptr);

        if (-1 == id) {
            id = PtrList.add (ptr);
        }
        SWRITE (fd, &id, sizeof (int));
        debug.log ("savePointer %3d %p", id, ptr);
    }
}


static void
loadPointer (int fd, void **ptr, const char *msg)
{
    int id;

    loadMagic (fd, 1);

    SREAD (fd, &id, sizeof (int));

    *ptr = NULL;
    if (-1 == id) {
        return;
    }
    if (id < PtrList.count ()) {
        *ptr = PtrList.get (id); /* might set to NULL */
    }
    if (NULL == *ptr) {
        UpdatePair *p;
        int i;

        debug.log ("loadPointer %3d 0x???????? <- %p %s", id, ptr, msg);
        for (i = 0; i < UpdateList.count (); i++) {
            p = UpdateList.get (i);
            if (p->mId == id) {
                p->mTargets.add (ptr);
                return;
            }
        }
        p = new UpdatePair (id);
        p->mTargets.add (ptr);
        UpdateList.add (p);

    } else {
        debug.log ("loadPointer %3d %p <- %p %s", id, *ptr, ptr, msg);
    }
}


/* loads a new object pointer */

static void
loadStruct (int fd, void *ptr)
{
    int id;
    int i;
    int j;

    loadMagic (fd, 1);

    SREAD (fd, &id, sizeof (int));

    if (id >= PtrList.count ()) {

        PtrList.ensure (id);

        for (i = PtrList.count (); i < id; i++) {
            PtrList.set (i, NULL);
        }
    }
    PtrList.set (id, ptr);

    debug.log ("loadStruct  %3d %p", id, ptr);

/*
    int check = PtrList.add (ptr);
    assert (check == id);
*/
    for (i = 0; i < UpdateList.count (); i++) {
        UpdatePair *p = UpdateList.get (i);
        if (p->mId == id) {
            for (j = 0; j < p->mTargets.count (); j++) {
                * p->mTargets.get (j) = ptr;
                debug.log ("                %p <- %p",
                          ptr, p->mTargets.get (j));
            }
            UpdateList.remove (p);
            delete p;
            return;
        }
    }
}


static void
saveTarget (int fd, void *ptr)
{
    savePointer (fd, ptr);
}


static void
saveOptions (int fd)
{
    SWRITE (fd, &Flags, sizeof (Flags));
}

static void
loadOptions (int fd)
{
    SREAD (fd, &Flags, sizeof (Flags));
}

static void
saveIlks (int fd)
{
    SWRITE (fd, Flavors, sizeof (Flavors));
    SWRITE (fd, MedicalProcedureData, sizeof (MedicalProcedureData));
    SWRITE (fd, MelnormeServiceData, sizeof (MelnormeServiceData));
}

static void
loadIlks (int fd)
{
    SREAD (fd, Flavors, sizeof (Flavors));
    SREAD (fd, MedicalProcedureData, sizeof (MedicalProcedureData));
    SREAD (fd, MelnormeServiceData, sizeof (MelnormeServiceData));
}

static void
saveInt (int fd, int x)
{
    saveMagic (fd, 99);

    SWRITE (fd, &x, sizeof (int));
}

static int
loadInt (int fd)
{
    int x;

    loadMagic (fd, 99);

    SREAD (fd, &x, sizeof (int));
    return x;
}

static void
saveString (int fd, const char *str)
{
    saveMagic (fd, 22);

    if (NULL == str) {
        saveInt (fd, 0);
    } else {
        saveInt (fd, strlen (str));
        SWRITE (fd, str, strlen (str));
    }
}

static void
loadString (int fd, const char **str)
{
    loadMagic (fd, 22);


    int len = loadInt (fd);
    if (0 == len) {
        *str = NULL;
    } else {
        *str = (const char *) malloc (len + 1);
        SREAD (fd, *str, len);
        ((char *)(*str))[len] = 0;
    }
}


void
saveDiscoveries (int fd)
{ /* Start from obj id 1, id 0 is kObjNothing. */
    for (int i = 1; i < kObjNumIlks; i++) {
        shObjectIlk *ilk = &AllIlks[i];
        saveInt (fd, ilk->mFlags & kIdentified);
        saveString (fd, ilk->mUserName);
    }
}


void
loadDiscoveries (int fd)
{ /* Start from obj id 1, kObjNothing has NULL pointer. */
    for (int i = 1; i < kObjNumIlks; i++) {
        shObjectIlk *ilk = &AllIlks[i];
        ilk->mFlags |= loadInt (fd);
        loadString (fd, &ilk->mUserName);
    }
}


void
saveKills (int fd)
{
    for (int i = 0; i < kMonNumberOf; ++i) {
        saveInt (fd, MonIlks[i].mKills);
    }
}


void
loadKills (int fd)
{
    for (int i = 0; i < kMonNumberOf; ++i) {
        MonIlks[i].mKills = loadInt (fd);
    }
}


void
saveWarpTrace (int fd)
{
    using namespace trn;
    saveInt (fd, last_trace);
    for (int i = 0; i < MAXTRACES; ++i) {
        saveInt (fd, traces[i].src_x);
        saveInt (fd, traces[i].src_y);
        saveInt (fd, traces[i].dst_x);
        saveInt (fd, traces[i].dst_y);
        saveInt (fd, traces[i].time);
        saveString (fd, traces[i].who);
        savePointer (fd, traces[i].level);
    }
}


void
loadWarpTrace (int fd)
{
    using namespace trn;
    last_trace = loadInt (fd);
    for (int i = 0; i < MAXTRACES; ++i) {
        traces[i].src_x = loadInt (fd);
        traces[i].src_y = loadInt (fd);
        traces[i].dst_x = loadInt (fd);
        traces[i].dst_y = loadInt (fd);
        traces[i].time = loadInt (fd);
        loadString (fd, (const char **) &traces[i].who);
        loadPointer (fd, (void **) &traces[i].level, "trace[i].level");
    }
}


void
saveLizards (int fd)
{
    for (int i = 0; i < NUMLIZARDS; ++i)
        saveInt (fd, lizard[i]);
}

void
loadLizards (int fd)
{
    for (int i = 0; i < NUMLIZARDS; ++i)
        lizard[i] = (shAttackId) loadInt (fd);
}


void
saveScenario (int fd)
{
    SWRITE (fd, &Scen, sizeof (scenario));
}

void
loadScenario (int fd)
{
    SREAD (fd, &Scen, sizeof (scenario));
}


void
saveMaze (int fd)
{
    int i;

    saveInt (fd, Maze.count ());
    for (i = 1; i < Maze.count (); i++) {
        Maze.get (i) -> saveState (fd);
        saveMagic (fd, 253);
    }
}


void
loadMaze (int fd)
{
    int i, n;

    n = loadInt (fd);
    Maze.add (NULL); /* dummy zeroth level */
    for (i = 1; i < n; i++) {
        shMapLevel *level = new shMapLevel ();
        Maze.add (level);
        level->loadState (fd);
        loadMagic (fd, 253);
    }
}


void
shCreature::saveState (int fd)
{
    saveMagic (fd, CREBEGIN);
    saveTarget (fd, this);

    savePointer (fd, mLevel);

    SWRITEBLOCK (fd, &mIlkId, &mWeapon);

    savePointer (fd, mWeapon);
    savePointer (fd, mHelmet);
    savePointer (fd, mBodyArmor);
    savePointer (fd, mJumpsuit);
    savePointer (fd, mBoots);
    savePointer (fd, mGoggles);
    savePointer (fd, mBelt);
    savePointer (fd, mCloak);
    for (int i = 0; i < shObjectIlk::kMaxSite; ++i) {
        savePointer (fd, mImplants[i]);
    }

    switch (mMimic) {
    case kMmObject:  saveInt (fd, mMimickedObject->mId); break;
    case kMmFeature: saveInt (fd, mMimickedFeature); break;
    case kMmMonster: saveInt (fd, mMimickedMonster->mId); break;
    case kMmNothing:
    default:
        saveInt (fd, 0); break;
    }
    int n = mTimeOuts.count ();
    SWRITE (fd, &n, sizeof (int));
    for (int i = 0; i < n; ++i) {
        SWRITE (fd, mTimeOuts.get (i), sizeof (shCreature::TimeOut));
    }

    n = mSkills.count ();
    SWRITE (fd, &n, sizeof (int));
    for (int i = 0; i < n; ++i) {
        shSkill *skill = mSkills.get (i);
        saveInt (fd, skill->mCode);
        saveInt (fd, skill->mPower);
        saveInt (fd, skill->mRanks);
        saveInt (fd, skill->mBonus);
        saveInt (fd, skill->mAccess);
    }

    n = mInventory->count ();
    SWRITE (fd, &n, sizeof (int));
    for (int i = 0; i < n; ++i) {
        mInventory->get (i) -> saveState (fd);
    }

    savePointer (fd, mLastLevel);
    saveMagic (fd, CREEND);
}


void
shCreature::loadState (int fd)
{
    loadMagic (fd, CREBEGIN);
    loadStruct (fd, this);

    loadPointer (fd, (void **) &mLevel, "c.mLevel");
    SREADBLOCK (fd, &mIlkId, &mWeapon);

    loadPointer (fd, (void **) &mWeapon, "c.mWeapon");
    loadPointer (fd, (void **) &mHelmet, "c.mHelmet");
    loadPointer (fd, (void **) &mBodyArmor, "c.mBodyArmor");
    loadPointer (fd, (void **) &mJumpsuit, "c.mJumpsuit");
    loadPointer (fd, (void **) &mBoots, "c.mBoots");
    loadPointer (fd, (void **) &mGoggles, "c.mGoggles");
    loadPointer (fd, (void **) &mBelt, "c.mBelt");
    loadPointer (fd, (void **) &mCloak, "c.mCloak");
    for (int i = 0; i < shObjectIlk::kMaxSite; ++i) {
        loadPointer (fd, (void **) &mImplants[i], "c.mImplants");
    }
    switch (mMimic) {
    case kMmObject: mMimickedObject = &AllIlks[loadInt (fd)]; break;
    case kMmFeature: mMimickedFeature = (shFeature::Type) loadInt (fd); break;
    case kMmMonster: mMimickedMonster = &MonIlks[loadInt (fd)]; break;
    case kMmNothing:
    default:
        loadInt (fd); break;
    }

    int n;
    SREAD (fd, &n, sizeof (int));
    for (int i = 0; i < n; ++i) {
        TimeOut *t = new TimeOut ();
        SREAD (fd, t, sizeof (TimeOut));
        mTimeOuts.add (t);
    }

    SREAD (fd, &n, sizeof (int));
    for (int i = 0; i < n; ++i) {
        shSkill *skill = new shSkill (kUninitializedSkill, kNoMutantPower);

        skill->mCode = (shSkillCode) loadInt (fd);
        skill->mPower = (shMutantPower) loadInt (fd);
        skill->mRanks = loadInt (fd);
        skill->mBonus = loadInt (fd);
        skill->mAccess = loadInt (fd);
        mSkills.add (skill);
    }

    SREAD (fd, &n, sizeof (int));
    for (int i = 0; i < n; ++i) {
        shObject *obj = new shObject ();
        obj->loadState (fd);
        mInventory->add (obj);
    }

    loadPointer (fd, (void **) &mLastLevel, "c.mLastLevel");

    loadMagic (fd, CREEND);
}


void
shHero::saveState (int fd)
{
    saveMagic (fd, 0x1337c0de);

    saveInt (fd, mScore);
    saveInt (fd, mBusy);
    if (mProfession) {
        saveInt (fd, mProfession->mId);
    } else {
        saveInt (fd, -1);
    }
    savePointer (fd, mCreature);

    int n = mStoryFlags.count ();
    saveInt (fd, n);
    for (int i = 0; i < n; ++i) {
        shStoryFlag *sf = mStoryFlags.get (i);
        saveString (fd, sf->mName);
        saveInt (fd, sf->mValue);
    }

    n = mPets.count ();
    saveInt (fd, n);
    for (int i = 0; i < n; ++i) {
        savePointer (fd, mPets.get (i));
    }
}

void
shHero::loadState (int fd)
{
    loadMagic (fd, 0x1337c0de);

    mScore = loadInt (fd);
    mBusy = loadInt (fd);
    int n = loadInt (fd);
    if (-1 == n) {
        mProfession = NULL;
    } else {
        mProfession = Professions.get (n);
    }
    loadPointer (fd, (void **) &mCreature, "H.mCreature");

    n = loadInt (fd);
    for (int i = 0; i < n; ++i) {
        shStoryFlag *sf = new shStoryFlag ();
        loadString (fd, &sf->mName);
        sf->mValue = loadInt (fd);
        mStoryFlags.add (sf);
    }

    n = loadInt (fd);
    mPets.setCount (n);
    for (int i = 0; i < n; ++i) {
        loadPointer (fd, (void **) mPets.getPtr (i), "H.mPets");
    }

}


void
shMapLevel::saveState (int fd)
{
    int i, x, y;

    saveMagic (fd, MAPBEGIN);
    saveTarget (fd, this);

    SWRITEBLOCK (fd, &mDLevel, &mObjects);

    i = 0;
    for (x = 0; x < mColumns; x++) {
        for (y = 0; y < mRows; y++) {
            shObjectVector *v = mObjects[x][y];
            if (v) {
                i += v->count ();
            }
        }
    }
    saveInt (fd, i);

    for (x = 0; x < mColumns; x++) {
        for (y = 0; y < mRows; y++) {
            shObjectVector *v = mObjects[x][y];
            if (v) {
                for (i = 0; i < v->count (); i++) {
                    shObject *obj = v->get (i);
                    obj->mX = x;
                    obj->mY = y;
                    obj->saveState (fd);
                }
            }
        }
    }

    i = mCrList.count ();
    saveInt (fd, i);
    for (i = 0; i < mCrList.count (); i++) {
        shCreature *c = mCrList.get (i);
        if (c->isHero ()) {
            saveInt (fd, 1);
            Hero.saveState (fd);
        } else {
            saveInt (fd, 0);
        }
        c-> saveState (fd);
    }

    i = mFeatures.count ();
    saveInt (fd, i);
    for (i = 0; i < mFeatures.count (); i++) {
        SWRITE (fd, mFeatures.get (i), sizeof (shFeature));
    }
    saveMagic (fd, MAPEND);
}



void
shMapLevel::loadState (int fd)
{
    int i, n;
    shObject *obj;

    loadMagic (fd, MAPBEGIN);
    loadStruct (fd, this);

    SREADBLOCK (fd, &mDLevel, &mObjects);

    n = loadInt (fd);
    for (i = 0; i < n; i ++) {
        obj = new shObject ();
        obj->loadState (fd);
        putObject (obj, obj->mX, obj->mY);
    }

    i = loadInt (fd);
    while (i--) {
        shCreature *c = new shCreature ();
        int ishero = loadInt (fd);
        if (ishero) {
            Hero.loadState (fd);
            Hero.mCreature = c;
        }
        c->loadState (fd);
        mCreatures[c->mX][c->mY] = c;
        mCrList.add (c);
        c->mLevel = this;
    }

    i = loadInt (fd);
    while (i--) {
        shFeature *f = new shFeature ();
        SREAD (fd, f, sizeof (shFeature));
        mFeatures.add (f);
        if (shFeature::kStairsUp == f->mType or
            shFeature::kStairsDown == f->mType)
        {
            mExits.add (f);
        }
    }
    loadMagic (fd, MAPEND);
}


void
shObject::saveState (int fd)
{
    saveMagic (fd, OBJBEGIN);
    saveTarget (fd, this);
    saveInt (fd, mIlkId);
    SWRITEBLOCK (fd, &mCount, &mOwner);
    if (kInventory == mLocation) {
        savePointer (fd, mOwner);
    }
    saveString (fd, mUserName);
    if (isA (kObjWreck)) {
        saveInt (fd, mWreckIlk);
    } else if (isA (kImplant)) {
        saveInt (fd, mImplantSite);
    } else {
        saveInt (fd, mColor);
    }
    saveMagic (fd, OBJEND);
}

void
shObject::loadState (int fd)
{
    loadMagic (fd, OBJBEGIN);
    loadStruct (fd, this);
    mIlkId = (shObjId) loadInt (fd);
    SREADBLOCK (fd, &mCount, &mOwner);
    if (kInventory == mLocation) {
        loadPointer (fd, (void **) &mOwner, "o.mOwner");
    }
    loadString (fd, &mUserName);
    if (isA (kObjWreck)) {
        mWreckIlk = (shMonId) loadInt (fd);
    } else if (isA (kImplant)) {
        mImplantSite = (shObjectIlk::Site) loadInt (fd);
    } else {
        mColor = loadInt (fd);
    }
    loadMagic (fd, OBJEND);
}


int
nameOK (const char *name)
{
    int fd;
    char filename[PRIME_PATH_LENGTH];
    const char *p;

    if (strlen (name) > HERO_NAME_LENGTH or
        strlen (name) < 1 or
        isspace (name[0]))
    {
        return 0;
    }
    for (p = name; *p; p++) {
        if (!isprint (*p) or '/' == *p or '\\' == *p)
            return 0;;
    }


    snprintf (filename, sizeof(filename)-1, "%s/%s.tmp", SCOREDIR, name);
    fd = open (filename, O_CREAT | O_WRONLY | O_EXCL | O_BINARY,
#ifdef _WIN32
               S_IWRITE | S_IREAD);
#else
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
#endif
    if (-1 == fd) {
        debug.log ("nameOK() couldn't create file %s:%d", filename, errno);
        return 0;
    }
    close (fd);
    unlink (filename);
    return 1;
}



/* returns 0 on success, -1 on failure*/
int
saveGame ()
{
    int fd;
    char savename[PRIME_PATH_LENGTH];
    int success = -1;
    PtrList.reset ();
    UpdateList.reset ();

    snprintf (savename, sizeof(savename)-1, "%s/%s.sav", UserDir, Hero.cr ()->mName);

retry:
    fd = open (savename, O_CREAT | O_WRONLY | O_EXCL | O_BINARY,
#ifdef _WIN32
               S_IWRITE | S_IREAD);
#else
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
#endif

    if (-1 == fd) {
        if (EEXIST == errno) {
            I->p ("Strangely, a save file already exists.");
            if (!I->yn ("Delete it?")) {
                return -1;
            }
            if (-1 == unlink (savename)) {
                I->p ("Couldn't unlink save file %s: %s",
                      savename, strerror (errno));
                return -1;
            }

            goto retry;
        } else {
            I->p ("Couldn't open the save file %s", savename);
            I->p ("%s", strerror (errno));
            return -1;
        }
    }

    I->p("Saving...");
    saveHeader (fd);

    saveInt (fd, Clock);
    saveScenario (fd);
    saveIlks (fd);
    saveLizards (fd);
    saveDiscoveries (fd);
    saveKills (fd);
    saveOptions (fd);
    saveMaze (fd);
    saveWarpTrace (fd);

    success = 0; /* if we got this far, it worked. */

    close (fd);
    return success;
}


/* returns 0 on success, -1 on failure*/
int
loadGame (const char* name)
{
    int fd;
    char savename[PRIME_PATH_LENGTH];
    int success = -1;

    if (!name)
        name = getenv ("USER");

    snprintf (savename, sizeof(savename)-1, "%s/%s.sav", UserDir, name);

    fd = open (savename, O_RDONLY | O_BINARY, 0);

    if (-1 == fd) {
        return -1;
    }

    I->p ("Loading save game...");

    if (loadHeader (fd)) {
        close (fd);
        if (!I->yn ("Erase it and start a new game?")) {
            delete I;
            exitPRIME (0);
        }
        unlink (savename);
        return -1;
    }

    Clock = loadInt (fd);
    loadScenario (fd);
    loadIlks (fd);
    initializeObjects ();
    loadLizards (fd);
    addLizardBreaths ();
    loadDiscoveries (fd);
    loadKills (fd);
    loadOptions (fd);
    loadMaze (fd);
    loadWarpTrace (fd);

    if (UpdateList.count ()) {
        int i;
        int giveup = 1;

        if (BOFH) {
            if (!I->yn ("Some pointers uninitialized, abort?")) {
                giveup = 0;
            }
        } else {
            I->p ("Crap! This save file is corrupt!");
            if (I->yn ("Erase the save file?"))
                unlink (savename);
        }
        for (i = 0; i < UpdateList.count (); i++) {
            UpdatePair *p = UpdateList.get (i);
            debug.log ("Still need object %d", p->mId);
        }
        if (giveup) abort ();
    }

    Level = Hero.cr ()->mLevel;

    if (Hero.getStoryFlag ("lost tail"))  Hero.amputateTail ();

    success = 0; /* if we got this far, it worked. */

    close (fd);

    I->p ("Save game loaded successfully.");

    if (BOFH and !I->yn ("erase save file?")) {
    } else {
        unlink (savename);
    }
    return success;
}
