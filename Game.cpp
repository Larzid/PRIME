#include "Global.h"
#include "Util.h"
#include "Map.h"
#include "Interface.h"
#include "Hero.h"
#include "Game.h"
#include "Scenario.h"

int GameOver = 0;

shTime Clock;

shVector <shMapLevel *> Maze;  /* all the levels */

int
newGame (const char *name)
{

    I->pageLog ();
    Clock = 0;

    generate_scenario (Scen);
    randomizeIlkNames ();
    randomizeServiceNames ();
    initializeObjects ();
    addLizardBreaths ();

    shMapLevel::buildMaze ();
    Level = Maze.get (1);

    shProfession *profession = chooseProfession ();
    Hero.init (name, profession);

    return 0;
}


int
speedToAP (int speed)
{
    /* I used to use a complicated priority queue to order events, but I've
       now implemented a speed system that's borrowed from Angband. -- CADV */

    if (speed >= 100) {
        /* An extra turn for each +100 to speed. */
        return speed;
    } else {
        /* Every one else gets a turn for each -100 to speed. */
        return 10000 / (200 - speed);
    }
}


void
gameLoop ()
{
    shCreature *c;
    shMapLevel *oldlevel = Level;

    Level->computeVisibility (Hero.cr ());
    I->drawScreen ();

    while (!GameOver) {
        /* cleanup deleted objects (HACK) */
        purgeDeletedObjects ();
        /* cleanup any dead monsters */
        for (int i = Level->mCrList.count () - 1;  i >= 0;  --i) {
            c = Level->mCrList.get (i);
            if (kDead == c->mState) {
                Level->mCrList.remove (c); /* redundant */
                delete c;
            }
        }

        if (oldlevel != Level) {
            oldlevel = Level;
            /* Entering Gamma Caves with gammasensitivity power on may blind
               you before you are able to take a cursory glance of the area. */
            Hero.cr ()->computeIntrinsics ();
            Level->computeVisibility (Hero.cr ());
            I->drawScreen ();
            continue;
        }

        /* Q: Why hero turn is separated from monster turns?
           A: When hero changes level the creature list count
              will change and may lead to crash if she was
              to be handled within the loop. */

        if (Hero.cr ()->mAP >= 0) {
            Hero.cr ()->playerControl ();
        }

        /* Maybe some monsters have a turn. */
        for (int i = Level->mCrList.count () - 1;
             i >= 0 and !GameOver;
             --i)
        {
            c = Level->mCrList.get (i);
            if (kDead == c->mState) {
                continue;
            }
            if (c->mAP >= 0 and !c->isHero ())
                c->monsterControl ();

            /* Award some AP for next time */
            c->mAP += speedToAP (c->mSpeed);
        }

        /* level timeouts */
        if (Level->mTimeOuts.mCompactor > 0 and
            Clock > Level->mTimeOuts.mCompactor)
        {
            if (Level->mCompactorState < 10) {
                if (Level->mCompactorState == 4 and
                    Level->mCompactorHacked == 1)
                {
                    shCreature *h = Hero.cr ();
                    if (Level->isInGarbageCompactor (h->mX, h->mY)) {
                        I->p ("The walls suddenly halt just inches"
                              " short of crushing you!");
                    }
                    Level->mCompactorHacked = 0;
                    Level->mCompactorState = 21;
                } else { /* compact */
                    if (Level->mCompactorState %2)
                        Level->moveWalls (2);
                    else
                        Level->moveWalls (1);
                }
            } else if (Level->mCompactorState > 30) {
                /* done reseting, unlock doors */
                Level->mCompactorState = -1;
                Level->mTimeOuts.mCompactor = -1;
                Level->magDoors (-1); /* kludgey! */
            } else if (Level->mCompactorState > 20) {
                /* reset the trap */
                if (Level->mCompactorState % 2)
                    Level->moveWalls (-1);
                else
                    Level->moveWalls (-2);
            } else {
                Hero.resetStoryFlag ("walls moving");
                Hero.resetStoryFlag ("walls heard");
            }
            ++Level->mCompactorState;
            if (Level->mTimeOuts.mCompactor > 0)
                Level->mTimeOuts.mCompactor += FULLTURN * 3;
        }

        Clock += 100;

        if (0 == Clock % 1000) {
            for (int i = 0;
                 i < Level->mCrList.count () and !GameOver;
                 ++i)
            {
                int mod = (Clock % 10000) / 1000;
                shCreature *c = Level->mCrList.get (i);
                if (c->mState == kDead)
                    continue;
                if ((intptr_t(c) / 1000) % 10 == mod)
                    c->upkeep ();
            }
        }
    }
}

/* WIN32: wait for the user to press a key before exiting, because
   they're probably running outside cmd.exe and the window will
   otherwise disappear.  (I tried registering this code using
   atexit(), but my process kept getting killed for no apparent reason
   while awaiting input...  Grr!!) */
void
exitPRIME (const int code)
{
#ifdef _WIN32
    printf ("Press enter to exit...\n");
    getchar ();
#endif
    exit (code);
}
