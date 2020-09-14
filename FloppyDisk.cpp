#include <ctype.h>
#include <stddef.h>
#include "Global.h"
#include "AttackType.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Transport.h"
#include "Implant.h"

static shCompInterface
checkInterface (shObject *computer)
{
    switch (computer->mIlkId) {
    case kObjMiniComputer:    return kIVisualVoice;
    case kObjMegaComputer:    return kIVisualVoice;
    case kObjSatCom:          return kIVisualOnly;
    case kObjBioComputer:     return kIUniversal;
    default:                  return kNotAComputer;
    }
}

static void
confusedComputingMessage (void)
{
    if (Hero.cr ()->intr (kBlind))
        if (Hero.isMute ())
            I->p ("You misexpress some of the commands in your confusion.");
        else
            I->p ("You mispronounce some of the commands in your confusion.");
    else
        I->p ("You make a few typos in your confusion.");
}

/* Checks whether hero can read map or object/monster locations printed
   on screen. Alternatively nerve interface can be employed in which
   case the image forms directly in the hero's brain.

returns:
   0: hero cannot get displayed information with specified interface
   1: hero managed to receive the information
*/
static int
visualDisplaySuccesful (shCompInterface interface) {
    if (!Hero.cr ()->intr (kBlind)) {
        return 1;   /* Silent, this is normal situation. */
    } else if (interface != kIUniversal) {
        I->p ("But you can't see the display!");
        return 0;
    } else {
        I->p ("You receive information through your bio computer's hypodermic nerve interface.");
        return 1;
    }
}

static void
criticalError (shObject *disk)
{
    I->p ("Critical error %d.  Execution halted.", RNG (1, 500));
    disk->set (obj::known_infected);
}

/* All that universally influences programming gets put here.
   If knownOnly is true only modifiers known to the hero are counted.
*/
static int
hackingMods (shObject *computer, bool estimate)
{   /* Hacker's ninja skillz. */
    int sk = Hero.cr ()->getSkillModifier (kHacking);
    /* Drunk coding is never good. */
    int conf = Hero.cr ()->is (kConfused) ? -5 : 0;
    /* TODO: move this to general kHacking boost. */
    /* It is said that programmers convert coffee to programs. :-) */
    int coffee = Hero.cr ()->getTimeOut (CAFFEINE) ? +2 : 0;

    int os = 0, virus = 0;
    /* Bonus from tools bundled with OS. */
    if (!estimate or computer->is (obj::known_enhancement))
        os = computer->hackingModifier ();
    /* Infected computer devotes some resources to resident virus. */
    if (!estimate or computer->is (obj::known_infected))
        virus = computer->is (obj::infected) ? -2 : 0;
    return sk + conf + coffee + os + virus;
}

int /* Not static, matter compiler also uses it. */
hackingRoll (shObject *computer)
{
    return RNG (1, 20) + hackingMods (computer, false);
}

static int
programDifficulty (shObjId id, shObject *comp, shObject *disk, bool estimate)
{
    if (id == kObjCorruptDisk)  return 100;
    shObjectIlk *ilk = &AllIlks[id];
    int score = hackingMods (comp, estimate);
    int req = 15 + ilk->mCost / 20;

    if (!estimate or disk->is (obj::known_bugginess)) {
        if (disk->isOptimized ()) {
            score += 2;
        } else if (disk->isBuggy ()) {
            score -= 4;
        }
    }

    if (ilk->mFlags & kIdentified) {
        score += 4;
    } else if (SoftwareEngineer != Hero.mProfession) {
        score -= 4;
    }

    int chance = (score - req + 20) * 5;
    if (chance < 0)  chance = 0;
    if (chance > 100)  chance = 100;
    if (id == kObjMatterCompiler)  chance /= 2; /* Very hard to write. */
    return chance;
}
/* Attempt to write a new program on a blank floppy disk. */
static shExecResult
doBlank (shObject *computer, shObject *disk)
{
    const char *prompt = "What program do you want to write on it?";
    char *buf = GetBuf ();
    if (!disk->is (obj::known_type)) {
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        I->p ("This disk is blank!");
        I->pause ();
    }

    shObjId progid = kObjNothing; /* Chosen program type. */
    int chance = -1; /* Undetermined yet. */

    /* You don't get to pick a program. */
    if (Hero.cr ()->is (kConfused)) {
        I->p ("You type random things.");
        if (RNG (2)) {
            int confilk;
            shObject *tmp;
            do {
                tmp = gen_obj_type (prob::FloppyDisk);
                confilk = tmp->mIlkId;
            } while (confilk == kObjBlankDisk or confilk == kObjCorruptDisk);
            delete tmp;
            progid = shObjId (confilk);
            chance = 100;
        } else {
            progid = kObjCorruptDisk;
            chance = 50; /* May just lose the disk. */
        }
    } else { /* Present some choice to player. */
        shMenu *program = I->newMenu (prompt, 0);
        program->attachHelp ("writing.txt");

        program->addHeader ("       Name                Chance");
        char let = 'a';
        int i = kObjCorruptDisk;
        do
        {
            shObjectIlk *ilk = &AllIlks[i++];

            if (!(ilk->mFlags & kIdentified) and
                Hero.mProfession != SoftwareEngineer)
            {
                continue;
            }

            const char *strptr = strstr (ilk->mReal.mName, " of ");
            if (strptr) {
                strptr += 4; /* Skip " of " and go right to program name. */
                int dc = programDifficulty (shObjId (i), computer, disk, true);
                sprintf (buf, "%-20s %3d%%", strptr, dc);
                program->addIntItem (let++, buf, i);
                if (let == 'z' + 1)  let = 'A';
            }
        } while (AllIlks[i].mReal.mType == kFloppyDisk);
        program->addIntItem (let, "other (specify)", -1);

        int res;
        if (!program->getIntResult (&res)) {
            return kNoExpiry; /* Cancelling writing a blank disk is free. */
        }
        progid = shObjId (res);
    }

    if (progid == -1) { /* Manually specify program name. */
        extern shObjId consumeIlk (const char *);
        char *buf2 = GetBuf ();
        I->getStr (buf, SHBUFLEN, prompt);

        /* Already good? */
        progid = consumeIlk (buf2);
        if (!progid) {
            snprintf (buf2, SHBUFLEN, "floppy disk of %s", buf);
            progid = consumeIlk (buf2);
        }
        if (!progid) {
            snprintf (buf2, SHBUFLEN, "%s floppy disk", buf);
            progid = consumeIlk (buf2);
        }

        if (progid == kObjNothing) {
            I->p ("There is no such program!");
            /* Blow up in 25% of the time to prevent abuse by guessing.
               Players should either get spoiled or be unable to bore
               themselves. */
            return RNG (4) ? kNoExpiry : kDestruct;
        } else if (progid == kObjCorruptDisk) {
            I->p ("You happily mash the keyboard.  Success!");
            chance = 100;
        } else if (progid == kObjBlankDisk) {
            I->p ("Don't be ridiculous!");
            return kNoExpiry;
        }
    }

    if (chance == -1)  /* Check hero's skill. */
        chance = programDifficulty (progid, computer, disk, false);

    if (RNG (100) < chance) { /* Hurrah! */
        disk = Hero.cr ()->removeOneObjectFromInventory (disk);
        disk->mIlkId = progid;
        if (!disk->isIlkKnown ())  /* Writing new programs. */
            Hero.cr ()->earnXP (disk->myIlk ()->mCost);

        disk->setIlkKnown (); /* You just wrote it. */
        disk->myIlk ()->mFlags |= kIdentified;
        if (computer->is (obj::known_infected))
            disk->set (obj::known_infected);
    } else {
        I->p ("Your hacking attempt was a failure.");
        switch (RNG (20)) { /* Slim chance for not blowing up the disk. */
            case 0:
                disk = Hero.cr ()->removeOneObjectFromInventory (disk);
                disk->mIlkId = kObjCorruptDisk;
                break;
            case 1:
                if (!disk->isBuggy ()) {
                    disk = Hero.cr ()->removeOneObjectFromInventory (disk);
                    disk->setBuggy ();
                    break;
                }
            default:
            return kDestruct;
        }
    }

    /* Deliver to inventory. */
    if (!Hero.cr ()->addObjectToInventory (disk)) {
        I->p ("The disk slips from your ", Hero.mProfession == XelNaga ? "claws!" : "hands!");
        Level->putObject (disk, Hero.cr ()->mX, Hero.cr ()->mY);
    }

    /* This applies regardless whether you succeed or not. */
    if (progid == kObjOperatingSystem and !Hero.cr ()->is (kConfused)) {
        I->p ("That was a very exhausting task.");
        Hero.cr ()->sufferDamage (kAttHypnoDiskO);
    }
    return kNoExpiry;
}

static void
identify_single_item (shObject *obj, bool infected)
{
    unsigned int flags = 0;
    if (!infected or RNG (3))
        flags |= obj::known_appearance;

    if (!infected or RNG (3))
        flags |= obj::known_bugginess;

    if (!infected or RNG (3))
        flags |= obj::known_charges;

    if (!infected or RNG (3))
        flags |= obj::known_cracked;

    if (!infected or RNG (3))
        flags |= obj::known_enhancement;

    if (!infected or RNG (3))
        flags |= obj::known_infected;

    if (!infected or RNG (3))
        flags |= obj::known_fooproof;

    if (!infected or RNG (3))
        flags |= obj::known_type;

    obj->set (flags);
}

void
identifyObjects (int howmany, int infected)
{
    shObject *obj;
    int didsomething = 0;
    const char *prompt = "What do you want to identify?";
    const char *after = "You have identified the following items:";
    shMenu *resmenu = NULL;

    while (howmany) {
        /* If 'howmany' is negative the identifying effect is unlimited. */
        shMenu *idmenu = I->newMenu (prompt, shMenu::kCategorizeObjects |
                              (howmany < 0 ? shMenu::kMultiPick : 0));
        shObjectVector v;

        deselectObjectsByFlag (&v, Hero.cr ()->mInventory,
                               obj::known_exact);
        if (!v.count ()) {
            I->p ("You have %s unidentified items.",
                  didsomething ? "no more" : "no");
            break;
        }
        if (didsomething)
            I->pause ();

        didsomething = 0;
        for (int i = 0; i < v.count (); ++i) {
            obj = v.get (i);
            idmenu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        idmenu->accumulateResults ();
        /* When identifying is unlimited and there were many choices display
           result in menu too. */
        bool doann = true;
        if (howmany < 0 and idmenu->getNumChoices () >= 5) {
            doann = false;
            resmenu = I->newMenu (after, shMenu::kCategorizeObjects | shMenu::kNoPick);
        }
        /* Operate on all choices while identify quota is not exceeded. */
        while (idmenu->getPtrResult ((const void **) &obj)) {
            ++didsomething;
            --howmany; /* Decrease the quota. */

            identify_single_item (obj, infected);

            if (doann) { /* Announce each identify by one or ... */
                obj->announce ();
            } else {     /* ... add identified item to results. */
                resmenu->addPtrItem (obj->mLetter, obj->inv (), obj);
            }
        }
        if (didsomething and howmany > 0) {
            prompt = "What do you want to identify next?";
        } else { /* Player wants to leave some things unidentified. */
            break;
        }
        delete idmenu;
    }
    /* Display lots of results. */
    if (resmenu) {
        resmenu->finish ();
        delete resmenu;
    }
}


static shExecResult
doIdentify (shObject *, shObject *disk)
{
    int howmany = disk->isOptimized () ? RNG (1, 5) : 1;
    if (disk->is (obj::cracked) and disk->is (obj::known_cracked))
        howmany = -1;

    if (!disk->is (obj::known_type)) {
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        I->p ("This is an identify program.");
        I->pause ();
    }
    identifyObjects (howmany, disk->is (obj::infected));
    if (howmany > 1)  disk->set (obj::known_bugginess);
    return kNormal;
}


static shExecResult
doSpam (shObject *, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    /* Clerkbots on the level get to advertise their wares to you. */
    if (Level->mFlags & shMapLevel::kHasShop and !disk->isBuggy ()) {
        int ads = 0;
        for (int i = 0; i < Level->mCrList.count (); ++i) {
            shCreature *c = Level->mCrList.get (i);
            if (c->isA (kMonClerkbot) and !c->isHostile () and c->isInShop ()) {
                shMenu *list = I->newMenu ("For sale:", shMenu::kNoPick);
                int stock = 0;
                int sx = 0, sy = 0, ex = -1, ey = -1;
                int room = Level->getRoomID (c->mX, c->mY);
                Level->getRoomDimensions (room, &sx, &sy, &ex, &ey);
                /* Reveal shop location. */
                for (int x = sx; x <= ex; ++x)
                    for (int y = sy; y <= ey; ++y) {
                        if (disk->is (obj::infected) and !RNG (3))  continue;
                        shSquare *s = Level->getSquare (x, y);
                        /* Show only the clerkbot. */
                        shCreature *dc = (x == c->mX and y == c->mY) ? c : NULL;
                        shObjectVector *v = Level->getObjects (x, y);
                        shObject *best = NULL;
                        if (v) {
                            for (int i = 0; i < v->count (); ++i) {
                                if (disk->is (obj::infected) and !RNG (3))  continue;
                                shObject *obj = v->get(i);
                                if (obj->is (obj::unpaid)) {
                                    best = obj;
                                    obj->set (obj::known_appearance);
                                    ++stock;
                                    list->addPtrItem (' ', obj->inv (), obj);
                                }
                            }
                        }
                        if (best)  Level->remember (x, y, best->mIlkId);
                        Level->remember (x, y, s->mTerr);
                        I->draw (x, y, dc, NULL, best, NULL, s);
                    }
                I->p ("Come visit %s!", Level->shopAd (room));
                I->cursorAt (c->mX, c->mY);
                if (stock and I->yn ("See items for sale?"))
                    list->finish ();
                ++ads;
            }
        }
        if (ads)  return kNormal;
    }

    static const char *spammsg [] = {
        "Warning!  Your warpspace connection is not optimized!",
        "Make thousands of buckazoids per cycle from the comfort of your own pod!",
        "Beautiful Arcturian babes are waiting to meet you!",
        "You could meet a real Asari Consort!",
        "In need of help?  Syreen rescue crew can take care of all your needs!",
        "Invest in Bentusi technology and get rich quick!"
    };
    static const char *spamprompt [] = {
        "Transmit %d buckazoids to Meta-Net Systems for an upgrade?",
        "Upload $%d to Hyperpyramid Industries to buy business plan?",
        "Beam over your Arcturian bride for only %d buckazoids?",
        "Book an interview with Asari Consort for only %d buckazoids?",
        "Hire a Syreen rescue crew for only %d buckazoids?",
        "Pay $%d buckazoids for a blueprint of breaktrough Bentusi technology?"
    };
    static const char *spamconfirm [] = {
        "Upgrading ... done!  Enjoy the supremely speedy warpspace connection!",
        "Your plan should appear in your mailbox shortly.",
        "Thank you for your order.  Please wait 4-6 orbit cycles for delivery.",
        "Your appointment is being scheduled, thank you for your patience.",
        "The Penetrator has departed!  ETA in 2-4 orbits.",
        "Expect to receive the blueprints by courier in 2-3 orbit cycles."
    };

    static const char *charitymsg [] = {
        "Help feed starving %s babies from %s!",
        "Please provide for the needs of elderly %s citizens of %s!",
        "Aid poor %s patients at %s Planetary Clinic!"
    };
    static const char *charityprompt [] = {
        "Donate %d buckazoids to %s %s Nursery?",
        "Give $%d to %s Non-profit House of Serene Elder Life of %s?",
        "Send %d buckazoids to %s Planetary Clinic?"
    };
    static const char *charityconfirm [] = {
        "Thank you!  Here, have this picture of adorable %s baby.",
        "Thank you for your most generous donation.",
        "Your selfless gesture has helped save %s lives."
    };

    int max;
    bool charity;
    const char **message, **prompt, **confirm;
    if (disk->isBuggy () or (disk->isDebugged () and RNG (2))) {
        charity = false;
        message = spammsg;
        prompt = spamprompt;
        confirm = spamconfirm;
        max = 6;
    } else {
        charity = true;
        message = charitymsg;
        prompt = charityprompt;
        confirm = charityconfirm;
        max = 3;
    }

    int idx = RNG (max);
    int price = RNG (500) + 500;
    bool sucker = false;

    if (price > Hero.cr ()->countMoney ()) {
        price = Hero.cr ()->countMoney ();
    }
    const char *randomWorld ();
    const char *randomRace ();
    const char *world = randomWorld (), *race = randomRace ();
    I->p (message[idx], race, world);

    if (price < 50) {
        I->p ("Run this disk again when you've got some more buckazoids.");
        return kNoExpiry;
    }

    if (disk->isBuggy ()) {
        I->p ("Transmitting %d buckazoids through Zero-Click purchase plan...",
              price);
        disk->set (obj::known_bugginess);
        sucker = true;
    } else {
        int newidx = idx;
        /* Give clue this disk is infected. */
        if (disk->is (obj::infected)) {
            /* Message and money receiver will not match. */
            while (newidx == idx)
                newidx = RNG (max);
        }
        sucker = I->yn (prompt[newidx], price, world, race);
    }

    if (sucker) {
        Hero.cr ()->loseMoney (price);
        int newidx = idx;
        if (disk->is (obj::infected)) {
            while (newidx == idx)
                newidx = RNG (max);
        }
        I->p (confirm[newidx], race);
    } else if (!charity) {
        I->p ("Perhaps you might be interested in some of our other products?");
    }
    return kNoExpiry;
}


static shExecResult
doCorrupt (shObject *, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    criticalError (disk);
    return kNoExpiry;
}

static shExecResult
doHypnosis (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (Hero.cr ()->intr (kBlind)) {
        I->p ("You listen to some %s trance music.",
            disk->is (obj::infected) ? "pitiful" : "excellent");
        /* No revealing infectedness.  This subtle change is easy to miss. */
    } else {
        I->p ("%s displays a %shypnotic screensaver!",
              YOUR (computer), disk->is (obj::infected) ? "sickening, " : "");
        disk->set (obj::known_infected);
    }

    shAttackId aid = disk->is (obj::infected) ? kAttSickHypnoDiskD : kAttHypnoDiskD;
    Level->attackEffect (&Attacks[aid + disk->mBugginess],
        NULL, Hero.cr ()->mX, Hero.cr ()->mY, kNoDirection, Hero.cr (), 0, 0);
    return kNormal;
}

/* Detection programs have realtively minor punishment for having
   the disk infected. 1/3 chances for not reporting found item. */
static shExecResult
doDetectObject (shObject *computer, shObject *disk)
{
    shCompInterface interface = checkInterface (computer);
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    I->p ("You scan the area for objects.");
    int display = visualDisplaySuccesful (interface);
    int stack = 0;
    int objs = 0;
    int constr = 0;

    if (!display) I->p ("Getting objects list will have to suffice.");

    if (!Hero.cr ()->is (kConfused)) {
        for (int y = 0; y < MAPMAXROWS; ++y) {
            for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
                shObjectVector *v = Level->getObjects (x, y);
                if (display and v) {
                    Level->forgetObject (x, y);
                }
                if (v and (!disk->is (obj::infected) or RNG (3))) {
                    int besttype = kMaxObjectType;
                    shObject *bestobj = NULL;
                    for (int i = 0; i < v->count (); ++i) {
                        shObject *obj = v->get (i);
                        objs += obj->mCount;
                        if (disk->isOptimized ()) obj->set (obj::known_appearance);
                        if (obj->apparent ()->mType < besttype) {
                            besttype = v->get (i) -> apparent () -> mType;
                            bestobj = v->get (i);
                        }
                    }
                    ++stack;
                    if (display) {
                        Level->remember (x, y, bestobj->mIlkId);
                        Level->drawSq (x, y);
                    }
                }
                shCreature *c = Level->getCreature (x, y);
                if (c and (!disk->is (obj::infected) or RNG (3))
                      and c->myIlk ()->mType == kConstruct)
                {
                    ++constr;
                    if (display) {
                        Level->remember (x, y, c->mIlkId);
                        Level->drawSq (x, y);
                    }
                }
            }
        }
        if (!disk->isBuggy ()) {
            for (int i = 0; i < Hero.cr ()->mInventory->count (); ++i)
                if (!disk->is (obj::infected) or RNG (3)) {
                    shObject *obj = Hero.cr ()->mInventory->get (i);
                    obj->set (obj::known_appearance);
                }
        }
    } else if (display) {
        for (int i = 0; i < Level->mFeatures.count (); ++i) {
            shFeature *f = Level->mFeatures.get (i);
            if (!disk->is (obj::infected) or RNG (3)) {
                if (f->isTrap () and !f->isBerserkDoor ())
                    continue;
                if ((f->mType == shFeature::kDoorHiddenVert or
                    f->mType == shFeature::kDoorHiddenHoriz))
                {
                    if (disk->isBuggy ()) continue;
                    f->mType = shFeature::kDoorClosed;
                    f->mSportingChance = 0;
                    f->mTrapUnknown = 0;
                }
                if (f->isBerserkDoor () and disk->isOptimized ()) {
                    f->mTrapUnknown = 0;
                }
                Level->remember (f->mX, f->mY, f);
                Level->drawSq (f->mX, f->mY);
            }
        }
    }
    I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
    I->p ("%d object stack%s detected.",
        stack, (stack == 0 or stack > 1) ? "s" : "");
    I->p ("%d object%s detected.",
        objs, (objs == 0 or objs > 1) ? "s" : "");
    I->p ("%d construct%s detected.",
        constr, (constr == 0 or constr > 1) ? "s" : "");
    return kNormal;
}


/* Returns number of creatures of given type detected. */
static int
doDetectMonKind (shObject *computer, shObject *disk,
                bool (shCreature::*predicate) ())
{
    shCompInterface interface = checkInterface (computer);
    bool canGetInfo = visualDisplaySuccesful (interface);
    int cnt = 0;

    for (int i = 0; i < Level->mCrList.count (); ++i) {
        shCreature *c = Level->mCrList.get (i);
        if (c->mState == kActing and (c->*predicate) () and
            (!disk->is (obj::infected) or RNG (3)))
        {
            if (canGetInfo)
                I->drawMem (c->mX, c->mY, c, NULL, NULL, NULL);

            cnt++;
        }
    }
    I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
    return cnt;
}


static shExecResult
doDetectLife (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (Hero.cr ()->is (kConfused)) {
        I->p ("You scan the area for droids.");
        if (disk->isBuggy ()) {
            I->pause ();
            I->p ("These aren't the droids you're looking for.");
            return kNormal;
        }
        I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
        int cnt = doDetectMonKind (computer, disk, &shCreature::isRobot);
        I->p ("%d droid%s detected.", cnt, (cnt == 0 or cnt > 1) ? "s" : "");
    } else {
        I->p ("You scan the area for lifeforms.");
        if (disk->isBuggy ()) { /* you detect yourself */
            I->p ("1 lifeform detected.");
            I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
            return kNormal;
        }
        I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
        int cnt = doDetectMonKind (computer, disk, &shCreature::isAlive);
        I->p ("%d lifeform%s detected.", cnt, (cnt == 0 or cnt > 1) ? "s" : "");
    }
    return kNormal;
}


static shExecResult
doDetectTraps (shObject *computer, shObject *disk)
{
    int cnt = 0;
    int canGetInfo;
    shCompInterface interface = checkInterface (computer);

    I->p ("You scan the area for traps.");
    canGetInfo = visualDisplaySuccesful (interface);
    for (int i = 0; i < Level->mFeatures.count (); ++i) {
        shFeature *f = Level->mFeatures.get (i);
        if (f->isTrap () and (!disk->is (obj::infected) or RNG (3))) {
            if (f->isBerserkDoor ()) {
                continue; /* not a trap, a malfunction */
            }
            if (canGetInfo) {
                f->mTrapUnknown = 0;
                Level->remember (f->mX, f->mY, f);
            }
            ++cnt;
        }
    }
    I->p ("%d trap%s detected.", cnt, (cnt == 0 or cnt > 1) ? "s" : "");
    return kNormal;
}


static shExecResult
doDetectBugs (shObject *computer, shObject *disk)
{
    shObject *obj;
    int btotal = 0;

    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (Hero.cr ()->is (kConfused)) {
        I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
        I->p ("You scan the area for bugs.");
        btotal = doDetectMonKind (computer, disk, &shCreature::isInsect);
    } else if (disk->isBuggy ()) {
        disk->set (obj::known_bugginess);
        I->p ("This disk is buggy!");
        return kNormal;
    } else {
        I->p ("Scanning inventory for bugs...");
        int reveal = false;
        for (int i = 0; i < Hero.cr ()->mInventory->count (); ++i) {
            obj = Hero.cr ()->mInventory->get (i);
            /* One third chance to ignore item. */
            if (!disk->is (obj::infected) or RNG (3)) {
                if (obj->isBuggy ()) {
                    obj->set (obj::known_bugginess);
                    btotal++;
                } else if (disk->isOptimized ()) {
                    if (!obj->is (obj::known_bugginess))  reveal = true;
                    obj->set (obj::known_bugginess);
                }
            }
        }
        if (reveal)  disk->set (obj::known_bugginess);
    }
    I->p ("%d bugs found.", btotal);
    /* TODO: For infected disks sum of buggy items found and buggy items
       already known might not match.  Have a chance of noticing this
       automatically by player character based in Intelligence. */
    return kNormal;
}


static shExecResult
doMapping (shObject *computer, shObject *disk)
{
    shCompInterface interface = checkInterface (computer);
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (Hero.cr ()->is (kConfused)) {
        return doDetectTraps (computer, disk);
    }
    I->p ("A map appears on your screen!");
    if (visualDisplaySuccesful (interface)) {
        if (!disk->is (obj::infected)) {
            Level->reveal (0);
        } else {
            Level->reveal (1);
            I->p ("It seems to be missing some parts.");
            disk->set (obj::known_infected);
        }
    }
    if (computer->isA (kObjSatCom)) {
        int aliens = doDetectMonKind (computer, disk, &shCreature::isAlien);
        if (aliens) {
            I->p ("%d worthy prey found.", aliens);
        }
    }
    return kNormal;
}


static bool
canBeOptimized (shObject *obj)
{
    return !obj->isBugProof () and
        (!obj->isOptimized () or !obj->is (obj::known_bugginess));
}

static bool
canBeDebugged (shObject *obj)
{
    return !obj->isBugProof () and
        (obj->isBuggy () or !obj->is (obj::known_bugginess));
}


static shExecResult
doDebug (shObject *computer, shObject *disk)
{
    int objsChanged = 0;
    shObject *obj;
    shObjectVector v;

    if (computer->isBuggy ()) {
        computer->setDebugged ();
        I->p ("%s seems to be working much better.", YOUR (computer));
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        computer->set (obj::known_bugginess);
        return kNormal;
    } else if (disk->is (obj::infected) and
               !computer->is (obj::known_bugginess) and computer->isDebugged ()) {
        /* Pretend to debug debugged computer. */
        I->p ("%s seems to be working much better.", YOUR (computer));
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        computer->set (obj::known_bugginess);
        return kNormal;
    }

    if (Hero.cr ()->is (kConfused)) {
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        disk->clear (obj::known_bugginess); /* Will be renenabled later. */
        if (disk->isBuggy ()) {
            disk->setDebugged ();
            I->p ("You patch some bugs in %s.", YOUR (disk));
            objsChanged = 1;
        } else if (disk->isOptimized ()) {
            int i;
            if (disk->is (obj::cracked) and disk->is (obj::infected))
                selectObjectsByFunction (&v, Hero.cr ()->mInventory, &shObject::isOptimized);
            selectObjectsByFunction (&v, Hero.cr ()->mInventory, &shObject::isBuggy);
            for (i = 0; i < v.count (); i++) {
                obj = v.get (i);
                obj->setDebugged ();
                if (disk->is (obj::infected))
                    obj->set (obj::infected);
                if (obj->isA (kObjTorc) and obj->is (obj::worn))
                    Hero.torcCheck ();
            }
            I->p ("%d objects debugged.", i);
            objsChanged = i;
        } else {
            disk->setOptimized ();
            I->p ("You optimize %s.", YOUR (disk));
            objsChanged = 1;
        }
    } else if (disk->isOptimized ()) {
        if (!disk->is (obj::known_type)) {
            disk->set (obj::known_type);
            disk->setIlkKnown ();
            I->p ("You have found a floppy disk of debugging!");
        }

        bool unrestricted = disk->is (obj::cracked) and disk->is (obj::known_cracked);
        shMenu *dbgmenu = I->newMenu ("What do you want to debug or optimize?",
            shMenu::kCategorizeObjects |
            (unrestricted ? shMenu::kMultiPick : 0));
        selectObjectsByCallback (&v, Hero.cr ()->mInventory, canBeOptimized);
        if (!v.count ()) {
            I->p ("You have nothing to debug or optimize.");
            return kNormal;
        }
        for (int i = 0; i < v.count (); ++i) {
            obj = v.get (i);
            dbgmenu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }

        while (dbgmenu->getPtrResult ((const void **) &obj)) {
            if (obj->isBugProof ()) {
                I->p ("%s %s not appear to be affected.", YOUR (obj),
                    obj->mCount > 1 ? "do" : "does");
            } else if (obj->isBuggy ()) {
                obj->setDebugged ();
                I->p ("You patch some bugs in %s.", YOUR (obj));
                ++objsChanged;
            } else if (obj->isOptimized ()) {
                I->p ("You verify that %s is fully optimized.", YOUR (obj));
            } else {
                if (disk->is (obj::infected) and RNG (3)) {
                    I->p ("You verify that %s is debugged.", YOUR (obj));
                    disk->set (obj::known_infected);
                } else {
                    obj->setOptimized ();
                    I->p ("You optimize %s.", YOUR (obj));
                    ++objsChanged;
                }
            }
            if (obj->isA (kObjTorc) and obj->is (obj::worn))
                Hero.torcCheck ();
            obj->set (obj::known_bugginess);
            if (!unrestricted)
                break;
        }
    } else {
        bool unrestricted = disk->is (obj::cracked) and disk->is (obj::known_cracked);
        shMenu *dbgmenu = I->newMenu ("What do you want to debug?",
            shMenu::kCategorizeObjects |
            (unrestricted ? shMenu::kMultiPick : 0));
        selectObjectsByCallback (&v, Hero.cr ()->mInventory, canBeDebugged);
        if (!v.count ()) {
            I->p ("No showstopper bugs found.");
            return kNormal;
        }
        for (int i = 0; i < v.count (); ++i) {
            obj = v.get (i);
            dbgmenu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        while (dbgmenu->getPtrResult ((const void **) &obj)) {
            if (obj->isBugProof ()) {
                I->p ("%s %s not appear to be affected.", YOUR (obj),
                    obj->mCount > 1 ? "do" : "does");
            } else if (obj->isBuggy ()) {
                if (disk->is (obj::infected) and RNG (3)) {
                    I->p ("You verify that %s is buggy.", YOUR (obj));
                    disk->set (obj::known_infected);
                } else {
                    obj->clear (obj::known_bugginess); /* For prettier message. */
                    I->p ("Debugging %s.", YOUR (obj));
                    obj->setDebugged ();
                    obj->set (obj::known_bugginess);
                    ++objsChanged;
                }
            } else {
                I->p ("%s does not appear to be buggy.", YOUR (obj));
            }
            if (obj->isA (kObjTorc) and obj->is (obj::worn))
                Hero.torcCheck ();
            if (!unrestricted)
                break;
        }
    }
    disk->set (obj::known_type | obj::known_bugginess);
    disk->setIlkKnown ();
    if (objsChanged) {
        Hero.cr ()->computeIntrinsics ();
        Hero.cr ()->computeSkills ();
    }
    return kNormal;
}


static shExecResult
doEnhanceArmor (shObject *, shObject *disk)
{
    shObject *obj;
    shObjectVector v, w;

    selectObjects (&v, Hero.cr ()->mInventory, kArmor);
    selectObjectsByFlag (&w, &v, obj::worn);
    v.reset ();
    if (!Hero.cr ()->is (kConfused)) {
        selectObjectsByFunction (&v, &w, &shObject::isEnhanceable);
    } else {
        selectObjectsByFunction (&v, &w, &shObject::isFooproofable);
    }
    if (0 == v.count ()) {
        I->p ("Your skin tingles for a moment.");
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        return kNormal;
    }
    obj = v.get (RNG (v.count ()));
    shObjectIlk *ilk = obj->myIlk ();
    const char *your_armor = YOUR (obj);
    int max_opt_enhc = ilk->mMaxEnhancement;
    int max_dbg_enhc = ilk->mMaxEnhancement / 2 + ilk->mMaxEnhancement % 2;

    if (Hero.cr ()->is (kConfused)) {
        if (kNoEnergy != obj->vulnerability ()) {
            if (!obj->is (obj::fooproof)) {
                if (!disk->is (obj::infected)) {
                    obj->set (obj::fooproof);
                } else {
                    obj->clear (obj::fooproof);
                }
            }
        }
        I->p ("%s vibrates.", your_armor);
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        if (obj->mDamage) {
            if (!disk->is (obj::infected)) {
                obj->mDamage = 0;
            } else {
                --obj->mDamage;
            }
            if (!Hero.cr ()->intr (kBlind)) {
                if (!obj->mDamage) {
                    I->p ("%s looks as good as new!", your_armor);
                } else {
                    I->p ("%s looks only slightly better.", your_armor);
                    disk->set (obj::known_infected);
                }
            }
        }
    } else if (disk->isBuggy ()) {
        if (obj->mEnhancement > -max_opt_enhc) {
            if (!disk->is (obj::infected) or RNG (3)) {
                --obj->mEnhancement;
            }
        }
        if (!disk->is (obj::infected) or !RNG (3)) {
            obj->setBuggy ();
        }
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        if (!Hero.cr ()->intr (kBlind)) {
            I->p ("Blue smoke billows from %s.", your_armor);
        } else {
            I->p ("You smell smoke.");
        }
    } else if (disk->isOptimized ()) {
        int bonus = (max_opt_enhc + 1 - obj->mEnhancement) / 2;
        bonus = bonus <= 0 ? 0 : RNG (1, bonus);
        if (bonus > 1 and disk->is (obj::infected) and RNG (3))
            bonus = 1;
        obj->mEnhancement += bonus;
        if (bonus >= 1) {
            disk->set (obj::known_type);
            disk->setIlkKnown ();
            disk->set (obj::known_bugginess);
        }
        if (!obj->isOptimized ()) {
            if (!disk->is (obj::infected) or !RNG (3)) {
                obj->setOptimized ();
            }
        }
        I->p ("%s feels warm for %s.", your_armor, bonus > 1 ? "a while" :
              bonus > 0 ? "a moment" : "an instant");
    } else {
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        if (obj->mEnhancement < max_dbg_enhc) {
            if (!disk->is (obj::infected) or RNG (3)) {
                ++obj->mEnhancement;
            }
            I->p ("%s feels warm for a moment.", your_armor);
        } else {
            I->p ("%s feels warm for an instant.", your_armor);
            disk->set (obj::known_bugginess);
        }
        if (obj->isBuggy ()) {
            if (!disk->is (obj::infected) or RNG (3)) {
                obj->setDebugged ();
            }
        }
    }
    Hero.cr ()->computeAC ();
    return kNormal;
}


static shExecResult
doEnhanceImplant (shObject *, shObject *disk)
{
    shObject *obj;
    shObjectVector v, w;

    disk->set (obj::known_type);
    disk->setIlkKnown ();
    selectObjects (&v, Hero.cr ()->mInventory, kImplant);
    selectObjectsByFlag (&w, &v, obj::worn);
    v.reset ();
    selectObjectsByFunction (&v, &w, &shObject::isEnhanceable);
    if (0 == v.count ()) {
        I->p ("Your brain throbs.");  /* Ragnarok weird fume reference. */
        return kNormal;
    }
    obj = v.get (RNG (v.count ()));

    int old_enh = obj->mEnhancement;
    if (disk->isBuggy ()) {
        I->p ("You feel a stinging sensation in your %s.",
              describeImplantSite (obj->mImplantSite));
        if (!disk->is (obj::infected) or RNG (3)) {
            --obj->mEnhancement;
        }
        if (!disk->is (obj::infected) or !RNG (3)) {
            obj->setBuggy ();
        }
        disk->set (obj::known_bugginess);
    } else if (disk->isOptimized ()) {
        int bonus = (6 - obj->mEnhancement) / 2;
        bonus = bonus <= 0 ? 0 : RNG (1, bonus);
        if (bonus > 1 and disk->is (obj::infected) and RNG (3))
            bonus = 1;
        obj->mEnhancement += bonus;
        I->p ("You feel a %s sensation in your %s.",
              bonus > 1 ? "buzzing" :
              bonus > 0 ? "tingling" : "brief tingling",
              describeImplantSite (obj->mImplantSite));
    } else if (obj->mEnhancement < 3) {
        if (!disk->is (obj::infected) or !RNG (3)) {
            ++obj->mEnhancement;
        }
        I->p ("You feel a tingling sensation in your %s.",
              describeImplantSite (obj->mImplantSite));
    } else {
        I->p ("You feel a brief tingling sensation in your %s.",
              describeImplantSite (obj->mImplantSite));
    }

    if (old_enh != obj->mEnhancement and
        !obj->is (obj::known_type) and
        imp::is_abil_booster (obj))
    {
        obj->set (obj::known_type);
        obj->set (obj::known_enhancement);
        obj->announce ();
    }

    Hero.cr ()->computeIntrinsics ();
    return kNormal;
}


static shExecResult
doEnhanceWeapon (shObject *, shObject *disk)
{
    shObject *obj;
    shObjectVector v;
    const char *your_weapon;

    disk->set (obj::known_type);
    disk->setIlkKnown ();

    if (NULL == Hero.cr ()->mWeapon) {
        I->p ("Your %ss sweat profusely.",
            Hero.mProfession == XelNaga ? "claws" : "hands");
        return kNormal;
    }

    obj = Hero.cr ()->mWeapon;
    your_weapon = YOUR (obj);
    shObjectIlk *ilk = obj->myIlk ();
    bool hasattacks = ilk->mMeleeAttack or ilk->mMissileAttack or
                      ilk->mGunAttack or ilk->mZapAttack;
    if (!hasattacks or obj->isBugProof ()) {
        I->p ("Your %ss sweat profusely.",
            Hero.mProfession == XelNaga ? "claws" : "hands");
        return kNormal;
    }
    const char *third_person = obj->mCount > 1 ? "" : "s";
    bool isPhaser = obj->isA (kObjPhaserPistol);
    if (Hero.cr ()->is (kConfused)) {
        if (kNoEnergy != obj->vulnerability ()) {
            if (!obj->is (obj::fooproof)) {
                if (!disk->is (obj::infected)) {
                    obj->set (obj::fooproof);
                } else {
                    obj->clear (obj::fooproof);
                }
            }
        }
        I->p ("%s vibrate%s.", your_weapon, third_person);
        if (obj->mDamage) {
            if (!disk->is (obj::infected)) {
                obj->mDamage = 0;
            } else if (!disk->is (obj::infected) or !disk->is (obj::cracked)) {
                --obj->mDamage;
            }
            if (!Hero.cr ()->intr (kBlind)) {
                if (!obj->mDamage) {
                    I->p ("%s look%s as good as new!", your_weapon, third_person);
                } else {
                    I->p ("%s look%s only slightly better.", your_weapon, third_person);
                    disk->set (obj::known_infected);
                }
            }
        }
    } else if (disk->isBuggy ()) {
        I->p ("%s shudder%s%s!", your_weapon, third_person,
              obj->isBuggy () or
              !(obj->isMeleeWeapon () and obj->isAimedWeapon ()) ?
                  "" : " and welds to your hand");
        if (obj->isEnhanceable () and !obj->isA (kObjBloodSword)) {
            if (!disk->is (obj::infected) or RNG (3)) {
                if (disk->is (obj::cracked))
                    obj->mEnhancement = -obj->myIlk ()->mMaxEnhancement;
                else if (obj->mEnhancement > -obj->myIlk ()->mMaxEnhancement)
                    --obj->mEnhancement;
            }
        }
        if (!disk->is (obj::infected) or !RNG (3)) {
            obj->setBuggy ();
        }
        disk->set (obj::known_bugginess);
    } else if (disk->isOptimized ()) {
        if (!obj->isOptimized () and obj->is (obj::known_bugginess))
            disk->set (obj::known_bugginess);
        if (!obj->isOptimized ()) {
            if (!disk->is (obj::infected) or !RNG (3)) {
                obj->setOptimized ();
            }
        }
        if (!obj->isEnhanceable () or obj->isA (kObjBloodSword)) {
            I->p ("%s feel%s warm for an instant.", your_weapon, third_person);
        } else {
            int bonus;
            if (disk->is (obj::cracked)) {
                bonus = obj->myIlk ()->mMaxEnhancement - obj->mEnhancement;
            } else {
                bonus = (obj->myIlk ()->mMaxEnhancement + 1 - obj->mEnhancement) / 2;
                bonus = maxi (0, bonus);
                bonus = RNG (1, bonus);
            }
            if (disk->is (obj::infected)) {
                if (isPhaser)
                    bonus = obj->mEnhancement > 0 ? -1 : 0;
                else if (bonus > 1)
                    bonus = 1;
            }
            if (bonus > 1)  disk->set (obj::known_bugginess);
            obj->mEnhancement += bonus;
            if (isPhaser) {
                I->p ("You %s the phase variance of %s.",
                bonus > 1 ? "modify" :
                bonus > 0 ? "tune" : "fine tune", your_weapon);
            } else {
                I->p ("%s feel%s warm for %s.", your_weapon, third_person,
                    bonus > 1 ? "a while" :
                    bonus > 0 ? "a moment" : "an instant");
            }
        }
    } else {
        if (!obj->isEnhanceable () or obj->isA (kObjBloodSword)) {
            I->p ("%s feel%s warm for an instant.", your_weapon, third_person);
        } else if (obj->mEnhancement < obj->myIlk ()->mMaxEnhancement / 2) {
            if (!disk->is (obj::infected) or !RNG (3)) {
                if (disk->is (obj::cracked))
                    obj->mEnhancement = obj->myIlk ()->mMaxEnhancement / 2;
                else if (obj->mEnhancement < obj->myIlk ()->mMaxEnhancement / 2)
                    ++obj->mEnhancement;
            }
            if (isPhaser) {
                I->p ("You modify the phase variance of %s.", your_weapon);
            } else {
                I->p ("%s feel%s warm for a moment.", your_weapon, third_person);
            }
        } else {
            if (isPhaser
                and obj->mEnhancement == obj->myIlk ()->mMaxEnhancement / 2
                and (!disk->is (obj::infected) or !RNG (3)))
            {
                ++obj->mEnhancement;
                I->p ("You finish fine tuning the phase variance of %s.", your_weapon);
            } else {
                I->p ("%s feel%s warm for an instant.", your_weapon, third_person);
            }
        }
        if (obj->isBuggy ()) {
            if (!disk->is (obj::infected) or RNG (3)) {
                obj->setDebugged ();
            }
        }
    }
    return kNormal;
}



static shExecResult
doWarpTrace (shObject *, shObject *disk)
{
    if (Level->noTransport ()) {
        I->p ("Nothing happens.");
        disk->maybeName ();
        return kNormal;
    }

    if (disk->isBuggy ()) {
        if (Level->attractWarpMonsters (Hero.cr ()->mX, Hero.cr ()->mY)) {
            disk->set (obj::known_type);
            disk->setIlkKnown ();
            disk->set (obj::known_bugginess);
        } else {
            I->p ("Nothing happens.");
            disk->maybeName ();
        }
        return kNormal;
    }
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    int found = 0;
    for (int i = 0; i < MAXTRACES; ++i) {
        /* Give traces from oldest to freshest. */
        int idx = (trn::last_trace + i) % MAXTRACES;
        trn::WarpTrace *trace = &trn::traces[idx];
        if (trace->level == Hero.cr ()->mLevel and
            trace->time + 20000 >= Clock)
        {
            int dist = distance (Hero.cr (), trace->dst_x, trace->dst_y);
            I->p ("%s transported to a %s location.", trace->who,
                dist < 60 ? "neighboring" :
                dist < 120 ? "nearby" : "distant");
            ++found;
        }
    }
    if (!found)
        I->p ("No warp traces found.");

    disk->set (obj::known_bugginess);
    if (found and disk->isOptimized ()) {
        int last = (trn::last_trace + MAXTRACES - 1) % MAXTRACES;
        if (I->yn ("Follow last trace?")) {
            int x = -1, y = -1;
            if (Hero.cr ()->is (kConfused)) {
                x = trn::traces[last].src_x;
                y = trn::traces[last].src_y;
            } else {
                x = trn::traces[last].dst_x;
                y = trn::traces[last].dst_y;
            }
            Level->findNearbyUnoccupiedSquare (&x, &y);
            Hero.cr ()->transport (x, y, 0, disk->is (obj::infected));
        }
    }
    return kNormal;
}


/* returns 0 on success
           1 if the creature dies
           -1 if otherwise failed
*/
int
shCreature::transport (int x, int y, int control, bool imprecise, bool force)
{   /* Transport may be completely blocked. */
    if (isHero ()) {
        if (Level->noTransport () and !force) {
            I->p ("You %s for a moment.",
                Hero.cr ()->intr (kBlind) ? "shudder" : "flicker");
            return 0;
        }
    }
    /* Warp control intrinsics modifies chances. */
    control += mIntrinsics.get (kWarpControl);

    int oldX = mX, oldY = mY;
    bool move = true;

    if (-1 == x) {
        if (isHero () and RNG (100) < control) {
            I->getSquare ("Transport to what location?", &x, &y, -1);
        } else {
            move = false;
            if (!imprecise) {  /* Find a square without trap. */
                mLevel->findUnoccupiedSquare (&x, &y);
            } else {  /* Indiscriminate.  May drop you into a pit. */
                mLevel->findOpenSquare (&x, &y);
            }
        }
    }

    if (isA (kMonShodan))
        move = false; /* Shodan is unaffected by mainframe imprecision. */

    /* A transport may place the creature off the destination in some cases. */
    if (move and !force and (Level->isMainframe () or imprecise)) {
        int tries = 50;
        int nx, ny;
        do {
            nx = x + RNG (15) - 7;
            ny = y + RNG (9) - 4;
            Level->findNearbyUnoccupiedSquare (&nx, &ny);
        } while (nx == x and ny == y and tries--);
        if (isHero ())
            I->p ("It's hard to transport accurately in here.");
        x = nx; y = ny;
    }

    mTrapped.mInPit = 0;
    mTrapped.mDrowning = 0;
    if (mZ < 0)
        mZ = 0;

    int tries = 30;
    while (tries--) {
        int res = mLevel->moveCreature (this, x, y);
        if (-1 == res) {
            /* Try again, random location. */
            mLevel->findUnoccupiedSquare (&x, &y);
        } else {
            trn::register_trace (this, oldX, oldY, mX, mY);
            if (isHero ())
                Level->attractWarpMonsters (x, y);
            return res;
        }
    }
    if (isHero ())
        I->p ("Your transport fails.");
    return -1;
}


static shExecResult
doTransport (shObject *, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (disk->isBuggy () and !RNG (20)) {
        /* Failed transport. */
        I->p ("The process fails.");
        disk->set (obj::known_bugginess);
        return kNormal;
    } else if (disk->isOptimized () and !Hero.cr ()->is (kConfused)) {
        /* Controlled local transport. */
        Hero.cr ()->transport (-1, -1, 100, disk->is (obj::infected));
        return kNormal;
    } else {
        /* Random local transport. */
        Hero.cr ()->transport (-1, -1);
        return kNormal;
    }
}

/* Gives only part of a string. */
const char *
shCreature::radLevels (void)
{
    return
        mRad > 1200 ? "unbelieveable" :
        mRad >  500 ? "extreme" :
        mRad >  300 ? "lethal" :
        mRad >  200 ? "very dangerous" :
        mRad >  125 ? "dangerous" :
        mRad >   75 ? "significant" :
        mRad >   50 ? "moderate" :
        mRad >   25 ? "acceptable" :
                      "minimal";
}

/* List hidden properties of a creature. */
void
shCreature::doDiagnostics (int quality, int present, /* 1 */ FILE *file /* NULL */)
{
    struct shGrammar
    {
        char you[4]; char r[5]; char are[9]; char have[9];
    };
    const shGrammar declension[5] =
    {
        {    "You",     "Your",   "You are",  "You have" },
        {    "He",      "His",    "He is",    "He has"   },
        {    "She",     "Her",    "She is",   "She has"  },
        {    "It",      "Its",    "It is",    "It has"   },
        {    "You",     "Your",   "You were", "You had"  }
    };
#   define YOU offsetof (shGrammar, you)
#   define YOU_R offsetof (shGrammar, r)
#   define YOU_ARE offsetof (shGrammar, are)
#   define YOU_HAVE offsetof (shGrammar, have)
    static const struct shDiagTestData
    {
        const char *fmt;
        unsigned int offset;
        shIntrinsic in;
    } tests[] =
    {
        { "%s very acute scent sense.",     YOU_HAVE,   kScent },
        { "%s can swim.",                   YOU,        kCanSwim },
        { "%s terrifying presence.",        YOU_HAVE,   kScary },
        { "%s telepathic.",                 YOU_ARE,    kTelepathy },
        { "%s tremor sense.",               YOU_HAVE,   kTremorsense },
        { "%s EM field vision.",            YOU_HAVE,   kEMFieldVision },
        { "%s night vision.",               YOU_HAVE,   kNightVision },
        { "%s X-ray vision.",               YOU_HAVE,   kXRayVision },
        { "%s reflection.",                 YOU_HAVE,   kReflection },
        { "%s narcoleptic.",                YOU_ARE,    kNarcolepsy },
        { "%s a light source.",             YOU_ARE,    kLightSource },
        { "%s shielded.",                   YOU_ARE,    kShielded },
        { "%s psionic protection.",         YOU_HAVE,   kBrainShielded },
        { "%s an air supply.",              YOU_HAVE,   kAirSupply },
        { "%s automatic searching.",        YOU_HAVE,   kAutoSearching },
        { "%s breathless.",                 YOU_ARE,    kBreathless },
//        { "%s sessile.",                    YOU_ARE,    &shCreature::isSessile },
        { "%s able to multiply rapidly.",   YOU_ARE,    kMultiplier },
//        { "%s attracted to warp traces.",   YOU_ARE,    &shCreature::isWarpish },
        { "%s body eliminates radiation.",  YOU_R,      kRadiationProcessing },
        { "%s body regenerates quickly.",   YOU_R,      kAutoRegeneration },
        { "%s highly acidic blood.",        YOU_HAVE,   kAcidBlood },
        { "%s monitor your health.",        YOU,        kHealthMonitoring },
//        { "%s will explode if destroyed.",  YOU,        &shCreature::isExplosive },
        { "%s able to understand alien languages.", YOU_ARE, kTranslation }
    };
    static const struct shDiagVarData
    {
        char fmt[50];
        unsigned int offset;
        int shCreature::*member;
    } vars[] =
    {
        { "%s speed %s %d.",                YOU_R, &shCreature::mSpeed },
        { "%s speed bonus %s %d.",          YOU_R, &shCreature::mSpeedBoost },
        { "%s natural armor bonus %s %d.",  YOU_R, &shCreature::mNaturalArmorBonus },
        { "%s to hit modifier %s %d." ,     YOU_R, &shCreature::mToHitModifier },
        { "%s damage modifier %s %d." ,     YOU_R, &shCreature::mDamageModifier },
        { "%s psionic modifier %s %d." ,    YOU_R, &shCreature::mPsiModifier }
    }; /* How many first variables reveal even if value is zero. */
#   define ALWAYS_SHOW 1
    const int BUFSIZE = 70;
    char buf[BUFSIZE];
    const shGrammar *you;
    if (!present) {
        you = &declension[4];
    } else if (isHero ()) {
        you = &declension[0];
    } else if (hasMind () or mGender == kMale) {
        you = &declension[1];
    } else if (hasMind () or mGender == kFemale) {
        you = &declension[2];
    } else {
        you = &declension[3];
    }
    shMenu *menu = I->newMenu ("Personal diagnostics", shMenu::kNoPick);
    /* Report variables first. */
    for (unsigned i = 0; i < sizeof (vars) / sizeof (shDiagVarData); ++i) {
        if (i < ALWAYS_SHOW or this->*vars[i].member) {
            snprintf (buf, BUFSIZE, vars[i].fmt,
                (char *) ((char *) you + vars[i].offset),
                (present) ? "is" : "was", this->*vars[i].member);
            if (!file) {
                menu->addText (buf);
            } else {
                fprintf (file, "%s\n", buf);
            }
        }
    } /* Resistance or vulnerability to energy types. */
    for (int e = kNoEnergy; e < kMaxEnergyType; e++) {
        int resist = getResistance ((shEnergyType) e);
        if (resist and quality < 0 and !RNG (4+quality)) {
            resist = 0; /* Keep silent about a resist. */
        }
        const char *description = energyDescription ((shEnergyType) e);
        if (resist and description) {
            snprintf (buf, BUFSIZE, "%s %s to %s.", you->are,
                resist < 0  ? "particularly vulnerable" :
                resist < 5  ? "somewhat resistant" :
                resist < 10 ? "resistant" :
                resist < 50 ? "extremely resistant" :
                              "nearly invulnerable",
                description);
            if (!file) {
                menu->addText (buf);
            } else {
                fprintf (file, "%s\n", buf);
            }
        }
    } /* Test for intrinsics. */
    for (unsigned i = 0; i < sizeof (tests) / sizeof (shDiagTestData); ++i) {
        if (tests[i].in == kXRayVision and
            this->mInnateIntrinsics.get (kBlind))
        {   /* Naturally blind characters do not benefit from X-ray vision. */
            continue;
        }
        if (intr (tests[i].in)) {   /* Present. */
            /* Default report line. */
            snprintf (buf, BUFSIZE, tests[i].fmt,
                (char *) ((char *) you + tests[i].offset));
            if (!present) { /* Past tense clumsiness. */
                /* This is not pretty!  Got no better idea now. :( -- MB */
                char *mod;
                if (strstr (buf, "swim")) {
                    snprintf (buf, BUFSIZE, "You could swim.");
                } else if ((mod = strstr (buf, "tes"))) {
                    *(mod + 2) = 'd'; /* eliminated, regenerated */
                } else if (strstr (buf, "monitor")) {
                    snprintf (buf, BUFSIZE,
                        "You monitored your health.");
                } else if (strstr (buf, "explode")) {
                    snprintf (buf, BUFSIZE, "You exploded.");
                }
            }
            if (!file) {
                menu->addText (buf);
            } else {
                fprintf (file, "%s\n", buf);
            }
        }
    } /* Vaguely express rad levels. */
    if (isHero ()) { /* Not implemented for monsters. */
        snprintf (buf, BUFSIZE, "%s been exposed to %s levels of radiation.",
                  you->have, radLevels ());
        if (!file) {
            menu->addText (buf);
        } else {
            fprintf (file, "%s\n", buf);
        }
    }
    int tmp = mImpregnation;
    if (tmp) {
        if (tmp > 70 or !present) {
            snprintf (buf, BUFSIZE, "The alien lifeform inside %s"
                " %s %skill %s%s.", the (), present ? "will" : "would",
                tmp > 70 ? "soon " : "", her (), present ? "" : " anyway");
        } else {
            snprintf (buf, BUFSIZE, "%s been impregnated with "
                "a dangerous alien lifeform.", you->have);
        }
        if (!file) {
            menu->addText (buf);
        } else {
            fprintf (file, "%s\n", buf);
        }
    } /* Only hero may have this --> only second person form needed. */
    if (isHero ()) {
        if (Hero.getStoryFlag ("superglued tongue")) {
            snprintf (buf, BUFSIZE, "The canister of super glue %s eventually"
                " come off.", (present) ? "will" : "would");
            if (!file) {
                menu->addText (buf);
            } else {
                fprintf (file, "%s\n", buf);
            }
        }
    }
    if (!file)  menu->finish ();
    delete menu;
}


static shExecResult
doDiagnostics (shObject *, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    I->p ("Initiating body scan.");
    I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
    /* Bugginess and infectedness each decrease quality by one point. */
    Hero.cr ()->doDiagnostics (disk->mBugginess - disk->is (obj::infected));
    return kNormal;
}

static bool
isHackable (shObject *obj)
{
    return obj->isCrackable ();
}

static bool
isBioHackable (shObject *obj)
{
    return obj->isCrackable () or
        (obj->is (obj::known_type) and obj->isA (kObjRestorationCanister));
}

static shExecResult
doHacking (shObject *computer, shObject *disk)
{
    shObject *obj;
    shObjectVector v;

    if (!disk->is (obj::known_type)) {
        I->p ("You have found a floppy disk of hacking!");
        disk->set (obj::known_type);
        disk->setIlkKnown ();
    }
    if (computer->isA (kObjBioComputer)) {
        selectObjectsByCallback (&v, Hero.cr ()->mInventory, isBioHackable);
    } else {
        selectObjectsByCallback (&v, Hero.cr ()->mInventory, isHackable);
    }
    obj = Hero.cr ()->quickPickItem (&v, "hack", 0);
    if (!obj) return kNormal;

    if (obj->isA (kObjBlankDisk)) {
        I->p ("No program there.");
        obj->set (obj::known_type);
        return kNormal;
    }
    if (obj->is (obj::cracked) and !obj->is (obj::known_cracked)) {
        obj->set (obj::known_cracked);
        if (obj->isA (kFloppyDisk)) {
            I->p ("This software has already been cracked!");
        } else if (obj->has_subtype (keycard)) {
            I->p ("The access level of this keycard has already been hacked!");
        } else {
            I->p ("This thingy has already been cracked!");
        }
        return kNormal;
    }
    int docrack = 1;
    if (obj->is (obj::infected) and obj->is (obj::known_infected)) {
        docrack = !I->yn ("Do you wish to remove virus manually?");
    }

    int score = hackingRoll (computer);
    if (obj->isA (kFloppyDisk)) {
        if (disk->isOptimized ()) {
            score += 2;
        } else if (disk->isBuggy ()) {
            score -= 4;
        }
        if (disk->is (obj::infected)) {
            score -= 3;
        }
        if (obj->mIlkId == kObjHackingDisk or obj->mIlkId == kObjMatterCompiler) {
            score -= 100; /* this is nearly impossible */
        }
        if (obj->isOptimized () or obj->isBuggy ()) {
            score -= 2; /* debugged code is more straightforward to hack */
        }
        if (!obj->is (obj::known_type)) {
            score -= 2; /* you have no clue what it is supposed to do */
        }
        if (obj->is (obj::infected)) {
            score -= 2; /* yet another complication */
            if (!obj->is (obj::known_infected)) {
                score -= 2; /* and you still have to learn about it */
            }
        }
        if (docrack) {
            if (score >= 15 and !disk->is (obj::infected)) {
                if (!obj->is (obj::cracked)) {
                    I->p ("You remove the copy protection from %s.", YOUR (obj));
                    obj->set (obj::cracked);
                    obj->set (obj::known_cracked);
                } else {
                    I->p ("You reenable the copy protection of %s.", YOUR (obj));
                    obj->clear (obj::cracked);
                    obj->set (obj::known_cracked);
                }
                if (score >= 18) {
                    if (obj->is (obj::infected) and !obj->is (obj::known_infected)) {
                        I->p ("You also notice trojan code interspersed among program routines.");
                    }
                    obj->set (obj::known_infected);
                }
                return kNormal;
            }
        } else {
            if (score >= 12 and !disk->is (obj::infected)) {
                I->p ("You remove the virus from %s.", YOUR (obj));
                obj->clear (obj::infected);
                obj->set (obj::known_cracked);
                if (score >= 15) {
                    if (obj->is (obj::cracked) and !obj->is (obj::known_cracked)) {
                        I->p ("You also notice this software is cracked.");
                    }
                    obj->set (obj::known_cracked);
                }
                return kNormal;
            }
        }
        I->p ("Your hacking attempt was a failure!");
        if (docrack)  obj->set (obj::known_cracked);
        /* TODO: Alternatively corrupt the disk. */
        if (score <= 9) {
            obj->setBuggy ();
        }
    } else if (obj->has_subtype (keycard)) {
        I->p ("You attach %s to USB port of %s.", THE (obj), YOUR (computer));
        if (obj->is (obj::cracked) and !obj->is (obj::known_cracked)) {
            obj->set (obj::known_cracked);
            I->p ("This keycard has already been cracked!");
            return kNormal;
        }
        obj->set (obj::known_cracked);
        if (score >= 13 and !disk->is (obj::infected)) {
            if (!obj->is (obj::cracked)) {
                I->p ("You set highest level of access on %s.", YOUR (obj));
                obj->identify (); /* Identify, since you learned its codes. */
                obj->set (obj::cracked);
            } else {
                I->p ("You reset access level to original state on %s.", YOUR (obj));
                obj->clear (obj::cracked);
            }
            if (obj->isA (kObjMasterKeycard)) {
                I->p ("It is a master keycard so you did it for sport.");
            }
        } else {
            I->p ("You fail to find access level data for %s.", YOUR (obj));
            if (obj->isA (kObjMasterKeycard)) {
                obj->set (obj::known_type);
                I->p ("It is a master keycard.  No point in this anyway.");
            }
        }
    } else if (obj->isA (kObjRemoteControl)) {
        I->p ("You attach %s to USB port of %s.", THE (obj), YOUR (computer));
        if (obj->is (obj::cracked) and !obj->is (obj::known_cracked)) {
            obj->set (obj::known_cracked);
            I->p ("This remote has already been cracked!");
            return kNormal;
        }
        obj->set (obj::known_cracked);
        if (score >= 14 and !disk->is (obj::infected)) {
            if (!obj->is (obj::cracked)) {
                I->p ("You disable protocol overrides on %s.", YOUR (obj));
                obj->set (obj::cracked);
            } else {
                I->p ("You reset protocol overrides on %s.", YOUR (obj));
                obj->clear (obj::cracked);
            }
        } else {
            I->p ("You fail to disable protocl overrides on %s.", YOUR (obj));
        }
    } else if (obj->isA (kObjRestorationCanister)) {
        I->p ("You submerge %s in %s.", THE (obj), YOUR (computer));
        obj = Hero.cr ()->removeOneObjectFromInventory (obj);
        if (score >= 20 and !disk->is (obj::infected)) {
            I->p ("You remove limitations on nanomachines in %s.", YOUR (obj));
            obj->mIlkId = kObjGainAbilityCanister;
        } else if (score <= 15 or disk->is (obj::infected)) {
            I->p ("You badly fail to remove limitations on nanomachines.");
            delete obj;
            obj = NULL;
            I->p ("Nanomachines assemble battle forms and attack!");
            int slimes = RNG (4, 8);
            for (int i = 0; i < slimes; ++i) {
                int x = Hero.cr ()->mX, y = Hero.cr ()->mY;
                if (!Level->findNearbyUnoccupiedSquare (&x, &y)) {
                    shCreature *slime = shCreature::monster (kMonMetalSlime);
                    Level->putCreature (slime, x, y);
                }
            }
        } else {
            I->p ("You fail to remove limitations on nanomachines.");
        }
        if (obj)  Hero.cr ()->addObjectToInventory (obj);
    } else {
        I->p ("Error POPCORN DELTA!");
        I->p ("You somehow chosen object for hacking without associated routine.");
        I->p ("Please report this bug.");
    }
    return kNormal;
}

void
doInstallRawSystem (shObject *computer, shObject *disk, int speak)
{
    if (disk->isOptimized ()) {
        computer->mEnhancement = RNG (kAverageSys, kSuperbSys);
    } else if (disk->isBuggy ()) {
        computer->mEnhancement = RNG (kAbysmalSys, kAverageSys);
    } else {
        computer->mEnhancement = RNG (kBadSys, kGoodSys);
    }
    /* You watched the installation and have knowledge. */
    if (!Hero.cr ()->intr (kBlind) and
        (RNG (5, 10) <= Hero.cr ()->getSkill (kHacking)->getValue ()))
    {
        computer->set (obj::known_enhancement);
    } else
        computer->clear (obj::known_enhancement);
    if (speak)
        I->p ("Operating system installed successfully.");
}

/* Generic stuff. */
#define SYS_UPGRADE "Upgrading system architecture."
#define SYS_DOWNGRADE "Downgrading system architecture."
/* Do we need better excuse for capping this? -- MB */
#define SYS_DEBUGGED_CAP \
    I->p ("This lite system distribution does not contain all packages."); \
    I->p ("Please upgrade to deluxe system distribution.")
/* Omitting semicolon above so I can have one after the macro. */
static shExecResult
doInstallSystem (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (Hero.cr ()->is (kConfused)) {
        I->p ("You scan the area for programs.");
        if (disk->isBuggy ()) {
            I->p ("Remote repository does not include your favorite programs.");
            I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
            return kNormal;
        }
        I->pauseXY (Hero.cr ()->mX, Hero.cr ()->mY);
        int cnt = doDetectMonKind (computer, disk, &shCreature::isProgram);
        I->p ("%d program%sdetected.", cnt, (cnt == 0 or cnt > 1) ? "s " : " ");
        return kNormal;
    }

    shObject *obj;
    shObjectVector v;
    selectObjectsByFunction (&v, Hero.cr ()->mInventory, &shObject::isComputer);
    obj = Hero.cr ()->quickPickItem (&v, "install or upgrade system", 0);
    if (!obj)  return kNormal;

    int hadSystem = (obj->mEnhancement != kNoSys);
    /* Rule for this is: if specific module was installed the system
       quality is known precisely. */
    int revealSysQuality = 0;

    if (disk->isBuggy ())   /* Buggy disks attempt downgrade. */
        disk->set (obj::known_bugginess);

    if (disk->is (obj::infected)) {
        if (!computer->is (obj::known_enhancement)) {
            if (disk->isBuggy ()) {
                I->p (SYS_DOWNGRADE);
            } else {
                I->p (SYS_UPGRADE);
            }
        } else { /* TODO: Get rid of this and change infected behavior. */
            criticalError (disk);
        }
        return kNormal;
    }
    /* FIXME: This is just bait for creating some kind of table and
              compressing all the lines into more readable thing. */
    switch (obj->mEnhancement) {
        case kNoSys:
            doInstallRawSystem (obj, disk, 1);
            break;
        case kAbysmalSys:
            if (disk->isBuggy ()) {
                I->p ("Further system downgrade not possible.");
                disk->set (obj::known_bugginess);
                revealSysQuality = 1;
            } else {
                I->p ("Installing voice support module.");
                ++obj->mEnhancement;
                /* Does not reveal bugginess because player has no idea
                   whether this disk would get him past kGoodSys. */
                revealSysQuality = 1;
            }
            break;
        case kTerribleSys:
            if (disk->isBuggy ()) {
                I->p ("Uninstalling voice support module.");
                --obj->mEnhancement;
                disk->set (obj::known_bugginess);
                revealSysQuality = 1;
            } else {
                I->p (SYS_UPGRADE);
                ++obj->mEnhancement;
            }
            break;
        case kBadSys:
            if (disk->isBuggy ()) {
                I->p (SYS_DOWNGRADE);
                --obj->mEnhancement;
                disk->set (obj::known_bugginess);
            } else {
                I->p (SYS_UPGRADE);
                ++obj->mEnhancement;
            }
            break;
        case kPoorSys:
            if (disk->isBuggy ()) {
                I->p (SYS_DOWNGRADE);
                --obj->mEnhancement;
                disk->set (obj::known_bugginess);
            } else {
                I->p ("Installing hypodermic nerve interface support.");
                ++obj->mEnhancement;
                revealSysQuality = 1;
            }
            break;
        case kAverageSys:
            if (disk->isBuggy ()) {
                I->p ("Uninstalling hypodermic nerve interface support.");
                --obj->mEnhancement;
                disk->set (obj::known_bugginess);
                revealSysQuality = 1;
            } else {
                I->p ("Installing transparent update architecture.");
                ++obj->mEnhancement;
                revealSysQuality = 1;
            }
            break;
        case kFineSys:
            if (disk->isBuggy ()) {
                I->p ("Uninstalling transparent update architecture.");
                --obj->mEnhancement;
                disk->set (obj::known_bugginess);
                revealSysQuality = 1;
            } else {
                I->p ("Installing system hardware analyzer.");
                ++obj->mEnhancement;
                revealSysQuality = 1;
            }
            break;
        case kGoodSys:
            if (disk->isBuggy ()) {
                I->p ("Uninstalling system hardware analyzer.");
                --obj->mEnhancement;
                revealSysQuality = 1;
            } else if (disk->isOptimized ()){
                I->p ("Installing software reliability tester.");
                disk->set (obj::known_bugginess);
                ++obj->mEnhancement;
                revealSysQuality = 1;
            } else {
                SYS_DEBUGGED_CAP;
                disk->set (obj::known_bugginess);
            }
            break;
        case kExcellentSys:
            if (disk->isBuggy ()) {
                I->p ("Uninstalling software reliability tester.");
                --obj->mEnhancement;
                revealSysQuality = 1;
            } else if (disk->isOptimized ()){
                I->p (SYS_UPGRADE);
                ++obj->mEnhancement;
            } else {
                SYS_DEBUGGED_CAP;
                disk->set (obj::known_bugginess);
            }
            break;
        case kSuperbSys:
            if (disk->isBuggy ()) {
                I->p (SYS_DOWNGRADE);
                --obj->mEnhancement;
            } else {
                I->p ("Further system upgrade not possible.");
                revealSysQuality = 1;
            }
            break;
    }

    if (hadSystem) {
        if (revealSysQuality) {
            obj->set (obj::known_enhancement);
        }
    }
    return kNormal;
}

/* Antivirus has two versions.  One common but only useful as a temporary */
static shExecResult      /* solution and one rare but very useful. */
doAntiVirus (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (computer->is (obj::infected)) {
        computer->clear (obj::infected);
        computer->set (obj::known_infected);
        I->p ("%s seems to be working much better.", YOUR (computer));
        return kNormal;
    }

    if (Hero.cr ()->is (kConfused)) {
        shObjectVector v;
        shObject *obj;
        selectObjectsByFlag (&v, Hero.cr ()->mInventory, obj::infected);
        if (disk->isOptimized ()) {
            for (int i = 0; i < v.count (); ++i) {
                obj = v.get (i);
                obj->set (obj::known_infected);
            }
        }
        if (disk->isBuggy ())
            v.reset ();
        I->p ("Found %s%d infected items.",
            disk->isOptimized () ? "and marked " : "", v.count ());
        if (disk->isOptimized ()) {
            disk->set (obj::known_bugginess);
        }
        return kNormal;
    }

    const char *prompt = "What do you want to scan for viruses?";
    bool unrestricted = disk->is (obj::cracked) and disk->is (obj::known_cracked);
    int howmany = RNG (1, 3) + disk->isOptimized ();
    if (unrestricted)  howmany = -1;
    int didsomething = 0;

    while (howmany) {
        shMenu *scanmenu = I->newMenu (prompt, shMenu::kCategorizeObjects |
            (unrestricted ? shMenu::kMultiPick : 0));
        shObjectVector v;
        selectObjectsByFunction (&v, Hero.cr ()->mInventory, &shObject::isInfectable);
        int candidates = 0;
        shObject *obj;
        for (int i = 0; i < v.count (); ++i) {
            obj = v.get (i);
            /* Skip known clean items. */
            if (obj->is (obj::known_infected) and !obj->is (obj::infected))  continue;
            /* Skip known infected items when you are aware
               no recovery is possible because disk is buggy. */
            if (disk->isBuggy () and disk->is (obj::known_bugginess) and
                obj->is (obj::infected) and obj->is (obj::known_infected))   continue;
            ++candidates;
            scanmenu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        if (!candidates) {
            if (!v.count ()) {
                I->p ("You have nothing to work on.");
                return kNormal;
            }
            if (disk->isBuggy () and disk->is (obj::known_bugginess) and !candidates) {
                I->p ("You have nothing %sto scan.",
                      didsomething ? "more " : "");
                return kNormal;
            }
            I->p ("You have nothing %sto scan or recover.",
                  didsomething ? "more " : "");
            return kNormal;
        }
        if (didsomething)  I->pause ();

        didsomething = 0;
        while (scanmenu->getPtrResult ((const void **) &obj)) {
            ++didsomething;
            --howmany;
            if (!obj->is (obj::known_infected)) {
                obj->set (obj::known_infected);
                if (obj->is (obj::infected)) {
                    disk->set (obj::known_bugginess); /* Behavior gives it away. */
                    if (disk->isOptimized ()) {
                        obj->clear (obj::infected);
                        if (obj->isA (kObjComputerVirus))
                            obj->mIlkId = kObjBlankDisk;
                        I->p ("Virus found and removed!");
                    } else {
                        I->p ("Virus found!");
                    }
                } else
                    I->p ("No threats found.");
                obj->announce ();
            } else if (obj->is (obj::infected)) {
                if (disk->isBuggy ()) {
                    disk->set (obj::known_bugginess);
                    I->p ("Unable to remove threat - program corrupt.");
                } else {
                    obj->clear (obj::infected);
                    if (obj->isA (kObjComputerVirus))
                        obj->mIlkId = kObjBlankDisk;
                    I->p ("Virus removed.");
                    obj->announce ();
                }
            } else
                I->p ("doAntiVirus: Path thought impossible reached?");
        }
        if (didsomething and howmany > 0) {
            prompt = "What do you want to scan next?";
        } else { /* Player wants to leave some things unscanned. */
            break;
        }
    }
    return kNormal;
}

static shExecResult
doResidentAntiVirus (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (computer->is (obj::infected)) {
        computer->clear (obj::infected);
        computer->set (obj::known_infected);
        I->p ("%s seems to be working much better.", YOUR (computer));
        return kNormal;
    }

    if (Hero.cr ()->is (kConfused)) {
        shObjectVector v;
        shObject *obj;
        int cnt = 0;
        /* This may also pick up any additional infected computers. */
        selectObjectsByFlag (&v, Hero.cr ()->mInventory, obj::infected);
        for (int i = 0; i < v.count (); ++i) {
            obj = v.get (i);
            if (!obj->isA (kFloppyDisk))  continue;
            ++cnt;
            if (!disk->isBuggy ())  obj->set (obj::known_infected);
            if (disk->isOptimized ()) {
                obj->clear (obj::infected);
                if (disk->mIlkId == kObjComputerVirus) {
                    obj->mIlkId = kObjBlankDisk;
                }
            }
        }
        I->p ("Found %s%d infected items.",
            disk->isOptimized () ? "and recovered " :
               !disk->isBuggy () ? "and marked "    : "", cnt);
    } else if (!computer->is (obj::fooproof)) {
        I->p ("Installing antivirus.");
        computer->set (obj::fooproof | obj::known_infected | obj::known_fooproof);
    } else {
        I->p ("You verify that %s is protected.", YOUR (computer));
        computer->set (obj::known_infected | obj::known_fooproof);
    }
    return kNormal;
}

#define MAXDRIVES 6
static int
countFloppyDrives (shObject *computer)
{
    switch (computer->mIlkId) {
    case kObjMiniComputer: return 1;
    case kObjMegaComputer: return MAXDRIVES;
    case kObjBioComputer: return 3;
    case kObjSatCom: return 2;
    default: return 0;
    }
}

/* Returns the blanked disk. */
shObject *
reformat (shObject *disk)
{
    assert (disk);
    disk->mIlkId = kObjBlankDisk;
    disk->clear (obj::cracked);
    disk->clear (obj::infected);
    disk->set (obj::known_infected);
    return disk;
}

/* Found idea for this from Cyrus' scratchpad and there it was called
   floppy disk of reformat.  I went with it instead of more staightforward
   floppy disk of formatting. -- MB */
static shExecResult
doReformat (shObject *computer, shObject *disk)
{/* TODO: When infected the result may be sometimes a corrupt disk instead
          of a blank one.  Unless it was already corrupt to begin with. */
    int drives = countFloppyDrives (computer);
    int chosen;
    char drivechar;
    shObject *inDrive[MAXDRIVES];
    shObject *obj;
    shObjectVector v;

    if (!disk->is (obj::known_type)) {
        if (disk->is (obj::known_appearance) and
            strstr (disk->myIlk ()->mAppearance.mName, "FORMAT C:"))
            I->p ("Hey, the label didn't lie!");
        else
            I->p ("This is a reformat program.");
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        I->pause ();
    }
    disk->set (obj::known_bugginess);

    inDrive[0] = disk;
    for (int i = 1; i < drives; ++i)
        inDrive[i] = NULL;

    if (disk->isBuggy ()) { /* Acts immediately. */
        chosen = RNG (drives + 1);
        drivechar = 'A' + chosen;
        debug.log ("buggy reformat phase");
        /* Place disk at C for single drive computers. */
        if (chosen == drives and drives == 1)
            ++drivechar;
        /* Picked empty floppy drive. */
        if (chosen != drives and inDrive[chosen] == NULL) {
            int retry = 1;
            do { /* TODO: Use dropResult to avoid recreating the menu. */
                int choice;
                shMenu *DOSJoke = I->newMenu ("Abort, retry or fail?", 0);
                DOSJoke->addIntItem ('a', "abort", 'a', 1);
                DOSJoke->addIntItem ('r', "retry", 'r', 1);
                DOSJoke->addIntItem ('f', "fail",  'f', 1);
                /* First message below should not end with a dot.
                   MS-DOS did not have one there. -- MB */
                I->p ("Not ready reading drive %c", drivechar);
                I->p ("Abort, Retry, Fail?");
                I->pause ();
                /* Forced choice. Not picking anything will ask again. */
                DOSJoke->getIntResult (&choice);
                delete DOSJoke;
                if (choice == 'a') {
                    return kDestruct; /* Abort meant lost work. */
                } else if (choice == 'f') {
                    retry = 0;
                }
            } while (retry);
        } else {
            I->p ("Reformat complete.");
            if (RNG (2)) {
                disk = Hero.cr ()->removeOneObjectFromInventory (disk);
                reformat (disk);
                Hero.cr ()->addObjectToInventory (disk);
                debug.log ("disk picked itself");
                /* No blow up since disk is now blank.  No eat either since
                   original disk pointer is now invalid. */
                /* TODO: use double pointer or reference. */
                return kGoneAlready;
            } else {
                debug.log ("disk picked computer's HDD");
                computer->mEnhancement = kNoSys;
                return kNormal;
            }
        }
    }

    debug.log ("nonbuggy reformat phase");
    if (disk->is (obj::infected) and !disk->is (obj::known_infected)) { /* Free hint. */
        I->p ("Warning: failed CRC data check.");
        disk->set (obj::known_infected);
    }
    int count, choices;
    char prompt[50];
    if (disk->isOptimized ()) {
        choices = drives - 1;
    } else {
        choices = mini(1, drives - 1);
    }
    snprintf (prompt, 50, "You may choose %d floppy disk%s for reformatting:",
        choices, (choices > 1) ? "s" : "");

    shMenu *disks = I->newMenu (prompt,
        (choices > 1) ? shMenu::kMultiPick | shMenu::kCountAllowed : 0);
    if (choices != 0) { /* No need to build the menu if not going to ask. */
        debug.log ("collect reformatable floppies phase");
        int added = 0;
        selectObjects (&v, Hero.cr ()->mInventory, kFloppyDisk);
        for (int i = 0; i < v.count (); ++i) {
            obj = v.get (i);
            /* Original reformat disk is already in, so ... */
            if (obj == disk) {
                if (obj->mCount != 1) {
                    disks->addPtrItem (obj->mLetter, obj->inv (), obj,
                        obj->mCount - 1);
                    ++added;
                }
            } else {
                disks->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
                ++added;
            }
        }
        if (!added) {   /* Hero only posesses reformat disk. */
            choices = 0;
        }
    }

    snprintf (prompt, 50, "Select drive%s to be reformatted:",
        (choices > 1) ? "s" : "" );
    shMenu *drivesToFormat = I->newMenu (prompt,
        disk->isOptimized() ? shMenu::kMultiPick : 0);

    int i = 0;
    drivechar = 'a';
    debug.log ("assign floppy drives phase");
    while (choices != 0 and disks->getPtrResult ((const void**) &obj, &count)) {
        if (count == 0) {
            return kNormal;
        }
        /* For multiple reformat all drives with disks except the
         * first are preselected. For single reformat no preselection.
         * Note that while blind reformat disk may not be in first (unselected)
         * drive and get formatted if player accepts without looking.
         */
        while (count and i < drives) {
            if (inDrive[i] != NULL) {   /* At most one to skip. */
                debug.log ("drive[%d]: %p", i, disk);
                ++i;
            }
            --count;
            if (choices == 1) { /* No point in filling remaining drives. */
                count = 0;
            }
            debug.log ("drive[%d]: %p", i, obj);
            inDrive[i++] = obj;
        }
    }
    delete disks;
    if (choices == 0) {   /* Stick reformat disk in the only floppy drive. */
        debug.log ("drive[%d]: %p", i, disk);
        inDrive[i++] = disk;
    }

    /* Blind people usually stick floppies in drives at random. */
    if ((Hero.cr ()->intr (kBlind) and RNG(3)) or Hero.cr ()->is (kConfused)) {
        debug.log ("shuffle floppies in drives phase");
        shuffle (inDrive, drives, sizeof(shObject *));
    }

    debug.log ("build floppy drive menu phase");
    for (int i = 0; i < drives; ++i) {
        if (inDrive[i]) {
            obj = inDrive[i];
            drivesToFormat->addPtrItem (drivechar++,
                obj->getDescription (), obj, 1, i > 0 and choices != 1);
        } else {
            drivesToFormat->addPtrItem (drivechar++, "empty", NULL, 1, 0);
        }
    } /* HDD letter is always one letter futher apart from floppy drives. */
    drivesToFormat->addPtrItem (++drivechar, "hard disk", NULL, -1, 0);

    debug.log ("reformat floppies phase");
    int reformatDiskReformatted = 0;
    int blankDisksCount = 0;
    shObject *blankDisks[MAXDRIVES];
    while (drivesToFormat->getPtrResult ((const void**) &obj, &count)) {
        if (count == -1) {
            computer->mEnhancement = kNoSys;
        } else if (obj != NULL) {
            if (obj == disk) {
                reformatDiskReformatted = 1;
                debug.log ("reformat reformats itself");
            }
            debug.log ("formatting %p", obj);
            obj = Hero.cr ()->removeOneObjectFromInventory (obj);
            blankDisks[blankDisksCount++] = reformat (obj);
        }
    }
    delete drivesToFormat;

    I->p ("Reformat complete.");
    /* Since reformat routine operates on multiple floppy disks it must
       handle disk eating by buggy computers itself.  Only disks acutally
       reformatted and original disks may be devoured. */
    if (computer->isBuggy ()) {
        int eatHowMany = RNG (-blankDisksCount, +blankDisksCount);
        debug.log ("devour floppies phase: eat %d of %d",
            eatHowMany, blankDisksCount);
        for (int i = 0; i < eatHowMany; ++i) {
            if (blankDisks[i] == disk) {
                /* Just a way to say no eating original disk since it
                   is already gone.  FIXME: pick better variable name */
                reformatDiskReformatted = 1;
            }   /* Crunch. Crunch. Crunch. */
            debug.log ("devouring %p", blankDisks[i]);
            Hero.cr ()->useUpOneObjectFromInventory (blankDisks[i]);
        }
        if (eatHowMany > 0) {    /* Oh no! */
            if (!Hero.cr ()->intr (kBlind)) {
                I->p ("Your computer ate %d of inserted floppy disks!",
                    eatHowMany);
            } else {
                I->p ("You listen to a %s om-nom-nom sound%c",
                    /* This compurer could not eat more? */
                    ((eatHowMany == drives) ?
                    /* No computer could eat more?! */
                        ((drives == MAXDRIVES)  ?
                            "terrible" : "scary") : "suspicious"),
                    ((eatHowMany == drives) ? '!' : '.'));
            }
        } /* Give back the rest.  Watch out not to start at negative index. */
        for (int i = maxi(0, eatHowMany); i < blankDisksCount; ++i) {
            Hero.cr ()->addObjectToInventory (blankDisks[i], 0);
        }
    } else {
        for (int i = 0; i < blankDisksCount; ++i) {
            blankDisks[i]->set (obj::known_type);
            Hero.cr ()->addObjectToInventory (blankDisks[i], 0);
        }
        if (blankDisksCount and !Hero.cr ()->intr (kBlind))
            AllIlks[kObjBlankDisk].mFlags |= kIdentified;
    }
    debug.log ("reformat returns");
    return reformatDiskReformatted ? kGoneAlready : kNoEat;
}


static shExecResult
doComputerVirus (shObject *computer, shObject *disk)
{
    disk->set (obj::infected); /* Paranoia.  This disk should be infected already. */
    if (computer->is (obj::fooproof)) {
        /* No message.  Antivirus already has printed one. */
        return kDestruct; /* You could have reformatted it.  Too late. */
    } else {
        disk->set (obj::known_type);
        disk->setIlkKnown ();
        return kFakeExpiry;
    }
}

/* To check whether a Smart-Disc can fly back. */
extern int calcShortestPath (int, int, int, int, shMonId, shDirection [], int);

static void
putAwayDiscs (shObjectVector *v)
{
    for (int i = 0; i < v->count (); ++i) {
        shObject *obj = v->get (i);
        Level->putObject (obj, obj->mX, obj->mY);
    }
}

static shExecResult
doRecallSmartDisc (shObject *, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (disk->isBuggy ()) {
        I->p ("Software error.  Recall failed.");
        disk->set (obj::known_bugginess);
        return kNoExpiry;
    }
    /* Find all non-buggy Smart-Discs on floor.  Buggy discs do not respond. */
    const int MAXPATH = 30;
    shObjectVector discs;
    for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
        for (int y = 0; y < MAPMAXROWS; ++y) {
            shObjectVector *v = Level->getObjects (x, y);
            if (v and v->count ()) {
                for (int i = v->count () - 1; i >= 0; --i) {
                    shObject *obj = v->get (i);
                    if (obj->isA (kObjSmartDisc) and !obj->isBuggy ()) {
                        v->remove (obj);
                        discs.add (obj);
                    }
                }
            }
        }
    }
    int n = discs.count ();
    if (n == 0) {
        I->p ("No Smart-Discs present.");
        return kNoExpiry;
    }
    shDirection buf[MAXPATH];
    /* Check distances for discs. */
    int *dist = new int[n];
    int reachable = 0;
    for (int i = 0; i < n; ++i) {
        shObject *disc = discs.get (i);
        /* Kludge: kMonFuelBarrel means no flying past closed doors. */
        dist[i] = calcShortestPath (disc->mX, disc->mY, Hero.cr ()->mX, Hero.cr ()->mY,
                                    kMonFuelBarrel, buf, MAXPATH);
        if (dist[i])  ++reachable;
    }
    if (!reachable) {
        if (n > 1 and disk->isOptimized ()) {
            I->p ("All of %d Smart-Discs are unreachable.", n);
        } else {
            I->p ("No reachable Smart-Discs present.");
        }
        delete [] dist;
        putAwayDiscs (&discs);
        return kNoExpiry;
    }
    /* Find disc with shortest path to fly. */
    int closest = 0;
    for (int i = 1; i < n; ++i) { /* Dist[x] == 0 means no path exists. */
        if (dist[i] and dist[i] < dist[closest])
            closest = i;
    }
    shObject *disc = discs.get (closest);
    /* Get the disc. */
    if (Hero.cr ()->addObjectToInventory (disc, 1)) {
        if (!Hero.cr ()->mWeapon) {
            Hero.cr ()->wield (disc, 1);
        }
        disc->announce ();
    } else {
        if (!Hero.cr ()->intr (kBlind)) {
            I->p ("%s lands at your feet.", disc->an ());
        } else {
            I->p ("You hear a thud near your feet.");
        }
        Level->putObject (disc, Hero.cr ()->mX, Hero.cr ()->mY);
    }
    if (n > 1 and disk->isOptimized ()) {
        I->p ("There are %d more Smart-Disc%s.  %d of those are reachable.",
              n-1, n-1 > 1 ? "s" : "", reachable-1);
    }
    discs.remove (disc);
    putAwayDiscs (&discs);
    delete [] dist;
    return kNoExpiry;
}


/* Returns true if entity succeeded at defense. */
static bool
softwareResistRoll (shCreature *entity, int skill)
{
    if (RNG (1, 100) + skill > entity->mCLevel * 5)
        return false;
    return true;
}


static shExecResult
doRecursiveLoop (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (disk->isOptimized ()) { /* Loop optimized out. */
        I->p ("Program exited successfully.");
        /* When other programs with such message appear change this. */
        return kNormal;
    }
    I->p ("A signal is being broadcast.");
    for (int i = 0; i < Level->mCrList.count (); ++i) {
        shCreature *c = Level->mCrList.get (i);
        if (c->mState == kDead)  continue;
        if (!c->isRobot () and !c->isProgram ())  continue;
        /* Line of sight does not give the same results usual field
           of view computation does so if the latter is not available
           due to hero being blind there are going to be artifacts.
           Not using existsLOS if possible reduces their number. */
        if ((!Hero.cr ()->intr (kBlind) and Hero.cr ()->canSee (c->mX, c->mY)) or
            (Hero.cr ()->intr (kBlind) and
             Level->existsLOS (Hero.cr ()->mX, Hero.cr ()->mY, c->mX, c->mY)))
        {
            c->newEnemy (Hero.cr ());
            int s = Hero.cr ()->getSkill (kHacking)->getValue ();
            if (softwareResistRoll (c, s)) { /* Attack succeeded. */
                if (Hero.cr ()->canSee (c))
                    I->p ("%s freezes in place!", THE (c));
                else
                    I->p ("%s freezes for a moment.", THE (c));
                c->inflict (kAsleep, FULLTURN * NDX (3, 3));
            }
        }
    }

    if (disk->isBuggy () or disk->is (obj::infected)) {
        I->p ("%s stops responding.", YOUR (computer));
        if (!RNG (4))  /* Drop a hint. */
            I->p ("Great.  Now you'll have to shake it to make it reboot.");
        computer->set (obj::toggled);
        if (disk->is (obj::infected) and
            disk->is (obj::known_bugginess) and !disk->isBuggy ())
        {
            disk->set (obj::known_infected);
        } else if (disk->is (obj::known_infected) and !disk->is (obj::infected)) {
            disk->set (obj::known_bugginess);
        }
        /* Suffocate the bio computer? */
    }
    return kNormal;
}


static shExecResult
destructiveProc (shObject *computer, int procno)
{
    assert (procno >= 0 and procno <= 4);
    switch (procno) {
    case 0:
        I->p ("%s overheats!", YOUR (computer));
        if (computer->mEnhancement > kAbysmalSys)
            --computer->mEnhancement;
        break;
    case 1:
        I->p ("Sparks fly from %s!", YOUR (computer));
        computer->setBuggy ();
        break;
    case 2:
    {   /* Both cases 0 and 1 in random order. */
        int first = RNG (2);
        int second = 1 - first;
        (void) destructiveProc (computer, first);
        (void) destructiveProc (computer, second);
        break;
    }
    case 3:
        I->p ("%s blows up!", YOUR (computer));
        return kDestroyComputer;
    }
    return kNormal;
}

static shExecResult
doLogicBomb (shObject *computer, shObject *disk)
{
    disk->set (obj::known_type);
    disk->setIlkKnown ();
    if (disk->isBuggy () or disk->is (obj::infected)) {
        (void) destructiveProc (computer, 0);
        if (disk->is (obj::infected) and
            disk->is (obj::known_bugginess) and !disk->isBuggy ())
        {
            disk->set (obj::known_infected);
        } else if (disk->is (obj::known_infected) and !disk->is (obj::infected)) {
            disk->set (obj::known_bugginess);
        } else if (computer->is (obj::known_infected) and !computer->is (obj::infected) and
                   !disk->is (obj::infected))
        {   /* Rationale: if the computer did not become unstable but
               destructive procedure was called it cannot be infected
               and therefore must be buggy. */
            disk->set (obj::known_bugginess);
        }
        return kNormal;
    }

    bool cancel = false;
    int x = -1, y = -1;
    shCreature *c = NULL;
    if (I->getSquare ("Target a program or robot:  (select a location)",
                      &x, &y, 0))
    {
        c = Level->getCreature (x, y);
    } else {
        I->p ("Cancelled.");
        cancel = true;
    }
    if (!c) {
        I->p ("No entity found at specified location.");
        cancel = true;
    } else if (!c->isRobot () and !c->isProgram ()) {
        I->p ("Entity cannot receive binary data.");
        cancel = true;
    } else {
        int skill = Hero.cr ()->getSkill (kHacking)->getValue ();
        int dmg = RNG (12, 20) + (skill ? RNG (1, skill) : 0);
        int stun = RNG (2, 6);
        if (disk->isOptimized ()) {
            dmg += 3;
            stun += 2;
        }
        if (c->isRobot ()) { /* Hardware grants some resistance. */
            dmg = dmg * 2 / 3;
            stun = stun * 2 / 3;
        }
        if (c->mHP <= dmg) {
            if (Hero.cr ()->canSee (c)) {
                c->pDeathMessage (THE (c), kSlain);
            }
            c->die (kSlain);
        } else {
            c->mHP -= dmg;
            int s = Hero.cr ()->getSkill (kHacking)->getValue ();
            if (!softwareResistRoll (c, s)) {
                c->inflict (kStunned, stun * FULLTURN);
                if (Hero.cr ()->canSee (c))
                    I->p ("Systems of %s malfunction.", THE (c));
            }
        }
    }

    if (cancel) {
        I->p ("Execution halted.");
        return kNormal;
    } else if (!disk->is (obj::cracked)) {
        return kDestruct;
    } else {
        int blowup = disk->is (obj::known_cracked);
        disk->set (obj::known_cracked);
        return destructiveProc (computer, RNG (3 + blowup));
    }
}


static void
attractLawyers (shCreature *pirate)
{
    int cnt = 0;

    for (int i = 0; i < Level->mCrList.count () ; ++i) {
        shCreature *c = Level->mCrList.get (i);
        if (c->isA (kMonLawyer)) {
            if (c->canSee (pirate)) {
                c->makeAngry ();
            } else if (cnt < 3) {
                int x = pirate->mX, y = pirate->mY;
                if (!Level->findAdjacentUnoccupiedSquare (&x, &y)) {
                    c->transport (x, y);
                    c->cure (kFleeing);
                    ++cnt;
                }
            }
        }
    }

    if (!cnt and !RNG (13)) {
        int x = pirate->mX, y = pirate->mY;
        if (!Level->findAdjacentUnoccupiedSquare (&x, &y)) {
            shCreature *lawyer = shCreature::monster (kMonLawyer);
            if (!Level->putCreature (lawyer, x, y)) {
                ++cnt;
            }
        }
    }

    if (!cnt)  return;
    I->p ("You seem to have attracted the attention of %s lawyer%s.",
          cnt > 1 ? "some" : "a", cnt > 1 ? "s" : "");
}


/* Big enough to get own file.  MatterCompiler.cpp */
extern shExecResult doMatterCompiler (shObject *computer, shObject *disk);

/* Returns number of ms taken. */
static int
executeFloppyDisk (shCreature *user, shObject *computer, shObject *disk)
{
    shObjectIlk *ilk = disk->myIlk ();
    shCompInterface interface = checkInterface (computer);
    int sys = computer->mEnhancement;
    shExecResult result;
    int summonlawyer = 0;
    int mishandle = 0;  /* User confused and system got fooled. */

    if (!computer->hasSystem ()) {
        if (disk->mIlkId == kObjOperatingSystem) {
            disk->set (obj::known_type);
            disk->setIlkKnown ();
            if (user->intr (kBlind)) {
                I->p ("The seeking sound continues.  "
                      "This disk must be bootable.");
            } else {
                I->p ("Booting from floppy.....success!");
            }
            doInstallRawSystem(computer, disk, !user->intr (kBlind));
            if (disk->is (obj::infected)) {
                computer->set (obj::infected); /* Gratis. */
            }
            /* Installation can be a lenghty process. */
            return computer->execTime () * 2;
        }
        return HALFTURN;   /* Not a system disk. */
    }

    if (user->intr (kBlind)) {
        /* Blind and mute. */
        if (Hero.isMute ()) {
            if (interface == kIUniversal) {
                int felt_dirty = Hero.getStoryFlag ("hypoderm");
                if (felt_dirty < 3) {
                    I->p ("You let in your bio computer's hypodermic nerve interface.");
                    I->p ("You feel dirty.");
                    Hero.setStoryFlag ("hypoderm", felt_dirty + 1);
                }
            } else {
                I->p ("But you're blind and mute!");
                return 0;
            }
        /* Blindness only. */
        } else if (interface >= kIVisualVoice) {
            I->p ("You make use of %s's voice interface.", YOUR (computer));
        } else {
            I->p ("But you're blind, and this computer doesn't "
                  "have a voice interface.");
            return 0;
        }
    }

    if (computer->is (obj::toggled)) {
        I->p ("%s is not responding.", YOUR (computer));
        return HALFTURN;
    }

    /* If we entered here while held, the computer must be bio computer. */
    if (user->mTrapped.mWebbed or user->mTrapped.mTaped)
        I->p ("At your command %s snatches %s.", YOUR (computer),
            disk->theQuick (1));

    /* Inspired by Vista OS. */
    if (computer->isUpdateTime ()) {
        computer->systemUpdateMessage ();
        return FULLTURN;
    }

    if (!computer->systemCheckDisk (disk))
        return computer->execTime () / 2;

    /* Invoke secondary disk effect unless system mishandles this. */
    if (user->is (kConfused) and disk->mIlkId != kObjBlankDisk) {
        confusedComputingMessage ();
        /* abysmal: 50% terrible: 33.(3)% bad: 25% */
        if (sys < kPoorSys and !RNG (1, sys + 5)) {
            I->p ("The computer fails to correct your errors.");
            mishandle = 1;
        }
    }

    /* If we are here hero managed to run the stuff. */

    if (user->isInShop ()) {
        user->usedUpItem (disk, 1, "use");
        disk->clear (obj::unpaid);
    }

    /* Save infectedness because inserted disk may be gone after execution. */
    int disk_infected = disk->is (obj::infected);

    if (!mishandle) {
        if (!disk->is (obj::known_type) and
            strstr (disk->myIlk ()->mAppearance.mName, "HAS README") and
            I->yn ("This program comes bundled with README.TXT.  Read it now?"))
        {
            disk->set (obj::known_type);
            disk->setIlkKnown ();
            I->p ("You learn this is %s.", disk->theQuick ());
            /* You take time to read it whole. */
            return computer->execTime () + 3 * FULLTURN;
        } else
            result = ilk->mRunFunc (computer, disk);
    } else {
        result = kNormal;
    }

    if (disk_infected and !computer->is (obj::fooproof)) {
        if (!computer->is (obj::infected)) {
            computer->set (obj::infected);
            if (result != kGoneAlready)
                disk->set (obj::known_infected);
            I->p ("You notice unusual system unstability.");
        }
    } else if (result != kGoneAlready and
        !disk->is (obj::infected) and computer->is (obj::infected) and
        disk->mIlkId != kObjAntivirusDisk and
        disk->mIlkId != kObjResidentAntivirusDisk and
        disk->mIlkId != kObjBlankDisk)
    {
        disk->set (obj::infected);
    }

    if (result == kDestroyComputer) {
        int ms = computer->execTime ();
        user->useUpOneObjectFromInventory (disk);
        user->useUpOneObjectFromInventory (computer);
        return ms;
    }

    if (result != kGoneAlready) {
        /* Check for police awareness of using cracked stuff. */
        if (result != kNoExpiry and disk->is (obj::cracked)) {
            int danger = Hero.getStoryFlag ("software piracy");
            if (0 == danger) {
                Hero.setStoryFlag ("software piracy", 5);
            } else if (RNG (0, danger) and RNG (0, danger)) {
                summonlawyer = 1;
            }
            if (!disk->is (obj::known_cracked)) {
                I->p ("The licence for this software has been cracked!");
                disk->set (obj::known_cracked);
            }
        }

        if (result == kDestruct) {
            I->p ("The floppy disk self destructs!");
            Hero.cr ()->useUpOneObjectFromInventory (disk);
            disk = NULL;
        } else if (result != kNoExpiry and
                   !disk->is (obj::cracked) and
                   (!RNG (3) or result == kFakeExpiry))
        {
            I->p ("Your license for this software has expired.");
            if (disk->isA (kObjReformatDisk)) {
                I->p ("The floppy disk reformats self!");
                disk = user->removeOneObjectFromInventory (disk);
                reformat (disk);
                user->addObjectToInventory (disk);
            } else {
                I->p ("The floppy disk self destructs!");
                user->useUpOneObjectFromInventory (disk);
            }
            disk = NULL;
        } else if (computer->isBuggy () and result != kNoEat and !RNG (5)) {
            if (Hero.cr ()->intr (kBlind))
                I->p ("You hear your computer go nyam-nyam.");
            else
                I->p ("Your computer eats the disk!");
            Hero.cr ()->useUpOneObjectFromInventory (disk);
            disk = NULL;
            computer->set (obj::known_bugginess);
        }
    } else {
        disk = NULL;
    }

    if (user->isHero () and summonlawyer)  attractLawyers (user);

    /* A bit of historical note.  This is here instead of various floppy disk
       execution routines which may affect inventory because the reorganization
       might cause the used disk to be merged with an item that recently
       changed status and result in double delete by useUpObject and post-merge
       routines. */
    Hero.cr ()->reorganizeInventory ();

    return computer->execTime ();
}

int
shObject::executeProgram (shObject *media)
{   /* Caller ought to check for is (kUnableCompute). */
    Hero.cr ()->employedItem (this, 20);
    /* So far only floppies. */
    return executeFloppyDisk (Hero.cr (), this, media);
}

void
initializeFloppyDisks ()
{
    trn::initialize ();

    AllIlks[kObjBugDetectionDisk].mRunFunc = doDetectBugs;
    AllIlks[kObjIdentifyDisk].mRunFunc = doIdentify;
    AllIlks[kObjSpamDisk].mRunFunc = doSpam;
    AllIlks[kObjCorruptDisk].mRunFunc = doCorrupt;
    AllIlks[kObjBlankDisk].mRunFunc = doBlank;
    AllIlks[kObjObjectDetectionDisk].mRunFunc = doDetectObject;
    AllIlks[kObjLifeformDetectionDisk].mRunFunc = doDetectLife;
    AllIlks[kObjMappingDisk].mRunFunc = doMapping;
    AllIlks[kObjDiagnosticsDisk].mRunFunc = doDiagnostics;
    AllIlks[kObjWarpTracerDisk].mRunFunc = doWarpTrace;
    AllIlks[kObjLogicBombDisk].mRunFunc = doLogicBomb;
    AllIlks[kObjDebuggerDisk].mRunFunc = doDebug;
    AllIlks[kObjEnhanceArmorDisk].mRunFunc = doEnhanceArmor;
    AllIlks[kObjEnhanceWeaponDisk].mRunFunc = doEnhanceWeapon;
    AllIlks[kObjEnhanceImplantDisk].mRunFunc = doEnhanceImplant;
    AllIlks[kObjHypnosisDisk].mRunFunc = doHypnosis;
    AllIlks[kObjTransportDisk].mRunFunc = doTransport;
    AllIlks[kObjRecursiveLoopDisk].mRunFunc = doRecursiveLoop;
    AllIlks[kObjAntivirusDisk].mRunFunc = doAntiVirus;
    AllIlks[kObjReformatDisk].mRunFunc = doReformat;
    AllIlks[kObjComputerVirus].mRunFunc = doComputerVirus;
    AllIlks[kObjRecallDisk].mRunFunc = doRecallSmartDisc;
    AllIlks[kObjOperatingSystem].mRunFunc = doInstallSystem;
    AllIlks[kObjHackingDisk].mRunFunc = doHacking;
    AllIlks[kObjResidentAntivirusDisk].mRunFunc = doResidentAntiVirus;
    AllIlks[kObjMatterCompiler].mRunFunc = doMatterCompiler;
}
