#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Profession.h"
#include "Creature.h"
#include "Interface.h"
#include "Game.h"

#include <ctype.h>

void
shHero::earnScore (int points)
{
    mScore += points;
}

const char *
shHero::getTitle ()
{
    int level = mCreature ? mCreature->mCLevel : 0;
    if (level/3 < 10)
        return mProfession->mTitles[level/3];
    return mProfession->mTitles[9];
}

static int pickedupitem;


void
shCreature::oldLocation (int newX, int newY)
{
    if (GameOver or !isHero ())
        return;

    shRoom *oldroom = Level->getRoom (mX, mY);
    shRoom *newroom = Level->getRoom (newX, newY);

    if (oldroom == newroom)
        return;

    if (mLevel->isInShop (mX, mY))
        leaveShop ();

    if (mLevel->isInHospital (mX, mY))
        leaveHospital ();

    if (mLevel->isInGarbageCompactor (mX, mY))
        Hero.leaveCompactor ();
}


void
shCreature::newLocation ()
{
    if (GameOver or !isHero ())
        return;

    if (!mLastLevel) mLastLevel = Level;
    shRoom *oldroom = mLastLevel->getRoom (mLastX, mLastY);
    shRoom *newroom = Level->getRoom (mX, mY);

//    if (mLastLevel != Level or distance (this, mLastX, mLastY) > 10)
    Level->computeVisibility (this);

    I->drawScreen ();

    if (sewerSmells () and !is (kSickened)) {
        I->p ("What a terrible smell!");
        inflict (kSickened, 2000);
    }

    if (newroom != oldroom) {
        if (Level->isInShop (mX, mY)) {
            enterShop ();
        }
        if (Level->isInHospital (mX, mY)) {
            enterHospital ();
        }
        if (Level->isInGarbageCompactor (mX, mY)) {
            Hero.enterCompactor ();
        }

        if (newroom->mType == shRoom::kNest and !intr (kBlind)) {
            I->p ("You enter an alien nest!");
            /* Avoid giving this message again. */
            newroom->mType = shRoom::kNormal;
        }
    }

    /* FIXME: AAARGH!  In how many places is the autopickup code duplicated?! */
    pickedupitem = 0;

    shObjectVector *v = Level->getObjects (mX, mY);
    if (v) {
        int n = v->count ();
        for (int i = 0; i < n; i++) {
            shObject *obj = v->get (i);
            shObjectType type = obj->apparent ()->mType;
            /* Cash gets picked up for free. */
            if (type == kMoney or obj->is (obj::thrown) or
                (Flags.mAutopickup and Flags.mAutopickupTypes[type] and
                 !obj->is (obj::unpaid)))
            {
                interrupt ();
                if (!intr (kBlind))
                    obj->set (obj::known_appearance);
                /* HACK: as discussed below (see case shInterface::kPickup:),
                   remove obj from floor before attempting to add to
                   inventory: */
                v->remove (obj);
                if (addObjectToInventory (obj)) {
                    --i;
                    --n;
                    pickedupitem++;
                    if (Level->isInShop (mX, mY)) {
                        pickedUpItem (obj);
                    }
                } else {
                    /* END HACK: add the object back to the end of the
                       vector, and don't look at it again: */
                    v->add (obj);
                    --n;
                }
            }
        }
        if (!v->count ()) {
            delete v;
            Level->setObjects (mX, mY, NULL);
        }
    }
}

/* Called every time after hero makes a move. */
void
shHero::lookAtFloor ()
{
    int x = cr ()->mX, y = cr()->mY;
    shFeature *f = Level->getFeature (x, y);
    int feelobjs = 0;
    shObjectVector *v;

    feel (x, y);

    /*
    if (kSewage == Level->getSquare (x, y) -> mTerr) {
        if (0 == mZ and 0 == objcnt) {
            I->p ("You are knee deep in sewage.");
        }
    }
    */

    /* Walking by staircases and sludge vats generates message. */
    if (f) {
        switch (f->mType) {
        case shFeature::kStairsUp:
            I->p ("There is a staircase up here.  Press '%s' to ascend.",
                  I->getKeyForCommand (shInterface::kMoveUp));
            break;
        case shFeature::kStairsDown:
            I->p ("There is a staircase down here.  Press '%s' to descend.",
                  I->getKeyForCommand (shInterface::kMoveDown));
            break;
        case shFeature::kVat:
            I->p ("There is %s here.", f->getDescription ()); break;
        default: break;
        }
        interrupt ();
    }

    /* If there are objects on this square mention this. */
    int objcnt = Level->countObjects (x, y);
    if (!objcnt)  return;
    if (Level->isWatery (x, y)) {
        if (kSewage == Level->getSquare (x, y) -> mTerr) {
            I->p ("You feel around in the sewage...");
            feelobjs = 1;
        }
    } else if (cr ()->intr (kBlind)) {
        if (cr ()->intr (kFlying) and cr ()->mZ == 1) return;
        I->p ("You feel around the floor...");
        feelobjs = 1;
    }
    interrupt ();
    v = Level->getObjects (x, y);

    for (int i = 0; i < v->count (); ++i) {
        shObject *obj = v->get (i);
        cr ()->innate_knowledge (obj);
        if (!feelobjs)
            obj->set (obj::known_appearance);
    }

    if (objcnt > I->freeLogLines () - 1 and objcnt > 1) {
        I->p ("There are several objects here.");
        return;
    }
    if (1 == objcnt) {
        I->p ("You %s %s.", feelobjs ? "find" : "see here",
              v->get (0) -> inv ());
        return;
    }
    I->p ("Things that are here:");
    for (int i = 0; i < objcnt; ++i) {
        I->p (v->get (i) -> inv ());
    }
}

/* Yep.  Just for wanton of ADOM-like fancy messaging.
   The "you find a trap!", "you find another trap!" and
   "you find yet another trap!" has impressed me so much
   I wanted to implement it in PRIME too.  -- MB */
static void
yetAnotherThing (int n, shFeature *f, const char *str)
{
    if (f)
        I->p (str, n > 1 ? "yet another " : n > 0 ? "another " : "",
              n > 0 ? f->getShortDescription () : AN (f));
    else
        I->p (str, n > 1 ? "yet another" : n > 0 ? "another" : "a");
}

static void
yetAnotherCreature (int n, shCreature *c, const char *str)
{
    I->p (str, n > 1 ? "yet another " : n > 0 ? "another " : "",
          n > 0 ? c->myIlk ()->mName : c->an ());
}

int
shCreature::spotStuff (bool force)
{
    if (intr (kPerilSensing)) {
        sensePeril ();
    }
    if (intr (kBlind))  return 0;

    const int SPOT_INTERVAL = 160 * FULLTURN;
    int sk = getSkillModifier (kSpot);
    int score;

    int crits = 0;
    shVector<shMonId> spotted_c;
    for (int i = 0; i < Level->mCrList.count (); ++i) {
        shCreature *c = Level->mCrList.get (i);
        if (c == this or !c->isHostile () or !canSee (c->mX, c->mY)) {
            continue;
        }
        bool mayspot = force or c->mSpotAttempted + SPOT_INTERVAL < Clock;
        if (canSee (c->mX, c->mY) and c->mHidden > 0 and mayspot and
            distance (this, c->mX, c->mY) < 30)
        {
            c->mSpotAttempted = Clock;
            score = D20 () + sk;
            debug.log ("spot check: %d", score);
            if (score > c->mHidden) {
                ++crits;
                int cnt = 0;
                for (int j = 0; j < spotted_c.count (); ++j)
                    if (spotted_c.get (j) == c->mIlkId)  ++cnt;
                yetAnotherCreature (cnt, c, "You spot %s%s!");
                spotted_c.add (c->mIlkId);
                c->mHidden *= -1;
                Level->drawSq (c->mX, c->mY);
                I->pauseXY (c->mX, c->mY);
                interrupt ();
            }
        }
    }

    int sdoors = 0, malf = 0, other = 0;
    shVector<shFeature::Type> spotted_f;
    for (int i = 0; i < Level->mFeatures.count (); ++i) {
        shFeature *f = Level->mFeatures.get (i);
        bool mayspot = force or f->mSpotAttempted + SPOT_INTERVAL < Clock;
        if (canSee (f->mX, f->mY) and mayspot and
            (distance (this, f->mX, f->mY) < 30))
        {
            f->mSpotAttempted = Clock;
            score = D20 () + sk;
            debug.log ("spot check: %d", score);
            if ((shFeature::kDoorHiddenHoriz == f->mType or
                 shFeature::kDoorHiddenVert == f->mType) and
                score >= 22)
            {
                yetAnotherThing (sdoors++, NULL, "You spot %s secret door!");
                f->mType = shFeature::kDoorClosed;
                f->mSportingChance = 0;
                f->mTrapUnknown = 0;
                Level->drawSq (f->mX, f->mY);
                I->pauseXY (f->mX, f->mY);
                interrupt ();
            } else if (f->isTrap () and f->mTrapUnknown and score >= 22) {
                if (f->isBerserkDoor ()) {
                    yetAnotherThing (malf++, NULL,
                                     "You spot %s malfunctioning door!");
                } else {
                    ++other;
                    int cnt = 0;
                    for (int j = 0; j < spotted_f.count (); ++j)
                        if (spotted_f.get (j) == f->mType)  ++cnt;
                    yetAnotherThing (cnt, f, "You spot %s%s!");
                    spotted_f.add (f->mType);
                }
                f->mTrapUnknown = 0;
                f->mSportingChance = 0;
                Level->drawSq (f->mX, f->mY);
                I->pauseXY (f->mX, f->mY);
                interrupt ();
            }
        }
    }
    /* Return number of things discovered. */
    return crits + sdoors + malf + other;
}


static void
checkFeature (shFeature *f, int sk, shVector<shFeature::Type> *found,
              int *sdoor, int *malf)
{
    int score = D20 () + sk + f->mSportingChance++;
    if ((shFeature::kDoorHiddenHoriz == f->mType or
         shFeature::kDoorHiddenVert == f->mType) and
        score >= 20)
    {
        f->mType = shFeature::kDoorClosed;
        f->mTrapUnknown = 0;
        f->mSportingChance = 0;
        yetAnotherThing (*sdoor++, NULL,
                         "You find %s secret door!");
        Hero.feel (f->mX, f->mY);
        Level->drawSq (f->mX, f->mY);
        I->pauseXY (f->mX, f->mY);
        Hero.interrupt ();
    } else if ((shFeature::kDoorClosed == f->mType or
                shFeature::kDoorOpen == f->mType) and
               shFeature::kBerserk & f->mDoor.mFlags and
               f->mTrapUnknown and
               score >= 12)
    {
        yetAnotherThing (*malf++, NULL,
                         "You find malfunction in %s door.");
        f->mTrapUnknown = 0;
        f->mSportingChance = 0;
        Hero.feel (f->mX, f->mY);
        Level->drawSq (f->mX, f->mY);
        I->pauseXY (f->mX, f->mY);
        Hero.interrupt ();
    } else if (f->isTrap () and
               f->mTrapUnknown and
               score >= 20)
    {
        int cnt = 0;
        for (int j = 0; j < found->count (); ++j)
            if (found->get (j) == f->mType)  ++cnt;
        yetAnotherThing (cnt, f, "You find %s%s!");
        found->add (f->mType);
        f->mTrapUnknown = 0;
        f->mSportingChance = 0;
        Hero.feel (f->mX, f->mY);
        Level->drawSq (f->mX, f->mY);
        I->pauseXY (f->mX, f->mY);
        Hero.interrupt ();
    }
}

void
shCreature::doSearch (void)
{
    int sk = getSkillModifier (kSearch);
    int sdoor = 0, malf = 0;
    shVector<shFeature::Type> found;
    for (int x = mX - 1; x <= mX + 1; ++x)
        for (int y = mY - 1; y <= mY + 1; ++y)
            if (Level->isInBounds (x, y)) {
                shFeature *f = Level->getFeature (x, y);
                if (isHero ())  Hero.feel (x, y);
                if (f)  checkFeature (f, sk, &found, &sdoor, &malf);
            }
}


int
shCreature::tryToTranslate (shCreature *c)
{
#define TALKERIDLEN 20
    char talkerid[TALKERIDLEN];
    const char *talker = c->the ();
    const char *langstr = c->isRobot () ? "beeps and chirps" : "alien language";
    int i;

    snprintf (talkerid, TALKERIDLEN, "translate %p", (void *) c);

    if (intr (kTranslation)) {
        if (mInnateIntrinsics.get (kTranslation)) {
            if (3 != Hero.getStoryFlag (talkerid)) {
                I->p ("You manage to comprehend %s's %s:", talker, langstr);
                Hero.setStoryFlag (talkerid, 3);
            }
            return 1;
        }

        assert (mImplants[shObjectIlk::kLeftEar] or
                NOT0 (mHelmet,->isA (kObjBioMask)));

        shObject *translator;
        (translator = mImplants[shObjectIlk::kLeftEar]) or
            (translator = mHelmet);

        if (1 != Hero.getStoryFlag (talkerid)) {
            I->p ("%s translates %s's %s:", YOUR (translator), talker, langstr);
            Hero.setStoryFlag (talkerid, 1);
            translator->set (obj::known_type);
        }
        return 1;
    }

    for (i = 0; i < Hero.mPets.count (); i++) {
        shCreature *pet = Hero.mPets.get (i);

        if (pet->isA (kMonProtocolDroid) and pet->mLevel == mLevel and
            distance (pet, c) < 5 * 20 and distance (this, pet) < 5 * 20)
        {
            if (2 != Hero.getStoryFlag (talkerid)) {
                I->p ("%s translates %s's %s:", pet->your (), talker, langstr);
                Hero.setStoryFlag (talkerid, 2);
            }
            return 1;
        }
    }
    return 0;
}


int
shCreature::looksLikeJanitor ()
{
    return myIlk ()->mType == kHumanoid and
        NOT0 (mWeapon,->isA (kObjMop)) and
        NOT0 (mJumpsuit,->isA (kObjJanitorUniform)) and
        (!mBodyArmor or
            (isHero () and Hero.mProfession == Janitor));
}


void
shCreature::sensePeril ()
{
    int oldperil = mGoggles->is (obj::toggled);
    int newperil = 0;

    if (oldperil) {
        mGoggles->clear (obj::toggled);
        computeIntrinsics ();
    }

    for (int i = 0; i < Level->mCrList.count (); i++) {
        shCreature *c = Level->mCrList.get (i);
        if (c == this or !c->isHostile () or !canSee (c->mX, c->mY)) {
            continue;
        }
        newperil++;
        break;
    }

    if (!newperil)
        for (int i = 0; i < Level->mFeatures.count (); i++) {
            shFeature *f = Level->mFeatures.get (i);
            if (f->isTrap () and canSee (f->mX, f->mY)) {
                newperil++;
                break;
            }
        }

    const char *your_goggles = YOUR (mGoggles);
    if (!oldperil and newperil) {
        bool wasblind = intr (kBlind);
        mGoggles->set (obj::toggled);
        if (isHero ())  interrupt ();
        computeIntrinsics ();
        if (intr (kXRayVision)) {
            msg (fmt ("%s have darkened a bit.", your_goggles));
        } else if (!wasblind) {
            msg (fmt ("%s have turned black!", your_goggles));
        }
        mGoggles->set (obj::known_type);
    } else if (oldperil and !newperil) {
        mGoggles->clear (obj::toggled);
        computeIntrinsics ();
        if (!intr (kBlind)) {
            msg (fmt ("%s have turned transparent.", your_goggles));
            mGoggles->set (obj::known_type);
        }
    } else if (oldperil) {
        mGoggles->set (obj::toggled);
        computeIntrinsics ();
    }
}

/* returns 1 if interrupted, 0 if not busy anyway */
int
shCreature::interrupt ()
{
    if (!isHero ())  return 0;
    if (Hero.mBusy) {
        Level->computeVisibility (this);
        I->drawScreen ();
        Hero.mBusy = 0;
        return 1;
    }
    return 0;
}

int
shHero::interrupt ()
{
    return mCreature->interrupt ();
}

static void
listDiscoveries ()
{
    shObjectType old = kUninitialized;
    char *buf = GetBuf ();
    int known = 0;
    shMenu *menu = I->newMenu ("Discoveries:", shMenu::kNoPick);
    for (int i = 0; i < kObjNumIlks; ++i) {
        bool isIdentified = AllIlks[i].mFlags & kIdentified;
        bool isNamed = AllIlks[i].mUserName;
        bool isPreidentified = AllIlks[i].mFlags & (kPreidentified ^ kIdentified);
        if ((isIdentified or isNamed) and !isPreidentified) {
            /* Try using headers for clarity. */
            shObjectType t = AllIlks[i].mReal.mType;
            if (t != old and t >= kMoney and t <= kEnergyCell) {
                menu->addHeader (objectTypeHeader[t]);
                old = t;
            }
            if (isIdentified)
                snprintf (buf, SHBUFLEN, "* %-35s * %-35s",
                    AllIlks[i].mAppearance.mName, AllIlks[i].mReal.mName);
            else /* isNamed */
                snprintf (buf, SHBUFLEN, "* %-35s ? %-35s",
                    AllIlks[i].mAppearance.mName, AllIlks[i].mUserName);
            menu->addText (buf);
            ++known;
        }
    }
    if (known) {
        menu->finish ();
    } else {
        I->p ("You recognize nothing.");
    }
    delete menu;
}


int
shCreature::listInventory ()
{
    if (!mInventory->count ()) {
        msg ("You aren't carrying anything!");
    } else {
        shMenu *menu = I->newMenu ("Inventory", shMenu::kCategorizeObjects);
        shObject *obj;
        for (int i = 0; i < mInventory->count (); ++i) {
            obj = mInventory->get (i);
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        do {
            menu->getPtrResult ((const void **) &obj);
            /* Only examination possible after death. */
            if (obj and GameOver) {
                obj->showLore ();
                menu->dropResults ();
            }
        } while (obj and GameOver);
        delete menu;
        if (GameOver) return 0;
        if (obj) {
            I->pageLog ();
            I->p ("Do what with %s?", obj->inv ());
            struct {
                shInterface::Command cmd;
                const char *label;
            } action_list[] =
            {
                {shInterface::kAdjust,    "adjust"},
                {shInterface::kUse,       "apply"},
                {shInterface::kDrop,      "drop"},
                {shInterface::kExamine,   "examine"},
                {shInterface::kExecute,   "execute"},
                {shInterface::kName,      "name"},
                {shInterface::kTakeOff,   "take off"},
                {shInterface::kThrow,     "throw"},
                {shInterface::kPay,       "pay for"},
                {shInterface::kQuaff,     "quaff"},
                {shInterface::kWear,      "wear"},
                {shInterface::kWield,     "wield"},
                {shInterface::kZapRayGun, "zap"}
            };
            const unsigned int num_actions = sizeof(action_list) / sizeof(action_list[0]);
            for (unsigned int i = 0; i < num_actions; ++i)
            {
                I->setColor (kWhite);
                I->p ("%6s", I->getKeyForCommand (action_list[i].cmd));
                I->setColor (kGray);
                I->nonlOnce ();
                I->p (" - %s", action_list[i].label);
                char fmt[10];
                sprintf (fmt, "%%%ds", 10 - strlen (action_list[i].label));
                I->nonlOnce ();
                I->p (fmt, " ");
                if ((i % 4) != 3)
                    I->nonlOnce ();
            }

            extern bool minimapOn;
            minimapOn = false;
            int retval = objectVerbCommand (obj);
            minimapOn = true;
            return retval;
        }
    }
    return 0;
}

void
shCreature::adjust (shObject *obj1)
{
    if (!obj1) {
        obj1 = quickPickItem (mInventory, "adjust", shMenu::kCategorizeObjects);
    }
    if (!obj1)  return;
    I->p ("Adjust to what letter?");
    char c = I->getChar ();
    if (!isalpha (c) or c == obj1->mLetter) {
        I->nevermind ();
        return;
    }
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj2 = mInventory->get (i);
        if (obj2->mLetter == c) { /* Swap letters with another item. */
            obj2->mLetter = obj1->mLetter;
            obj1->mLetter = c;
            obj1->announce ();
            obj2->announce ();
            return;
        }
    }
    obj1->mLetter = c; /* Adjust letter to unused one. */
    obj1->announce ();
}

static void
nameObject (shObject *obj)
{
    int ilk = !I->yn ("Name an individual object?");
    if (!obj) {
        obj = Hero.cr ()->quickPickItem (Hero.cr ()->mInventory, "name",
            shMenu::kCategorizeObjects);
    }
    if (obj) {
        if (ilk) {
            obj->nameIlk ();
        } else {
            obj->name ();
        }
    }
}

static int
tryToThrowObject (shObject *obj)
{
    if (obj->isWeldedWeapon () and !Hero.cr ()->isOrc ()) {
        obj->set (obj::known_bugginess);
        I->p ("Your weapon is welded to your hands!");
        return HALFTURN;
    } else if (obj->is (obj::worn)) {
        I->p ("You can't throw that because you're wearing it!");
    } else {
        shDirection dir = I->getDirection ();
        if (kNoDirection == dir) return 0;

        shObject *stack = obj;
        obj = Hero.cr ()->removeOneObjectFromInventory (obj);
        return Hero.cr ()->throwObject (obj, stack, dir);
    }
    return 0;
}

static shObject *
chooseComputer ()
{
    shObjectVector v;
    selectObjects (&v, Hero.cr ()->mInventory, computer);
    if (!v.count ()) {
        I->p ("You don't have a computer!");
        return NULL;
    } else if (Hero.cr ()->is (kUnableCompute)) {
        I->p ("You will not employ thinking machines.");
        return NULL;
    } else if (1 == v.count ()) {
        return v.get (0);
    } else {
        for (int i = 0; i < v.count (); ++i) {
            if (v.get (i)->is (obj::designated)) {
                return v.get (i);
            }
        }
        shObject *computer = Hero.cr ()->quickPickItem (&v, "compute with", 0);
        return computer;
    }
}

static shObject *
tryToDrop (shObject *obj, int cnt)
{
    if (obj == Hero.cr ()->mWeapon) {
        if (obj->isWeldedWeapon () and !Hero.cr ()->isOrc ()) {
            I->p ("Your weapon is welded to your hands.");
            obj->set (obj::known_bugginess);
            return NULL;
        }
        if (0 == Hero.cr ()->unwield (obj)) {
            return NULL;
        }
    }
    if (obj->is (obj::worn)) {
        I->p ("You can't drop that because you're wearing it.");
        return NULL;
    }
    return Hero.cr ()->removeSomeObjectsFromInventory (obj, cnt);
}

int
shCreature::doWear (shObject *obj)
{
    if (!isHero ())  return 0;
    if (obj->has_subtype (computer)) {
        if (obj->is (obj::designated)) {
            I->p ("It is already your designated machine for running software.");
            return 0;
        }
        for (int i = 0; i < mInventory->count (); ++i) {
            shObject *o = mInventory->get (i);
            if (o->has_subtype (computer) and o->is (obj::designated))
                o->clear (obj::designated);
        }
        I->p ("Whenever you have more than one computer to run software you will use:");
        obj->announce ();
        obj->set (obj::designated);
        I->drawSideWin (this); /* Update equippy chars. */
        return 0; /* Meta action takes no time. */
    }

    int elapsed = 0;
    /* Many implants need the possibility to remove helmet temporarily. */
    if (obj->isA (kImplant) and mHelmet and mHelmet->isBuggy () and !isOrc () and
        obj->mIlkId != kObjTorc and obj->mIlkId != kObjMechaDendrites)
    {   /* FIXME: but what about in vacuum/poison gas cloud? CADV */
        I->p ("You can't remove your helmet to install the implant!");
        if (!mHelmet->is (obj::known_bugginess)) {
            I->p ("It must be buggy!");
            mHelmet->set (obj::known_bugginess);
            elapsed = FULLTURN;
        }
        return elapsed;
    }
    /* Mecha-dendrites and sealed helmets don't mix. */
    if (obj->isA (kObjMechaDendrites) and
        mHelmet and mHelmet->isSealedArmor ())
    {
        I->p ("%s conflict with %s.", obj->getShortDescription (),
            mHelmet->getShortDescription ());
        return elapsed;
    }
    /* Armor piece might need to be swapped out. */
    shObject *swap = NULL;
    if (obj->has_subtype (jumpsuit) and mJumpsuit) {
        /* Complicated case first. */
        if (mBodyArmor and mBodyArmor->isBuggy () and !isOrc ()) {
            if (mBodyArmor->is (obj::known_bugginess)) {
                I->p ("Your armor is buggy.  You can't take it off to wear jumpsuit.");
            } else {
                I->p ("You can't seem to take off your armor.  It must be buggy!");
                mBodyArmor->set (obj::known_bugginess);
                elapsed = FULLTURN;
            }
            return elapsed;
        }
        swap = mJumpsuit;
    } else if (obj->has_subtype (body_armor) and mBodyArmor) {
        swap = mBodyArmor;
    } else if (obj->has_subtype (helmet) and mHelmet) {
        swap = mHelmet;
    } else if (obj->has_subtype (goggles) and mGoggles) {
        swap = mGoggles;
    } else if ((obj->has_subtype (cloak) or obj->isA (kObjPlasmaCaster)) and mCloak) {
        swap = mCloak;
    } else if (obj->has_subtype (belt) and mBelt) {
        swap = mBelt;
    } else if (obj->has_subtype (boots) and mBoots) {
        swap = mBoots;
    }
    /* Swapped armor piece must be not buggy for non-orcs. */
    if (swap and swap->isBuggy () and !isOrc ()) {
        I->p ("You can't take off %s.", YOUR (swap));
        if (!swap->is (obj::known_bugginess)) {
            I->p ("It must be buggy!");
            swap->set (obj::known_bugginess);
            elapsed = FULLTURN;
        }
        return elapsed;
    }
    if (swap) {
        I->p ("You take off %s.", YOUR (swap));
        doff (swap);
    }
    /* You could have wielded the item previously.  Not a problem. */
    if (obj->is (obj::wielded)) {
        /* FIXME: this allows to switch welded plasma caster to shoulder. */
        unwield (obj);
    }
    elapsed = FULLTURN;
    int donned = don (obj);

    /* Thematic message for wearing equipment of xenophobic marines. */
    if (donned and Hero.mProfession == SpaceMarine and
        obj->isXenosHunterGear ())
    {
        I->p (RNG (2) ? "A thought crosses your mind:"
                      : "You notice yourself thinking:");
        const char *thought[] = {
            "Blessed is the mind too small for doubt.",
            "Innocence proves nothing.",
            "Know the mutant; kill the mutant.",
            "There is no such thing as innocence, only degrees of guilt.",
            "It is better to die for the Emperor than to live for yourself."
        };
        const int nthoughts = sizeof (thought) / sizeof (char *);
        I->p ("\"%s\"", thought [RNG (nthoughts)]);
    }
    return elapsed;
}

int
shCreature::doTakeOff (shObject *obj)
{
    if (!isHero ())  return 0;
    int elapsed = 0;
    /* Weapon is a special case. */
    if (obj == mWeapon) {
        if (0 == wield (NULL)) {
            return 0;
        }
        return HALFTURN;
    } /* So is designated computing machine. */
    if (obj->has_subtype (computer)) {
        obj->clear (obj::designated);
        I->p ("You will be asked to choose computer each time if you have more than one.");
        I->drawSideWin (this); /* Update equippy chars. */
        return 0; /* Meta action takes no time. */
    }
    /* Body armor must not be buggy to manipulate jumpsuit. */
    if (obj == mJumpsuit and mBodyArmor) {
        if (mBodyArmor->isBuggy () and !isOrc ()) {
            if (!mBodyArmor->is (obj::known_bugginess)) {
                I->p ("You can't seem to take your armor off.  It must be buggy!");
                mBodyArmor->set (obj::known_bugginess);
                elapsed = FULLTURN;
            } else {
                I->p ("You can't take your armor off because it is buggy.");
                I->p ("You need to do so in order to take off your jumpsuit.");
            }
            return elapsed;
        }
    }
    /* Helmet must not be buggy to manipulate most implants. */
    if (obj->isA (kImplant) and mHelmet and mHelmet->isBuggy () and !isOrc ()
        and obj->mIlkId != kObjTorc and obj->mIlkId != kObjMechaDendrites)
    {   /* FIXME: but what about in vacuum/poison gas cloud? CADV */
        if (!mHelmet->is (obj::known_bugginess)) {
            I->p ("You can't remove your helmet!  It must be buggy!");
            mHelmet->set (obj::known_bugginess);
            elapsed = HALFTURN;
        } else {
            I->p ("You can't take your helmet off because it is buggy.");
            I->p ("You need to do to free way for implants leaving your cranium.");
        }
        return elapsed;
    }

    /* Buggy belts may be forcibly taken off. */
    if (obj->isBuggy () and obj->has_subtype (belt) and !isOrc ()) {
        if (!obj->is (obj::known_bugginess)) {
            I->p ("You find %s is buggy.", YOUR (obj));
            obj->set (obj::known_bugginess);
            elapsed = HALFTURN;
        }
        if (mAbil.Str () >= 16 or
            (mAbil.Str () >= 14 and !obj->isA (kObjArmoredBelt))) {
            I->p ("You force the belt open.");
            if (RNG (2)) {
                I->nonlOnce ();
                I->p ("  It was destroyed in the process.");
                doff (obj);
                removeObjectFromInventory (obj);
                delete obj;
                return FULLTURN;
            }
        } else {
            if (obj->isA (kObjArmoredBelt) and mAbil.Str () >= 14 and
                (!obj->is (obj::known_appearance) or !obj->is (obj::known_type))) {
                I->p ("Nnnggh!  This must be an armored belt.");
                I->p ("You are strong enough to force any other belt type open.");
                obj->set (obj::known_type);
                elapsed += FULLTURN;
                return elapsed;
            } else {
                I->p ("You are too weak to forcibly open it.");
                return elapsed;
            }
        }
    } else if (obj->isA (kObjUngooma)) {
        I->p ("The parasite refuses to leave.");
        return 0;
    /* Item itself must not be buggy. */
    } else if (obj->isBuggy () and
               (!isOrc () or (obj->isA (kImplant) and !obj->isA (kObjTorc))))
    {
        if (obj->mIlkId == kObjTorc) {
            I->p ("It won't come out!  It must be some kind of trap of those fey folks!");
            if (!obj->is (obj::known_appearance)) {
                obj->set (obj::known_appearance);
                elapsed = FULLTURN;
            }
        } else if (!obj->is (obj::known_bugginess)) {
            if (obj->isA (kImplant)) {
                I->p ("It won't come out!  It must be buggy!");
            } else {
                I->p ("You can't seem to take it off.  It must be buggy!");
            }
            obj->set (obj::known_bugginess);
            elapsed = FULLTURN;
        } else if (obj->isA (kImplant)) {
            I->p ("It is buggy and will not respond to recall command.");
        } else {
            I->p ("To take it off it mustn't be buggy.");
        }
        return elapsed;
    }
    const char *verb = obj->isA (kImplant) ? "uninstall" : "take off";
    I->p ("You %s %s.", verb, YOUR (obj));
    doff (obj);
    return FULLTURN;
}

static int
doQuaff (shObject *obj)
{
    if (obj->isA (kCanister) or obj->isA (kObjFlamerFuel))
        return Hero.cr ()->quaffCanister (obj);
    return obj->myIlk ()->mQuaffFunc (Hero.cr (), obj);
}

static bool
HeroIsWebbedOrTaped ()
{
    shCreature *h = Hero.cr ();
    if (h->mTrapped.mWebbed or h->mTrapped.mTaped) {
        const char *something = h->mTrapped.mWebbed and h->mTrapped.mTaped ?
            "webs and duct tape" : h->mTrapped.mTaped ? "duct tape" : "webs";
        I->p ("Not while you are held by %s.", something);
        return true;
    }
    return false;
}

static bool
HeroCanThrow ()
{
    if (HeroIsWebbedOrTaped ())
        return false;
    if (Hero.cr ()->is (kFrightened)) {
        I->p ("You are too afraid to throw anything!");
        return false;
    }
    if (Hero.cr ()->is (kUnableAttack)) {
        I->p ("What an insulting display of belligerence.");
        return false;
    }
    return true;
}

static bool
HeroCanZap ()
{
    if (HeroIsWebbedOrTaped ())
        return false;
    if (Hero.cr ()->is (kFrightened)) {
        I->p ("You are too afraid to use a raygun!");
        return false;
    }
    if (Hero.cr ()->is (kUnableAttack)) {
        I->p ("What an insulting display of belligerence.");
        return false;
    }
    return true;
}

static bool
HeroCanFire ()
{
    if (HeroIsWebbedOrTaped ())
        return false;
    /* Caller has to check if weapon is NULL.  Here it cannot be. */
    shObject *held = Hero.cr ()->mWeapon;
    int dothrow = !held->myIlk ()->mGunAttack
                  and !held->isA (kRayGun)
                  and !held->has_subtype (computer);
    if (Hero.cr ()->is (kFrightened)) {
        I->p ("You are too afraid to %s your weapon!",
              dothrow ? "throw" : "fire");
        return false;
    }
    if (Hero.cr ()->is (kUnableAttack)) {
        I->p ("What a blatant display of belligerence.");
        return false;
    }
    return true;
}

static bool
HeroCanExecute (shObject *computer)
{
    /* Intentionally silent check at first. */
    bool webortape = Hero.cr ()->mTrapped.mWebbed or Hero.cr ()->mTrapped.mTaped;
    if (!webortape)
        return true;
    if (computer->isA (kObjBioComputer)) {
        if (Hero.isMute ()) {
            I->p ("Being mute, you are unable to command %s.", YOUR (computer));
            return false;
        } else {
            return true;
        }
    }

    HeroIsWebbedOrTaped ();
    return false;
}

static bool
HeroCanDrink ()
{
    if (Hero.getStoryFlag ("superglued tongue")) {
        I->p ("You can't drink anything with this stupid canister "
                "glued to your mouth!");
        return false;
    }
    return true;
}

int /* Time elapsed. */
shCreature::objectVerbCommand (shObject *obj)
{
    shInterface::Command cmd = I->getCommand ();
    switch (cmd) {
    case shInterface::kAdjust:
        adjust (obj);
        return 0;
    case shInterface::kDrop:
        obj = tryToDrop (obj, obj->mCount);
        if (!obj) return 0;
        drop (obj);
        Hero.feel (mX, mY);
        return HALFTURN;
    case shInterface::kExamine:
        obj->showLore ();
        return 0;
    case shInterface::kExecute:
    {
        if (!obj->myIlk ()->mRunFunc) {
            I->p ("You cannot execute this.");
            return 0;
        }
        shObject *computer = chooseComputer ();
        if (!computer)  return 0;
        if (!HeroCanExecute (computer))  return 0;
        return computer->executeProgram (obj);
    }
    case shInterface::kName:
        nameObject (obj);
        return 0;
    case shInterface::kPay:
    {
        int price = obj->mCount * obj->myIlk ()->mCost;
        shCreature *shopkeeper = Level->getShopKeeper (mX, mY);
        if (!obj->is (obj::unpaid)) {
            I->p ("You don't need to pay for it.");
        } else if (countMoney () < price) {
            I->p ("You do not have enough money to purchase it.");
        } else if (!shopkeeper) {
            I->p ("There's nobody around to pay.");
        } else {
            loseMoney (price);
            shopkeeper->gainMoney (price);
            obj->clear (obj::unpaid);
            I->p ("You buy %s for %d buckazoids.", THE (obj), price);
        }
        return 0;
    }
    case shInterface::kQuaff:
        if (!HeroCanDrink ())
            return 0;
        if (!obj->myIlk ()->mQuaffFunc) {
            I->p ("You cannot drink this.");
            return 0;
        }
        return doQuaff (obj);
    case shInterface::kTakeOff:
        if (!obj->isEquipped ()) {
            I->p ("You haven't equipped this item.");
            return 0;
        }
        return doTakeOff (obj);
    case shInterface::kThrow:
    {
        if (!HeroCanThrow ()) return 0;
        return tryToThrowObject (obj);
    }
    case shInterface::kUse:
        if (obj->myIlk ()->mUseFunc) {
            if (HeroIsWebbedOrTaped ())  return 0;
            return obj->myIlk ()->mUseFunc (this, obj);
        } else {
            I->p ("This object cannot be applied.");
            return 0;
        }
    case shInterface::kWear:
        if (!obj->canBeWorn ()) {
            I->p ("%s cannot be worn.", THE (obj));
            return 0;
        }
        if (obj->is (obj::worn)) {
            I->p ("You are wearing this already.");
            return 0;
        }
        return doWear (obj);
    case shInterface::kWield:
        if (!wield (obj)) {
            return 0;
        } else {
            return HALFTURN;
        }
    case shInterface::kZapRayGun:
    {
        if (!obj->myIlk ()->mZapAttack) {
            I->p ("You cannot zap this.");
            return 0;
        }
        if (!HeroCanZap ()) return 0;

        shDirection dir = I->getDirection ();
        if (kNoDirection == dir) {
            return 0;
        }
        return shootWeapon (obj, dir);
    }
    default:
        I->nevermind ();
        return 0;
    }
}

/* returns non-zero if the hero dies */
int
shCreature::instantUpkeep ()
{
    bool change = false;
    for (int i = mInventory->count () - 1; i >= 0; --i) {
        shObject *obj = mInventory->get (i);
        if ((obj->is (obj::active) and obj->myIlk ()->mEnergyUse)
            or obj->has_subtype (power_plant))
        {
            if (MAXTIME == obj->mLastEnergyBill) {
                /* unnecessary, should be handled by set (obj::active) */
                obj->mLastEnergyBill = Clock;
            } else {
                while (Clock - obj->mLastEnergyBill > obj->myIlk ()->mEnergyUse) {
                    if (obj->has_subtype (power_plant)) {
                        gainEnergy (1);
                    } else if (0 == loseEnergy (1)) {
                        obj->clear (obj::active);
                        I->p ("%s has shut itself off.", YOUR(obj));
                        change = true;
                    }
                    obj->mLastEnergyBill += obj->myIlk ()->mEnergyUse;
                }
            }
        }
    }
    if (intr (kAutoRegeneration))
        while (Clock - mLastRegen > 1500) {
            if (mHP < mMaxHP)  ++mHP;
            mLastRegen += 1500;
        }

    if (checkTimeOuts ())
        return -1;

    if (change)
        computeIntrinsics ();
    return 0;
}


void
shHero::feel (int x, int y, int force)
{
    if (!(force or cr ()->isAdjacent (x, y)))
        return;

    Level->feelSq (x, y);
}


/* try to move the hero one square in the given direction
   RETURNS: AP expended
 */

int
shCreature::doPlayerMove (shDirection dir)
{
    if (!isHero ())  return FULLTURN;
    int x = mX;
    int y = mY;
    int tx, ty;
    static int leftsq, rightsq; /* this static crap is not tasty -- CADV */
    shFeature *f;
    int speed;

    speed = isDiagonal (dir) ? DIAGTURN: FULLTURN;

    if (!Level->moveForward (dir, &x, &y)) {
        //I->p ("You can't move there!");
        return 0;
    }

    mDir = dir;

    /* mBusy != 0 --> hero is gliding. */
    if (Hero.mBusy)
        /* Do not approach hostile monsters.  Check 3x3 area hero
           is moving into for creatures known to be enemies.  Regard
           known beings of uncertain motives as hostile. */
        for (int dx = maxi(0,x-1); dx <= mini(x+1,MAPMAXCOLUMNS-1); ++dx)
            for (int dy = maxi(0,y-1); dy <= mini(y+1,MAPMAXROWS-1); ++dy) {
                shCreature *c = Level->getCreature (dx, dy);
                if (!c or c->isHero ())  continue;
                /* Do not notice burrowed mines, hidden aliens, etc. */
                if (c->mHidden > 0)  continue;
                bool hostile = false;
                if (canSee (c) or canHearThoughts (c)) {
                    hostile = c->isHostile (); /* Hero can discern. */
                } else if (isAwareOf (c)) {
                    hostile = true; /* Better be cautious. */
                }
                if (hostile) { /* Threat nearby.  Stop gliding. */
                    Hero.interrupt ();
                    return 0;
                }
            }

    if (kOrigin == dir) {
        /* No further checks needed for waiting in place. */
    } else if (Hero.mBusy == 1) {
        /* First step of long walk does not consider anything
           adjacent as reason to stop moving. */
        tx = x; ty = y;
        if (!Level->moveForward (leftTurn (dir), &tx, &ty)) {
            leftsq = 0;
        } else {
            leftsq = Level->appearsToBeFloor (tx, ty);
        }
        tx = x; ty = y;
        if (!Level->moveForward (rightTurn (dir), &tx, &ty)) {
            rightsq = 0;
        } else {
            rightsq = Level->appearsToBeFloor (tx, ty);
        }
        ++Hero.mBusy;
    } else if (Hero.mBusy) {
        if (is (kStunned)) {
            Hero.interrupt ();
            return 0;
        }
        Hero.feel (x, y);
        if (Level->isObstacle (x, y)) {
            Hero.interrupt ();
            return 0;
        }
        if ((f = Level->getFeature (x, y))) {
            if (!f->mTrapUnknown or f->isObstacle ()) {
                Hero.interrupt ();
                return 0;
            }
        }

        if (intr (kBlind)) {
            goto trymove;
        }

        /* Stop if passing by interesting features. */
        tx = x;  ty = y;
        if (!Level->moveForward (leftTurn (dir), &tx, &ty) or
            (leftsq != Level->appearsToBeFloor (tx, ty)) or
            (Level->countObjects (tx, ty) and !Level->isWatery (tx, ty)) or
            Level->getKnownFeature (tx, ty))
        {
            Hero.interrupt ();
        }
        tx = x;  ty = y;
        if (!Level->moveForward (rightTurn (dir), &tx, &ty) or
            (rightsq != Level->appearsToBeFloor (tx, ty)) or
            (Level->countObjects (tx, ty) and !Level->isWatery (tx, ty)) or
            Level->getKnownFeature (tx, ty))
        {
            Hero.interrupt ();
        }
    }
trymove:

    if (Level->isObstacle (x, y)) {
        int ismapped = Level->getMemory (x, y).mTerr != kMaxTerrainType;
        if (!interrupt ()) {
            if (is (kStunned) or is (kConfused) or
                (intr (kBlind) and /* Try to open remembered doors. */
                 Level->getMemory (x, y).mFeat != shFeature::kDoorClosed))
            {
                I->p ("You bump into %s!",
                    ismapped ? Level->the (x, y) : Level->an (x, y));
                Hero.feel (x, y);
                return speed;
            }
        }
        /* Auto open doors when you bump into them. */
        shFeature *f = Level->getFeature (x, y);
        if (f and f->isDoor ()) {
            openDoor (x, y);
            Hero.feel (x, y);
            return FULLTURN;
        }
    } else if (Level->isOccupied (x, y) and dir != kOrigin) {
        Hero.feel (x, y);
        if (!interrupt ()) {
            shCreature *c = Level->getCreature (x, y);
            if (is (kStunned) or is (kConfused) or
                (intr (kBlind) and !canHearThoughts (c) and !canSenseLife (c)))
            {
                I->p ("You bump into %s!", THE (c));
                if (!c->isHostile ()) {
                    if (c->isRobot ()) {
                        I->p ("It beeps.");
                        if (intr (kTranslation)) {
                            Level->remember (x, y, c->mIlkId);
                            I->refreshScreen ();
                            I->pauseXY (x, y, 1000, 0);
                        }
                    } else if (c->isA (kMonMelnorme)) {
                        if (tryToTranslate (c)) {
                            I->p ("\"Please be careful.\"");
                            Level->remember (x, y, c->mIlkId);
                            I->refreshScreen ();
                            I->pauseXY (x, y, 1000, 0);
                        } else {
                            I->p ("You hear annoyed mumbling.");
                        }
                    } else if (c->isA (kMonMonolith)) {
                        I->p ("It seems to be a solid block.");
                        Level->remember (x, y, c->mIlkId);
                    }
                }
                return speed;
            }
        }
    } else {
        /* Ask before entering square with a trap covered by items. */
        shFeature *f = Level->getFeature (x, y);
        if ((x != mX or y != mY) and /* This is not wait in place. */
            Level->getObjects (x, y) and /* Stuff is lying there. */
            f and !f->mTrapUnknown and /* There is a trap you know about. */
            ( /* List of bad stuff to prompt confirmation: */
             f->mType == shFeature::kAcidPit or
             f->mType == shFeature::kPit or
             f->mType == shFeature::kHole or
             f->mType == shFeature::kBrokenGrating or
             f->mType == shFeature::kTrapDoor or
             f->mType == shFeature::kRadTrap
            ))
        {
            if (!I->yn ("Confirm entering place with an obscured %s?",
                        f->getShortDescription ()))
                return 0; /* Cancelled. */
        }

        Level->moveCreature (this, x, y);

        /* If blind we assume moving character tries to feel what is in front
           of them.  This is not done if hero has automatic search because it
           reveals all neighboring squares. */
        if (intr (kBlind) and !intr (kAutoSearching) and
            !is (kConfused) and !is (kStunned))
        {
            tx = x;  ty = y;
            if (Level->moveForward (leftTurn (dir), &tx, &ty)) {
                Hero.feel (tx, ty);
            }
            tx = x;  ty = y;
            if (Level->moveForward (dir, &tx, &ty)) {
                Hero.feel (tx, ty);
            }
            tx = x;  ty = y;
            if (Level->moveForward (rightTurn (dir), &tx, &ty)) {
                Hero.feel (tx, ty);
            }
        }

        if (Hero.mBusy) {
            I->pauseXY (mX, mY, 10);
            if (kOrigin == dir) {
                ++Hero.mBusy;
                if (Hero.mBusy == 20+1)
                    Hero.interrupt (); /* Limit resting to 20 turns. */
            }
        }
        return speed;
    }

    return 0;
}

void
shCreature::takeRads (int amt)
{
    mRad += amt;
    if (mIlkId == kMonGhoul) {
        int heal = RNG (amt / 3, amt / 2) + 1;
        healing (heal, 0);
    }
}

/* called every 10 seconds */
void
shCreature::upkeep ()
{
    /* Temporary modifiers decay. */
    FOR_ALL_ABILITIES (a) {
        /* Psionics decays and regenerates twice as fast as other abilities. */
        if (a != abil::Psi and (Clock % 10000 == Clock % 20000))
            continue;

        int mod = mAbil.temp (a);
        if (mod) {
            int neg_sgn = mod > 0 ? -1 : +1;
            mAbil.temp_mod (a, neg_sgn);
        }
    }

    if (mImpregnation) {
        pregnancy ();
        if (mState == kDead)
            return;
    }

    if (!isHero ())  return;

    if (!is (kSickened) and (!is (kPlagued) or mHP < mMaxHP / 2)) {
        /* Recover hit points. */
        if (mHP < mMaxHP) {
            mHP += 1 + mCLevel / 3 + isOrc ();
            if (mHP > mMaxHP)  mHP = mMaxHP;
        }
    }
    /* Superglued tongue. */
    {
        int glue = Hero.getStoryFlag ("superglued tongue");
        if (glue > 0) {
            if (++glue > 100) {
                I->p ("The canister of super glue has finally fallen off!");
                Hero.resetStoryFlag ("superglued tongue");
            } else {
                Hero.setStoryFlag ("superglued tongue", glue);
            }
        }
    }

    /* Reduce police awareness of your software piracy crimes. */
    {
        int danger = Hero.getStoryFlag ("software piracy");
        if (danger > 0) {
            Hero.setStoryFlag ("software piracy", --danger);
        }
    }

    /* Check radiation. */
    int r = checkRadiation ();
    r and (r = maxi (0, r - getResistance (kRadiological)));
    mRad += r;

    /* Large amounts of radiation decay faster. */
    if (mRad > 300)  mRad -= RNG (1, 5);

    /* Radiation processor can work if no new radiation is incoming. */
    !r and (mRad -= intr (kRadiationProcessing));

    if (mRad < 0)  mRad = 0;

    if (mRad >= 400) {
        if (!mInnateIntrinsics.get (kLightSource)) {
            if (!intr (kBlind)) {
                if (mRad >= 500) {
                    I->p ("You glow so much you could put "
                          "a Christmas tree to shame.");
                } else {
                    I->p ("You start glowing!");
                }
            }
        }
        mInnateIntrinsics.set (kLightSource, mRad / 100);
        computeIntrinsics ();
    }
    if (mRad < 300 and mInnateIntrinsics.get (kLightSource)) {
        if (!intr (kBlind)) {
            I->p ("You no longer glow.");
        }
        mInnateIntrinsics.set (kLightSource, 0);
        computeIntrinsics ();
    }

    if (mRad >= 3000)  mRad = 3000;

    /* Every minute test for radiation sickness.  Ghouls are immune. */
    if (!isA (kMonGhoul) and (Clock / (10 * FULLTURN)) % 6 == 2) {
        int stage = Hero.getStoryFlag ("radsymptom");
        int tolerance = 150 + 50 * usesPower (kBGAwareness);
        if (stage >= 10 and mRad > 1000 and !RNG (100)) {
            /* TODO: Mutation. */
        }
        /* Give messages of straning health. */
        if (stage == 0 and mRad >= 75 and mRad <= tolerance and
            checkRadiation ())
        {
            switch (RNG (3)) {
            case 0:
                I->p ("You have a slight headache."); break;
            case 1:
                I->p ("You feel fatigued."); break;
            case 2:
                I->p ("You feel somewhat sick."); break;
            }
        /* This is too much for body to take. */
        } else if (mRad > tolerance) {
            if (mRad / 12 > RNG (20) + mAbil.Con () - 10) {
                /* Figure out how many Con points to lose. */
                int more_dmg = stage / 7 + mRad / 300;
                int dmg = RNG (1, 2) + more_dmg;
                if (usesPower (kBGAwareness))
                    dmg = maxi (1, dmg / 2);
                /* Do hurt. */
                if (stage <= 15 and sufferAbilityDamage (abil::Con, dmg)) {
                    I->p ("You are overcome by your illness.");
                    die (kKilled, "radiation sickness");
                    return;
                } else {
                    /* If hero's constitution is already damaged or very low
                       advance sickness to later stage. */
                    if (stage < 7 - mAbil.Con ()) {
                        stage = 7 - mAbil.Con ();
                    }
                    switch (stage) {
                    case 0:
                        I->p ("You feel tired."); break;
                    case 1:
                        I->p ("You feel feverish."); break;
                    case 2:
                        I->p ("You feel nauseated."); break;
                    case 3:
                        I->p ("You vomit!"); break;
                    case 4:
                        I->p ("Your gums are bleeding!"); break;
                    case 5:
                        I->p ("Your hair is falling out!"); break;
                    case 6:
                        I->p ("Your skin is melting!"); break;
                    case 16:
                        I->p ("You transform into a ghoul!");
                        mInnateResistances[kRadiological] += 5;
                        mIlkId = kMonGhoul;
                        mGlyph.mColor = myIlk ()->mGlyph.mColor;
                        mAbil.restore (abil::Con, dmg); /* Revert last attack. */
                        mAbil.perm_mod (abil::Con, -5);
                        if (mAbil.Con () <= 0) {
                            I->p ("You are too weak to survive in your current form.");
                            die (kMisc, "Rotted to nothing");
                        }
                        stage = -1;
                        break;
                    default:
                        I->p ("Your body is wracked with convulsions.");
                        inflict (kStunned, stage * FULLTURN);
                    }
                    I->nonlOnce ();
                    I->p ("  (-%d Con)", dmg);
                    ++stage;
                    Hero.setStoryFlag ("radsymptom", stage);
                }
            } else if (0 == stage) {
                I->p ("You have a headache.");
                Hero.setStoryFlag ("radsymptom", 1);
            } else if (RNG (2)) {
                I->p ("You feel weary.");
            } else {
                I->p ("You feel quite sick.");
            }
        } else {
            if (Hero.getStoryFlag ("radsymptom")) {
                I->p ("You're beginning to feel better.");
                Hero.setStoryFlag ("radsymptom", 0);
            }
        }
        if (!isA (kMonGhoul))
            mRad = maxi (mRad - 5, 0);  /* Slowly recover. */
    }
    if (isA (kMonGhoul)) {
        int minimum = 4 * mAbil.Con ();
        if (mRad > minimum)  mRad -= RNG (1, 10);
    }

    /* Buggy shock capacitors rarely may discharge into your brain. */
    if (4 == (Clock / 10000) % 6 and !RNG (25)) {
        for (int i = 0; i <= shObjectIlk::kCerebellum; ++i) {
            shObject *imp = mImplants[i];
            if (imp and imp->isA (kObjShockCapacitor) and imp->isBuggy () and
                imp->mCharges and RNG (2))
            {
                int discharge = RNG (1, imp->mCharges);
                int dmg = maxi (1, discharge / 5);
                imp->mCharges -= discharge;
                I->p ("%s malfunctions and dicharges into your brain!", YOUR (imp));
                /* Decrement health directly because sending electrical damage
                   through sufferDamage would get captured by the very same
                   shock capacitor that just discharged. */
                mHP -= discharge;
                if (mHP < 0)  mHP = 0;
                if (sufferAbilityDamage (abil::Int, RNG (1, dmg)) or !mHP) {
                    die (kMisc, "Fried his brain");
                    return;
                }
                imp->set (obj::known_type);
                imp->set (obj::known_charges);
                checkConcentration ();
                inflict (kConfused, FULLTURN * maxi (3, RNG (discharge)));
                Hero.interrupt ();
            }
        }
    }

    /* Ungooma may try to take over you, reproduce or mature. */
    int ungooma = hasInstalled (kObjUngooma);
    if (5 == (Clock / 10000) % 12 and ungooma) {
        int free = 0; /* Free brain lobes. */
        for (int i = 0; i <= shObjectIlk::kCerebellum; ++i)
            free += !mImplants[i];
        /* Takeover? */
        if (ungooma == 5 or (ungooma == 4 and mAbil.Int () < 10)) {
            I->p ("You are taken over by brain parasites.");
            die (kMiscNoYouDie, "Got taken over by Ungooma collective mind");
            return;
        }
        /* Maturing. */
        if (!RNG (25)) {
            /* More parasites means more chance for maturing. */
            int slot = RNG (shObjectIlk::kFrontalLobe, shObjectIlk::kCerebellum);
            shObject *parasite = mImplants[slot];
            if (parasite and parasite->isA (kObjUngooma)) {
                if (!parasite->isOptimized ()) {
                    ++parasite->mBugginess;
                    if (parasite->is (obj::known_type)) {
                        I->p ("%s matures.", YOUR (parasite));
                        parasite->set (obj::known_bugginess);
                        computeIntrinsics (true);
                    }
                }
            }
        }
        /* Reproduction. */
        if (!RNG (20) and free) {
            /* The less space there is the less chance of reproduction. */
            int slot = RNG (shObjectIlk::kFrontalLobe, shObjectIlk::kCerebellum);
            if (!mImplants[slot]) {
                shObject *parasite = new shObject (kObjUngooma);
                parasite->setBuggy (); /* Youngling saps a lot of resources. */
                if (!addObjectToInventory (parasite, true)) {
                    delete parasite;
                } else {
                    don (parasite, true);
                }
            }
        }
    }

    /* Narcolepsy induces random sleep attacks. */
    if (intr (kNarcolepsy) and !is (kAsleep) and !RNG (10)) {
        if (mResistances[kMesmerizing]) {
            I->p ("You feel drowsy.");
        } else {
            I->p ("You suddenly fall asleep!");
            inflict (kAsleep, FULLTURN * RNG (20, 100));
        }
    }

    /* A YASD with YAFM. */
    if (!isA (kMonGhoul) and (Clock / 10000) % 20 == 12 and mAbil.Int () == 0) {
        I->p ("Having lost your mind you wander %s for years to come.",
              Level->getDescription ());
        die (kMisc, "Lost his mind");
        return;
    }
}


void
shCreature::playerControl ()
{
    int dx;
    int dy;
    int glidemode = 0;
    int elapsed = 0;

    if (instantUpkeep ()) { /* hero died */
        return;
    }
    Level->computeVisibility (this);
    I->cursorOnHero ();
    I->drawScreen ();
    spotStuff (false);

    if (isHelpless ()) {
        /* lose a turn */
        mAP -= 1000;
        return;
    }

    if (intr (kAutoSearching)) {
        doSearch ();
        Level->drawSq (mX, mY);
    }

 getcmd:

    { /* Process Geiger counters. */
        int rl = checkRadiation ();
        int ol = Hero.getStoryFlag ("geiger counter message");

        /* React to radiation level changes. */
        int cnt = intr (kRadiationDetection);
        if (cnt and rl != ol) {
            for (int i = 0; i < mInventory->count (); ++i) {
                shObject *obj = mInventory->get (i);
                if (obj->isA (kObjGeigerCounter) and obj->is (obj::active) and
                    !obj->isBuggy () and !obj->is (obj::known_type))
                {
                    obj->set (obj::known_type);
                    obj->announce ();
                }
            }
            interrupt ();
            const char *form, *dowhat;
            if (rl == 0) {
                form = cnt > 1 ? "s have" : " has";
                dowhat = "stopped clicking";
            } else {
                form = cnt > 1 ? "s are" : " is";
                if (ol == 0) {
                    dowhat = "making a clicking noise";
                } else if (ol < rl) {
                    dowhat = "clicking more rapidly";
                } else {
                    dowhat = "clicking more slowly";
                }
            }
            I->p ("Your Geiger counter%s %s.", form, dowhat);
        }
        Hero.setStoryFlag ("geiger counter message", rl);
    }


    if (Hero.mBusy and !isTrapped () and !is (kStunned) and !is (kConfused)) {
        elapsed = doPlayerMove (mDir);
        if (elapsed) {
            mAP -= elapsed;
            if (mDir == kOrigin) { /* Search around while resting. */
                doSearch ();
            }
            return;
        }
    }

    Hero.mBusy = 0;
    elapsed = 0;
    shInterface::Command cmd = I->getCommand ();

    dx = 0;
    dy = 0;
    if (cmd >= shInterface::kMoveN and cmd <= shInterface::kMoveNW) {
        /* Stun totally randomizes movement every turn. */
        if (is (kStunned)) {
            cmd = (shInterface::Command) (shInterface::kMoveN + RNG (8));
        /* Confusion tilts to left or right every third turn or so. */
        } else if (is (kConfused) and !RNG (3)) {
            if (RNG (2)) {
                cmd = shInterface::Command (int (cmd) - 1);
            } else {
                cmd = shInterface::Command (int (cmd) + 1);
            }
            if (cmd < shInterface::kMoveN)  cmd = shInterface::kMoveNW;
            if (cmd > shInterface::kMoveNW)  cmd = shInterface::kMoveN;
        }
    }

    switch (cmd) {
    case shInterface::kNoCommand:
        I->p ("Unknown command.  Type '%s' for help.",
              I->getKeyForCommand (shInterface::kHelp));
        break;
    case shInterface::kHelp:
        I->showHelp ();
        break;
    case shInterface::kHistory:
        I->showHistory ();
        break;
    case shInterface::kRest:
        doSearch ();
        elapsed = doPlayerMove (kOrigin);
        break;
    case shInterface::kGlide:
    {
        if (is (kConfused) or is (kStunned) or isTrapped ()) {
            break;
        }
        int dz = 0;
        shDirection d;
        d = I->getDirection (&dx, &dy, &dz, 1);
        if (kNoDirection == d or dz) {
            break;
        }
        mDir = d;
        glidemode = 1;
        Hero.mBusy = 1;
        I->pageLog ();

        if (kOrigin == d) {
            /* Do not attempt trap escape.  Go straight to waiting instead. */
            elapsed = doPlayerMove (kOrigin);
            break;
        }
        goto domove;
    }
    case shInterface::kGlideN:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveN:
        --dy;  goto domove;
    case shInterface::kGlideNE:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveNE:
        ++dx; --dy; goto domove;
    case shInterface::kGlideE:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveE:
        ++dx; goto domove;
    case shInterface::kGlideSE:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveSE:
        ++dx; ++dy; goto domove;
    case shInterface::kGlideS:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveS:
        ++dy; goto domove;
    case shInterface::kGlideSW:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveSW:
        ++dy; --dx; goto domove;
    case shInterface::kGlideW:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveW:
        --dx; goto domove;
    case shInterface::kGlideNW:
        glidemode = 1; Hero.mBusy = 1; /* fall through */
    case shInterface::kMoveNW:
        --dx; --dy;

    domove:
        /* FIXME: Remove all logic pertraining when to untrap self from
            following block and place it first.  That should eliminate all
            gotos and make the code piece easier to read. */
        {
            int x = mX + dx;
            int y = mY + dy;
            shCreature *c;

            if (!Level->isInBounds (x, y))
                break;

            c = Level->getCreature (x, y);

            if (mTrapped.mWebbed or mTrapped.mTaped)  goto untrap;
            if (Level->rememberedCreature (x, y) or
                (c and (!intr (kBlind) or isAwareOf (c))))
            { /* this must be an attack */
                if (glidemode) {
                    glidemode = 0;
                    break;
                }

                if (c and c->mZ < 0 and mLevel->isObstacle (x, y)) {
                    I->p ("You can't attack there!");
                    break;
                }
                if (mZ < 0 and mLevel->isObstacle (mX, mY)) {
                    if (isTrapped ())
                        goto untrap;
                    else
                        break;
                }
                if (c and c->isPet () and !is (kStunned) and !is (kConfused)
                    and !c->feat (kSessile))
                {
                    if (isTrapped ()) {
                        goto untrap;
                    }
                    elapsed = Hero.displace (c);
                } else {
                    if (c and is (kFrightened) and !is (kConfused) and
                             !is (kStunned) and !intr (kBlind))
                    {
                        I->p ("You are too afraid to attack %s!", THE (c));
                        break;
                    }
                    if (c and !c->isHostile () and !c->isA (kMonMonolith) and
                        !intr (kBlind) and !is (kConfused) and !is (kStunned))
                    {
                        if (!I->yN ("Really attack %s?", THE (c))) {
                            I->nevermind ();
                            break;
                        }
                    }
                    if (c and c->isA (kMonFuelBarrel) and isAwareOf (c) and
                        mHP <= Attacks[kAttExplodingMonster].mDamage[0].mHigh
                        and !c->is (kFrozen) and
                        !Hero.getStoryFlag ("melee barrel"))
                    {   /* No point in killing players who like DoomRL very
                           much that way.  Expectations carry over. */
                        I->p ("Be warned that you cannot push barrels.");
                        I->p ("You need %d hit points to surely survive the explosion.",
                            Attacks[kAttExplodingMonster].mDamage[0].mHigh + 1);
                        I->p ("There will be no futher warnings.");
                        Hero.setStoryFlag ("melee barrel", 1);
                        I->pause ();
                        break;
                    }
                    elapsed = meleeAttack (mWeapon, I->moveToDirection (cmd));
                }
                if (!elapsed) {
                    break;
                }
            } else if (isTrapped ()) {
            untrap:
                elapsed = tryToFreeSelf ();
            } else {
                elapsed = doPlayerMove (vectorDirection (dx, dy));
            }
        }
        break;
    {
        shDirection dir;

    case shInterface::kFireWeapon:
    case shInterface::kFireN:
    case shInterface::kFireNE:
    case shInterface::kFireE:
    case shInterface::kFireSE:
    case shInterface::kFireS:
    case shInterface::kFireSW:
    case shInterface::kFireW:
    case shInterface::kFireNW:
//    case shInterface::kFireDown:
//    case shInterface::kFireUp:
        if (cmd == shInterface::kFireWeapon) {
            dir = kNoDirection;
        } else {
            dir = (shDirection) ((int) cmd - (int) shInterface::kFireN);
        }

        if (!mWeapon or
            !(mWeapon->myIlk ()->mGunAttack or
              mWeapon->isThrownWeapon () or mWeapon->isA (kRayGun) or
              (mWeapon->has_subtype (computer) and mCloak and
               mCloak->isA (kObjPlasmaCaster))))
        {
            I->p ("You're not wielding a ranged weapon.");
            break;
        }

        if (!HeroCanFire ())  break;

        if (kNoDirection == dir) {
            dir = I->getDirection ();
            if (kNoDirection == dir) {
                break;
            }
        }
        if (kOrigin == dir) {
            if (!I->yn ("Really harm yourself?  Please confirm.")) break;
        }
        Hero.resetStoryFlag ("strange weapon message");

        int dothrow = !mWeapon->myIlk ()->mGunAttack
                      and !mWeapon->isA (kRayGun)
                      and !mWeapon->has_subtype (computer);
        if (dothrow) {
            shObject *obj = removeOneObjectFromInventory (mWeapon);
            elapsed = throwObject (obj, mWeapon, dir);
        } else {
            elapsed = shootWeapon (mWeapon, dir);
        }
        break;
    }
    case shInterface::kThrow:
    {
        shObject *obj;
        shObjectVector v;

        if (!HeroCanThrow ())  break;

        selectObjectsByFunction (&v, mInventory, &shObject::isThrownWeapon);
        obj = quickPickItem (&v, "throw", shMenu::kFiltered |
                             shMenu::kCategorizeObjects);
        if (!obj)  break;
        elapsed = tryToThrowObject (obj);
        break;
    }
    case shInterface::kKick:
    {
        if (HeroIsWebbedOrTaped ())  break;
        shDirection dir;

        dir = I->getDirection ();
        if (kNoDirection == dir) {
            break;
        } else if (kOrigin == dir) {
            I->p ("Kicking yourself isn't going to make things any better.");
            break;
        }

        elapsed = kick (dir);
        break;
    }

    case shInterface::kZapRayGun:
    {
        if (!HeroCanZap ())  break;
        shObjectVector v;
        selectObjects (&v, mInventory, kRayGun);
        shObject *obj = quickPickItem (&v, "zap", 0);
        if (!obj)  break;
        shDirection dir = I->getDirection ();
        if (kNoDirection == dir)  break;
        elapsed = shootWeapon (obj, dir);
        break;
    }
    case shInterface::kMoveUp:
    {
        int x, y;
        shMapLevel *oldlevel = Level;
        shFeature *stairs = Level->getFeature (mX, mY);

        if (isTrapped ()) {
            elapsed = tryToFreeSelf ();
            break;
        }

        if (intr (kFlying) and mZ < 1) {
            ++mZ;
            if (mZ)  I->p ("You are now flying high above floor.");
            else     I->p ("You reach ground level.");
            elapsed = FULLTURN;
            break;
        }

        if (!stairs or shFeature::kStairsUp != stairs->mType) {
            if (intr (kFlying))
                I->p ("You cannot fly higher.");
            else
                I->p ("You can't go up here.");
            break;
        }
        Level->removeCreature (this);
        Level = Maze.get (stairs->mDest.mLevel);

        if (-1 == stairs->mDest.mX) {
            Level->findUnoccupiedSquare (&x, &y);
        } else {
            x = stairs->mDest.mX;
            y = stairs->mDest.mY;
            if (Level->isOccupied (x, y)) {
                Level->findNearbyUnoccupiedSquare (&x, &y);
            }
        }
        Level->putCreature (this, x, y);
        Hero.checkForFollowers (oldlevel, stairs->mX, stairs->mY);
        elapsed = FULLTURN;
        break;
    }
    case shInterface::kMoveDown:
    {
        int x, y;
        shMapLevel *oldlevel = Level;
        shFeature *stairs = Level->getFeature (mX, mY);

        if (-1 == mZ) {
            I->p ("You're already at the bottom.");
            break;
        }

        if (intr (kFlying) and mZ == 1) {
            I->p ("You gracefully descend.");
            mZ = 0;
            elapsed = HALFTURN;
            break;
        }

        if (!stairs or stairs->mTrapUnknown) {
            I->p ("You can't go down here.");
            break;
        }

        bool cancel = false;
        switch (stairs->mType) {
        case shFeature::kBrokenGrating:
        case shFeature::kHole:
        case shFeature::kTrapDoor:
            I->p ("You %s a %s!", intr (kFlying) ? "fly down" : "jump into",
                  stairs->getShortDescription ());
            break;
        case shFeature::kStairsDown:
            break;
        case shFeature::kPit:
            I->p ("You %s into the pit.", intr (kFlying) ? "fly down" : intr (kJumpy) ? "jump" : "climb down");
            mZ = -1;
            mTrapped.mInPit = NDX (1, 6);
            elapsed = FULLTURN;
            break;
        case shFeature::kAcidPit:
            I->p ("You %s into the pit.", intr (kFlying) ? "fly down" : intr (kJumpy) ? "jump" : "climb down");
            mZ = -1;
            mTrapped.mInPit = NDX (1, 6);
            if (!intr (kFlying)) {
                if (sufferDamage (kAttAcidPitBath)) {
                    die (kMisc, "Dissolved in acid");
                    return;
                }
                setTimeOut (TRAPPED, 1000, 0);
            }
            elapsed = FULLTURN;
            break;
        case shFeature::kSewagePit:
            I->p ("You dive into the sewage.");
            mZ = -1;
            mTrapped.mInPit = NDX (1, 6);
            if (!intr (kAirSupply) and !intr (kBreathless)) {
                I->p ("You're holding your breath!");
                mTrapped.mDrowning = mAbil.Con ();
            }
            setTimeOut (TRAPPED, 1000, 0);
            elapsed = FULLTURN;
            break;
        default:
            I->p ("You can't go down here.");
            cancel = true;
            break;
        }

        if (cancel or elapsed)
            break;

        /* o/w we are descending a level */
        Level->removeCreature (this);
        Level = Maze.get (stairs->mDest.mLevel);
        if (!Level) {
            Level = oldlevel->getLevelBelow ();
        }
        if (-1 == stairs->mDest.mX) {
            Level->findUnoccupiedSquare (&x, &y);
        } else {
            x = stairs->mDest.mX;
            y = stairs->mDest.mY;
            if (Level->isOccupied (x, y)) {
                Level->findNearbyUnoccupiedSquare (&x, &y);
            }
        }
        Level->putCreature (this, x, y);
        Hero.checkForFollowers (oldlevel, stairs->mX, stairs->mY);
        elapsed = FULLTURN;
        break;
    }
    case shInterface::kOpen:
    case shInterface::kClose:
    {
        if (HeroIsWebbedOrTaped ())  break;
        int dz;
        int x, y;
        shFeature *f;
        shCreature *c;

        if (kNoDirection == I->getDirection (&dx, &dy, &dz)) {
            break;
        }
        x = dx + mX;
        y = dy + mY;
        if (!Level->isInBounds (x, y)) {
            I->p ("There is nothing to %s there.",
                  shInterface::kOpen == cmd ? "open" : "close");
            break;
        }
        Hero.feel (x, y);
        f = Level->getFeature (x, y);
        c = Level->getCreature (x, y);
        if (shInterface::kOpen == cmd) {
            if (c and c->isPet ()) {
                shObject *bolt = NULL;
                for (int i = 0; i < c->mInventory->count (); ++i) {
                    shObject *obj = c->mInventory->get (i);
                    if (obj->isA (kObjRestrainingBolt)) {
                        bolt = obj;
                        break;
                    }
                }
                bool tookaction = false;
                if (bolt) {
                    if (I->yn ("Remove restraining bolt from %s?", THE (c))) {
                        c->mInventory->remove (bolt);
                        addObjectToInventory (bolt);
                        Hero.mPets.remove (c);
                        c->mTame = 0;
                        c->mTactic = shCreature::kNewEnemy;
                        c->mDisposition = shCreature::kHostile;
                        c->mStrategy = c->myIlk ()->mDefaultStrategy;
                        elapsed = FULLTURN;
                        tookaction = true;
                    }
                }
                if (c->isA (kMonSmartBomb) and !tookaction) {
                    if (I->yn ("Deactivate %s?", THE (c))) {
                        Hero.mPets.remove (c); /* Prevent sadness message. */
                        c->die (kMisc);
                        --c->myIlk ()->mKills; /* That was not a kill. */
                        elapsed = FULLTURN;
                        tookaction = true;
                    }
                }
                if (!tookaction)  break;
            } else if (f and shFeature::kDoorClosed == f->mType) {
                openDoor (x, y);
                Hero.feel (x, y, 1);
                elapsed = FULLTURN;
            } else if (f and shFeature::kDoorOpen == f->mType) {
                I->p ("It's already open.");
                break;
            } else if (f and shFeature::kPortableHole == f->mType) {
                int nx = x + dx, ny = y + dy, fail = 0;
                /* Probe for another hole. */
                while (Level->getSquare (nx, ny)->mTerr == kStone) {
                    nx += dx;
                    ny += dy;
                    if (!Level->isInBounds (nx, ny)) {
                        fail = 1;
                        break;
                    }
                }
                shFeature *end = NULL;
                if (!fail) {
                    end = Level->getFeature (nx, ny);
                    if (!end or end->mType != shFeature::kPortableHole) {
                        fail = 1;
                    }
                }
                Level->removeFeature (f);
                if (fail) {
                    I->p ("The portable hole breaks down.");
                } else { /* Dig a tunnel. */
                    Level->removeFeature (end);
                    Level->dig (x, y);
                    while (x != nx or y != ny) {
                        x += dx;
                        y += dy;
                        Level->dig (x, y);
                    }
                    I->p ("You open the portable hole.");
                }
                elapsed = FULLTURN;
            } else {
                I->p ("There is nothing there to open.");
                break;
            }
        } else {
            if (f and shFeature::kDoorOpen == f->mType) {
                if (0 != Level->countObjects (x, y)) {
                    I->p ("There's something obstructing it.");
                } else if (Level->isOccupied (x, y)) {
                    I->p ("There's a monster in the way!");
                } else if (0 == closeDoor (x, y)) {
                    I->p ("You fail to close the door.");
                }
                Hero.feel (x, y, 1);
                elapsed = HALFTURN; /* it takes less time to slam a door shut */
            } else if (f and shFeature::kDoorClosed == f->mType) {
                int res = closeDoor (x, y);
                if (res) {
                    Hero.feel (x, y, 1);
                    elapsed = FULLTURN;
                } else {
                    break;
                }
            } else {
                I->p ("There is nothing there to close.");
                break;
            }
        }
        break;
    }
    case shInterface::kPickup:
    case shInterface::kPickUpAll:
    {
        shObjectVector *v = Level->getObjects (mX, mY);
        shObject *obj;

        int objcnt = v ? v->count () : 0;
        if (0 == objcnt) {
            I->p ("There is nothing on the ground to pick up.");
            break;
        }
        if (intr (kFlying) and mZ == 1) {
            I->p ("You are flying high above the floor.");
            break;
        }
        if (cmd == shInterface::kPickUpAll) {
            int num = objcnt;
            objcnt = 0;
            for (int i = 0; i < num; ++i) {
                obj = v->get (0);
                if (!intr (kBlind))  obj->set (obj::known_appearance);
                /* HACK BEGIN: explained later. */
                v->remove (obj);
                if (!addObjectToInventory (obj)) {
                    /* HACK END: everything is explained in next comment. */
                    Level->putObject (obj, mX, mY);
                } else {
                    if (Level->isInShop (mX, mY)) {
                        pickedUpItem (obj);
                    }
                    ++objcnt;
                }
            }
            if (!v->count ()) {
                Level->setObjects (mX, mY, NULL);
                delete v;
            }
        } else if (1 == objcnt) {
            if (!intr (kBlind))
                v->get (0) -> set (obj::known_appearance);
            /* HACK: if the object is merged into an inventory object,
               the the floor will momentarily contain a deleted object
               which would be dereferenced during the screen redraw that
               occurs while printing the "you picked up an object"
               message.  So, we preemptively null out the floor objs: */
            Level->setObjects (mX, mY, NULL);
            if (1 == addObjectToInventory (v->get (0))) {
                if (Level->isInShop (mX, mY)) {
                    pickedUpItem (v->get (0));
                }
                delete v;
                v = NULL;
            }
            /* END HACK: Restore the floor. */
            Level->setObjects (mX, mY, v);
        } else {
            int cnt;
            shMenu *menu = I->newMenu ("Pick up what?",
                shMenu::kMultiPick | shMenu::kCountAllowed |
                shMenu::kCategorizeObjects);

            v->sort (&compareObjects);
            for (int i = 0; i < v->count (); ++i) {
                obj = v->get (i);
                if (!intr (kBlind)) obj->set (obj::known_appearance);
                int let = i % 52;
                menu->addPtrItem (let < 26 ? let + 'a' : let - 26 + 'A',
                                 obj->inv (), obj, obj->mCount);
            }
            objcnt = 0;
            while (menu->getPtrResult ((const void **) &obj, &cnt)) {
                assert (cnt);
                ++objcnt;
                if (!intr (kBlind))
                    obj->set (obj::known_appearance);
                if (cnt != obj->mCount) {
                    obj = obj->split (cnt);
                } else {
                    /* HACK: as above, need to pre-emptively remove the
                       object from the floor vector even though we might
                       not actually pick it up: */
                    v->remove (obj);
                }
                if (0 == addObjectToInventory (obj)) {
                    /* END HACK: put it back. */
                    Level->putObject (obj, mX, mY);
                } else {
                    if (Level->isInShop (mX, mY)) {
                        pickedUpItem (obj);
                    }
                }
            }
            delete menu;
            if (!v->count ()) {
                Level->setObjects (mX, mY, NULL);
                delete v;
            }
        }
        if (!objcnt) { /* Didn't pick anything up means no time taken. */
            break;
        }
        Hero.feel (mX, mY);
        elapsed = FULLTURN;
        break;
    }
    case shInterface::kListInventory:
    {
        reorganizeInventory ();
        elapsed = listInventory ();
        break;
    }
    case shInterface::kAdjust:
    {
        adjust (NULL);
        break;
    }
    case shInterface::kCharScreen:
    {
        I->showCharacter (this);
        break;
    }
    case shInterface::kDrop:
    {
        shObject *obj;
        int cnt = 1;
        if (0 == mInventory->count ()) {
            I->p ("You aren't carrying anything.");
            break;
        }
        reorganizeInventory ();
        obj = quickPickItem (mInventory, "drop",
                             shMenu::kCountAllowed |
                             shMenu::kCategorizeObjects, &cnt);
        if (!obj) break;
        obj = tryToDrop (obj, cnt);
        if (!obj) break;
        drop (obj);
        Hero.feel (mX, mY);
        elapsed = HALFTURN;
        break;
    }

    case shInterface::kDropMany:
    {
        shObject *obj;
        int cnt;
        int ndropped = 0;

        reorganizeInventory ();

        if (0 == mInventory->count ()) {
            I->p ("You aren't carrying anything.");
            goto donedropping;
        } else {
            shMenu *menu = I->newMenu ("Drop what?",
                shMenu::kMultiPick | shMenu::kCountAllowed |
                shMenu::kCategorizeObjects);
            for (int i = 0; i < mInventory->count (); ++i) {
                obj = mInventory->get (i);
                menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
            }

            while (menu->getPtrResult ((const void **) &obj, &cnt)) {
                assert (cnt);
                if (0 == cnt) {
                    I->p ("Error popcorn delta: Menu fault.");
                    break;
                }
                if (obj == mWeapon) {
                    if (obj->isWeldedWeapon () and !isOrc ()) {
                        I->p ("Your weapon is welded to your hands.");
                        obj->set (obj::known_bugginess);
                        continue;
                    }
                    if (0 == unwield (obj)) {
                        continue;
                    }
                }
                if (obj->is (obj::worn)) {
                    I->p ("You can't drop %s because you're wearing it.",
                          YOUR (obj));
                    continue;
                }
                obj = removeSomeObjectsFromInventory (obj, cnt);
                drop (obj);
                ++ndropped;
            }
            delete menu;
        }
    donedropping:
        if (!ndropped) { /* didn't drop anything, no time penalty */
            break;
        } else if (1 == ndropped) {
            elapsed = HALFTURN;
        } else {
            elapsed = FULLTURN;
        }
        Hero.feel (mX, mY);
        break;
    }

    case shInterface::kLook:
    {
        char *buf = GetBuf ();
        snprintf (buf, SHBUFLEN,
            "Look at what (select a location, '%s' for help)",
            I->getKeyForCommand (shInterface::kHelp));
        int x = -1, y = -1;
        while (I->getSquare (buf, &x, &y, -1, 1))
        {
            shCreature *c = Level->getCreature (x, y);
            shFeature *f = Level->getFeature (x, y);
            int seen = 0;
            I->pageLog ();

            //FIXME: should remember things not in LOS

            if (!canSee (x, y) and Level->rememberedCreature (x, y)) {
                I->p ("You remember an unseen monster there.");
                seen++;
            } else if (c) {
                const char *pcf = "";
                const char *desc = NULL;
                if (c->isHostile ()) {
                    desc = AN (c);
                } else {
                    pcf = c->isPet () ? "a restrained " : "a peaceful ";
                    desc = c->myIlk ()->mName;
                }
                if (canSee (c) and c->mHidden <= 0) {
                    const char *wielding = "";
                    const char *weapon = "";
                    char health[13] = "";
                    char shield[18] = "";
                    if (c->mWeapon) {
                        wielding = ", wielding ";
                        weapon = AN (c->mWeapon);
                    }
                    if (mGoggles and mGoggles->isA (kObjScouterGoggles)) {
                        int HP;
                        if (mGoggles->isBuggy ()) {
                            mGoggles->set (obj::known_bugginess);
                            HP = RNG (9001, 9999); /* Over nine thousand. :-P */
                        } else {
                            HP = c->mHP;
                        }
                        snprintf (health, 13, " (HP: %d)", HP);
                        if (c->intr (kShielded)) {
                            if (mGoggles->mBugginess) {
                                sprintf (shield, " (shields: %d)",
                                    mGoggles->isOptimized () ?
                                    c->countEnergy () : RNG (9001, 9999));
                            } else {
                                sprintf (shield, " (shielded)");
                            }
                        }
                        if (!mGoggles->is (obj::known_type)) {
                            I->p ("You notice a weird number in a corner of your goggles.");
                            mGoggles->set (obj::known_type);
                        }
                    }
                    I->p ("You see %s%s%s%s.%s%s", pcf, desc, wielding, weapon, health, shield);
                    if (mGlyph.mSym == 'g') {
                        if (c->mResistances[kViolating]) {
                            I->p ("It is not even worth probing this one.");
                        } else if (c->mConditions & kNoExp) {
                            I->p ("You have already performed abduction of this one.");
                        }
                    }
                    seen++;
                } else if (canHearThoughts (c)) {
                    I->p ("You sense %s%s.", pcf, desc);
                    seen++;
                } else if (canSenseLife (c)) {
                    I->p ("You feel the presence of a lifeform here.");
                    seen++;
                } else if (canFeelSteps (c)) {
                    I->p ("You feel something moving here.");
                    seen++;
                } else if (canTrackMotion (c)) {
                    I->p ("You see a blip on your motion tracker.");
                    seen++;
                } else if (canSee(c) and c->mHidden > 0) {
                    switch (c->mMimic) {
                    case shCreature::kMmObject:
                        I->p ("You see %s.", c->mMimickedObject->mVague.mName);
                        seen++;
                        break;
                    default:
                        /* nothing seen */
                        break;
                    }
                }
            }
            int mdrange = intr (kMotionDetection);
            if (mdrange and distance (this, x, y) < mdrange * 5 and
                !intr (kBlind) and f and f->isDoor () and
                f->mDoor.mMoveTime + SLOWTURN >= Clock)
            { /* Automatic doors also show up on motion tracker. */
                I->p ("You see a blip on your motion tracker.");
                seen++;
            }
            if (!canSee (x, y)) {
                if (intr (kBlind)) {
                    I->p ("You are blind and cannot see.");
                } else if (!seen) {
                    I->p ("You can't see that location from here.");
                }
                continue;
            }

            if (Level->countObjects (x, y) and !Level->isWatery (x, y)) {
                shObjectVector *objs = Level->getObjects (x, y);
                shObject *bestobj = findBestObject (objs);
                I->p ("You see %s.", AN (bestobj));
                int cnt = objs->count ();
                if (cnt > 1) {
                    I->p ("You see %d more object%s underneath.",
                          cnt-1, cnt > 2 ? "s" : "");
                }
                seen++;
            }
            if (f and !(f->isTrap () and f->mTrapUnknown)) {
                I->p ("You see a %s.", f->getDescription ());
            } else {
                if (Level->isFloor (x, y)) {
                    if (kBrokenLightAbove == Level->getSquare(x,y)->mTerr) {
                        I->p ("You see a broken light under the ceiling.");
                    } else if (kSewage == Level->getSquare(x,y)->mTerr) {
                        I->p ("You see a pool of sewage.");
                    } else if (!seen) {
                        I->p ("You see the floor.");
                    }
                } else if (kVoid == Level->getSquare (x, y) ->mTerr) {
                    I->p ("You see a yawning void.");
                } else {
                    I->p ("You see a wall.");
                }
            }
        }
        I->p ("Done.");
        break;
    }

    case shInterface::kPay:
    {
        payShopkeeper ();
        break;
    }

    case shInterface::kQuaff:
    {
        shObject *obj;
        shObjectVector v;

        if (!HeroCanDrink ())  break;

        shFeature *f = Level->getFeature (mX, mY);
        if (f and shFeature::kVat == f->mType and mZ < 1 and
            I->yn ("Drink from the vat?"))
        {
            quaffFromVat (f);
            elapsed = FULLTURN;
        } else if (f and shFeature::kAcidPit == f->mType and mZ == -1 and
            I->yn ("Drink from the acid pit?"))
        {
            quaffFromAcidPit ();
            elapsed = FULLTURN;
        } else {
            selectObjectsByFunction (&v, mInventory, &shObject::canBeDrunk);
            obj = quickPickItem (&v, "quaff", 0);
            if (!obj) break;
            elapsed = doQuaff (obj);
        }
        break;
    }
    case shInterface::kSwap:
    {
        shObject *swap = NULL;
        for (int i = 0; i < mInventory->count (); ++i) {
            if (mInventory->get (i)->is (obj::prepared)) {
                swap = mInventory->get (i);
                break;
            }
        }
        if (mWeapon and mWeapon->isWeldedWeapon () and !isOrc ()) {
            if (mWeapon->is (obj::known_bugginess)) {
                I->p ("You cannot switch away from buggy weapon.");
                break;
            }
            I->p ("You can't let go of %s!  It must be buggy.", YOUR (mWeapon));
            mWeapon->set (obj::known_bugginess);
            elapsed = HALFTURN;
            break;
        }
        elapsed = HALFTURN;
        shObject *weap = mWeapon;
        if (mWeapon and !unwield (mWeapon)) {
            I->p ("Somehow, you can't let go of %s.", YOUR (mWeapon));
            break;
        }
        /* You can have only one item prepared. */
        for (int i = 0; i < mInventory->count (); ++i) {
            mInventory->get (i)->clear (obj::prepared);
        }
        if (weap)  {
            weap->set (obj::prepared);
            weap->announce ();
        }
        if (swap) {
            swap->clear (obj::prepared);
            wield (swap, 1);
            swap->announce ();
        } else {
            I->p ("You are now unarmed.");
        }
        break;
    }
    case shInterface::kUse:
    {
        if (HeroIsWebbedOrTaped ())  break;
        shObjectVector v;

        selectObjectsByFunction (&v, mInventory, &shObject::isUseable);
        shObject *obj = quickPickItem (&v, "use", shMenu::kCategorizeObjects);
        if (!obj) {
            break;
        }
        elapsed = obj->myIlk ()->mUseFunc (this, obj);
        break;
    }
    case shInterface::kDiscoveries:
    {
        listDiscoveries ();
        break;
    }
    case shInterface::kEditSkills:
    {
        editSkills ();
        break;
    }
    case shInterface::kExamine:
    {
        shObject *obj = quickPickItem (mInventory, "examine",
            shMenu::kCategorizeObjects);
        if (obj)
            obj->showLore ();
        break;
    }
    case shInterface::kExecute:
    {
        shObject *computer = chooseComputer ();
        if (!computer or !HeroCanExecute (computer))
            break;
        elapsed = computer->myIlk ()->mUseFunc (this, computer);
        break;
    }
    case shInterface::kWield:
    {
        if (HeroIsWebbedOrTaped ())
            break;
        shObject *obj;
        shObjectVector v1, v2;

        selectObjectsByFunction (&v1, mInventory, &shObject::isKnownWeapon);
        deselectObjectsByFlag (&v2, &v1, obj::wielded);
        v1.reset ();
        deselectObjectsByFlag (&v1, &v2, obj::worn);
        obj = quickPickItem (&v1, "wield", shMenu::kFiltered |
                                           shMenu::kCategorizeObjects);
        if (!obj or !wield (obj)) {
            break;
        }
        elapsed = HALFTURN;
        break;
    }
    case shInterface::kWear:
    {   /* One can wear armor and install bionic implants. */
        if (HeroIsWebbedOrTaped ())  break;
        shObjectVector v1, v2;
        selectObjectsByFunction (&v1, mInventory, &shObject::canBeWorn);
        deselectObjectsByFlag (&v2, &v1, obj::worn);
        v1.reset ();
        deselectObjectsByFlag (&v1, &v2, obj::designated);
        /* Pick something. */
        shObject *obj = quickPickItem (&v1, "wear", shMenu::kCategorizeObjects);
        if (!obj) break;
        elapsed = doWear (obj);
        break;
    }
    case shInterface::kTakeOff:
    {   /* Collect worn, installed and wielded stuff. */
        shObjectVector v;
        selectObjectsByFunction (&v, mInventory, &shObject::isEquipped);
        /* Choose item to unequip. */
        shObject *obj = quickPickItem (&v, "remove", shMenu::kCategorizeObjects);
        if (!obj) break;
        elapsed = doTakeOff (obj);
        break;
    }
    case shInterface::kMutantPower:
    {
        elapsed = useMutantPower ();
        break;
    }
    case shInterface::kName:
    {
        nameObject (NULL); /* This will prompt player to choose one. */
        break;
    }
    case shInterface::kToggleAutopickup:
        Flags.mAutopickup = !Flags.mAutopickup;
        I->p ("Autopickup is now %s.", Flags.mAutopickup ? "ON" : "OFF");
        break;
    case shInterface::kMainMenu:
    {
        shMenu *menu = I->newMenu ("Game menu:", shMenu::kNoHelp);
        menu->addIntItem ('s', "Save and quit", 1);
        menu->addIntItem ('o', "Set options", 2);
        menu->addIntItem ('v', "See game version", 3);
        menu->addIntItem ('h', "See high score table", 4);
        menu->addIntItem ('Q', "Quit without saving", 5);
        int res;
        menu->getIntResult (&res);
        delete menu;
        switch (res) {
        case 1:
            if (0 == saveGame ()) {
                I->p ("Game saved.  Press enter.");
                GameOver = 1;
                I->pause ();
                return;
            } else {
                I->p ("Save game operation failed!");
            }
            break;
        case 2:
            I->editOptions (); break;
        case 3:
            I->showVersion (); break;
        case 4:
            I->showHighScores (); break;
        case 5:
            if (I->yn ("Really quit?")) {
                die (kQuitGame, "quit");
                return;
            } else {
                I->nevermind ();
            }
            break;
        }
       break;
    }

    /* debugging commands */
    case shInterface::kBOFHPower:
        if (!BOFH) {
            I->p ("You have to be Bastard Operator From Hell to use this.");
            break;
        }
        Hero.useBOFHPower ();
        /* Experimental BOFH power can now transfer control. */
        if (!isHero ())  return monsterControl ();
        break;

    default:
//      I->p ("Impossible command.  The game is corrupt!");
        break;
    }

    if (0 == elapsed and !GameOver) {
        goto getcmd;
    } else if (elapsed > 0) {
        mAP -= elapsed;
    }

    return;
}

void
shHero::torcCheck ()
{
    if (!getStoryFlag ("worn torc")) {
        setStoryFlag ("worn torc", 1);
        if ((!RNG (10) or (mProfession == Psion and !RNG (3)))
            and cr ()->getMutantPower ())
        {
            I->p ("The torc seems to have unlocked your latent powers.");
        } else {
            I->p ("A thrill passes down your spine.");
        }
    }
}

bool
shHero::isPsyker ()
{   /* Stupid skill check.  It only works because all psykers have
       access four in metapsychic faculty skills. */
    shSkill *s = cr ()->getSkill (kMFRedaction);
    if (s and s->mAccess == 4)
        return true;
    return false;
}

void
shHero::amputateTail (void)
{   /* This code fragment relies on the tail attack being number 4. */
    Hero.cr ()->myIlk ()->mAttackSum -= Hero.cr ()->myIlk ()->mAttacks[3].mProb;
    Hero.cr ()->myIlk ()->mAttacks[3].mProb = 0;
    Hero.cr ()->myIlk ()->mAttacks[3].mAttId = kAttDummy;
}

void
shHero::touchMonolith (shCreature *monl)
{
    I->p ("You touch the monolith.");
    I->p ("You feel more experienced!");
    cr ()->beatChallenge (-1);
    I->p ("The monolith mysteriously vanishes!");
    monl->die (kSuicide);
}

void
shHero::death (shCauseOfDeath how, shCreature *killer, shObject *,
    shAttack *attack, const char *killstr)
{
    bool won = false;
    shCreature *hero = cr ();

    char newreason[100];
    if (killer and how != kMisc and how != kMiscNoYouDie) {
        hero->resetBlind (); /* Yes, do it for Xel'Naga too. */
        Level->setLit (killer->mX, killer->mY, 1, 1, 1, 1);
        if (hero == killer) {
            how = kKilled;
            killer = NULL;
            strcpy (newreason, hero->her ("own weapon"));
        } else {
            strcpy (newreason, AN (killer));
        }
        killstr = newreason;
    }

    Level->computeVisibility (mCreature);
    I->drawScreen ();
    switch (how) {
    case kSlain:
    case kKilled:
    case kMisc:
    case kSuicide:
        if (attack == &Attacks[kAttACMESign]) {
            I->p ("You now resemble Road Runner-burger.");
        } else {
            I->p ("You die.");
        }
        break;
    case kMiscNoYouDie:
        break; /* It is assumed caller has printed apt message. */
    case kDrowned:
        I->p ("You drown."); break;
    case kAnnihilated:
        I->p ("You are annihilated."); break;
    case kSuddenDecompression:
        switch (RNG (3)) { /* I just couldn't decide on one message. -- CADV */
        case 0:
            I->p ("Your lungs explode painfully."); break;
        case 1:
            I->p ("Your blood boils, enveloping your body in a crimson mist.");
            break;
        case 2:
            I->p ("Your internal organs rupture and your eyeballs pop."); break;
        }
        break;
    case kQuitGame:
        break;
    case kWonGame:
        won = true;
        I->p ("Congratulations, you are the baddest motherfucker "
              "in the galaxy now!");
        break;
    }

    if (!won and SpaceMarine == mProfession) { /* YAFM */
        I->p ("Game over, man!  Game over!");
    }
    /* Do it now, before DYWYPI spoils the beautiful screen to be captured. */
    mCreature->postMortem ();
    char message[200];
    epitaph (message, 200, how, killstr, killer);
    logGame (message);

    I->pause ();

    Level->clearSpecialEffects ();

    shMenu *query = I->newMenu ("Do you want ...", 0);
    query->addIntItem ('i', "... your possessions identified?", 1, 1);
    query->addIntItem ('m', "... to see complete map of current area?", 2, 1);
    query->addIntItem ('d', "... your diagnostics revealed?", 3, 1);
    query->addIntItem ('h', "... to see the console message history?", 4, 1);
    query->addIntItem ('k', "... to read the kill list?", 5, 1);
    query->addIntItem ('s', "... to view the high score table?", 6, 1);

    GameOver = 1;
    int r;
    while (query->getIntResult (&r)) {
        switch (r) {
        case 1:
            for (int i = 0; i < hero->mInventory->count (); ++i) {
                hero->mInventory->get (i) -> identify ();
            }
            hero->listInventory ();
            break;
        case 2:
            for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
                for (int y = 0; y < MAPMAXROWS; ++y) {
                    shFeature *f = Level->getFeature (x, y);
                    if (f)  f->mTrapUnknown = 0;
                    Level->mVisibility[x][y] = 1;
                    Level->setLit (x, y);
                }
            }
            I->drawScreen ();
            I->pause ();
            break;
        case 3:
            hero->doDiagnostics (0, won);
            break;
        case 4:
            I->showHistory ();
            break;
        case 5:
            I->showKills ();
            break;
        case 6:
            I->showHighScores ();
            break;
        }
        query->dropResults ();
    }
    delete query;

    //if (!won and kQuitGame != how)  tomb (message);

    I->p ("Goodbye...");

    if (!won)  debug.log ("Hero is slain!");
}

shCreature *
shHero::cr ()
{
    return mCreature;
}

//REQUIRES: monster types initialized already
void
shHero::init (const char *name, shProfession *profession)
{
    int x = 1, y = 1;
    mCreature = new shCreature ();
    shCreature *hero = mCreature;

    strncpy (hero->mName, name, HERO_NAME_LENGTH);
    hero->mName[HERO_NAME_LENGTH] = 0;
    hero->mLevel = Level;
    hero->mCLevel = 1;
    hero->mState = kActing;
    hero->mPsionicStormFatigue = 0;

    profession->mInitFunction (cr ());
    profession->postInit (cr ());
    memcpy (hero->mInnateResistances, hero->myIlk ()->mInnateResistances,
            sizeof (hero->mInnateResistances));
    for (int i = 0; i < MAXINTR; ++i)
        if (hero->myIlk ()->mIntrinsics[i] != kNoIntrinsic)
            hero->mInnateIntrinsics.set (hero->myIlk ()->mIntrinsics[i], true);

    /* These are duplicates of instructions from Monster instance creation. */
    hero->mInnateResistances[kMagnetic] = 122;
    /* Xel'Naga are kAlien. */
    if (hero->myIlk ()->mType == kAlien) {
        hero->mInnateIntrinsics.set (kSenseLife, 5);
        hero->mInnateIntrinsics.set (kAcidBlood, true);
    }

    hero->mAP = -1000;
    hero->mHP = hero->mMaxHP;

    hero->computeIntrinsics (true);
    hero->computeAC ();

    mScore = 0;
    hero->mSkillPoints = 0;
    hero->mXP = 0;

    do {
        Level->findUnoccupiedSquare (&x, &y);
    } while (Level->isInShop (x, y) or !Level->isInRoom (x, y));
    Level->putCreature (hero, x, y);
}

/* LOOK!  Here it is. */
shHero Hero;
