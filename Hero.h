#ifndef HERO_H
#define HERO_H

#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Monster.h"



struct shStoryFlag
{
    const char *mName;
    int mValue;
    shStoryFlag () {}
    shStoryFlag (const char *name, int value)
    {
        mName = strdup (name);
        mValue = value;
    }
};


struct shHero
{
    int mBusy;
    int mScore;
    shProfession *mProfession;
    shCreature *mCreature;
    shVector <shCreature *> mPets;

    void saveState (int fd);
    void loadState (int fd);

    shCreature *cr ();
    int isHero () { return 1; }

    void init (const char *name, shProfession *profession);

    const char *getTitle ();

    void death (shCauseOfDeath how, shCreature *killer,
        shObject *implement, shAttack *attack, const char *killstr);
    void epitaph (char *buf, int len, shCauseOfDeath how,
        const char *killstr, shCreature *killer);
    int tallyScore ();
    void logGame (char *message);

    int wield (shObject *obj, int quiet = 0);

    void earnScore (int points);

    void lookAtFloor ();
    int getStoryFlag (const char *name) {
        int i;
        for (i = 0; i < mStoryFlags.count (); i++) {
            if (0 == strcmp (name, mStoryFlags.get (i) -> mName)) {
                return mStoryFlags.get (i) -> mValue;
            }
        }
        return 0;
    }

    void setStoryFlag (const char *name, int value) {
        int i;
        for (i = 0; i < mStoryFlags.count (); i++) {
            if (0 == strcmp (name, mStoryFlags.get (i) -> mName)) {
                mStoryFlags.get (i) -> mValue = value;
                return;
            }
        }
        mStoryFlags.add (new shStoryFlag (name, value));
    }

    void resetStoryFlag (const char *name) {
        setStoryFlag (name, 0);
    }

    int interrupt ();

    int isMute() { return getStoryFlag ("superglued tongue"); };

    void useBOFHPower ();

    /* Garbage compactor. */
    void enterCompactor ();
    void leaveCompactor ();
    /* Pet section. */
    void checkForFollowers (shMapLevel *level, int sx, int sy);
    int displace (shCreature *c);
    /* Environment perception. */
    void feel (int x, int y, int force = 0);

    void touchMonolith (shCreature *monl);

    void torcCheck ();

    int useMutantPower ();
    bool isPsyker ();

    void amputateTail ();

 private:
    shVector <shStoryFlag *> mStoryFlags;

};
#endif
