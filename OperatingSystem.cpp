#include "Interface.h"
#include "Hero.h"
#include "Object.h"

bool
shObject::isComputer (void)
{
    return has_subtype (computer);
}

int
shObject::execTime (void)
{   /* Optimized computers execute programs faster. */
    return isOptimized () ? FULLTURN : LONGTURN;
}

int
shObject::hackingModifier (void)
{
    return hasSystem () ? mEnhancement : 0;
}

bool
shObject::hasSystem (void)
{
    if (!isComputer ()) {
        return false;
    }
    if (mEnhancement != kNoSys)
        return true;
    else {
        if (!Hero.cr ()->intr (kBlind)) {
            I->p ("DISK BOOT FAILURE.  INSERT SYSTEM DISK AND PRESS ENTER.");
            set (obj::known_enhancement);
            I->pause ();
            I->p ("Your computer lights floppy drive LEDs for a moment.");
        } else
            I->p ("You hear floppy drives seeking briefly.");
        return false;
    }
}

bool
shObject::systemRunPOST (void)
{
    if (mEnhancement >= kGoodSys) {
        if (!is (obj::known_bugginess)) {
            if (isBuggy ())
                I->p ("Warning!  POST detected hardware malfunction.");
            else if (isOptimized ())
                I->p ("POST running... high performance settings active.");
            else
                I->p ("POST running... all tests passed.");
            set (obj::known_bugginess);
            if (isBuggy ())
                return I->yn ("Cancel execution?");
        }
    }
    return true;
}

/* Returns true if system check passed and false otherwise. */
bool
shObject::systemCheckDisk (shObject *disk)
{
    int sys = mEnhancement;
    if (sys >= kGoodSys and !disk->is (obj::known_type) and
        (disk->isA (kObjSpamDisk) or disk->isA (kObjHypnosisDisk)))
    {
        I->p ("Program has been recognized as fraudulent software.");
        disk->set (obj::known_type);
        disk->announce ();
        int ans = I->yn ("Run anyway?");
        if (ans == 0)
            return false;
    }

    /* Does not tell debugged from optimized.  That would be too good. */
    if (sys >= kExcellentSys and
        !disk->is (obj::known_bugginess) and disk->isBuggy ())
    {
        I->p ("Software has not passed reliability tests.");
        disk->set (obj::known_bugginess);
        disk->announce ();
        int ans = I->yn ("Continue?");
        if (ans == 0)
            return false;
    }

    /* This one does. */
    if (sys >= kSuperbSys and !disk->is (obj::known_bugginess))
    {
        disk->set (obj::known_bugginess);
        disk->announce ();
        int ans = I->yn ("Continue?");
        if (ans == 0)
            return false;
    }

    /* Antivirus in computer can recover infected floppies. */
    if (is (obj::fooproof) and disk->is (obj::infected)) {
        I->p ("Antivirus alert: trojan Zap'M-OverwriteDisk found.");
        disk->set (obj::known_infected);
        if (disk->isA (kObjComputerVirus)) {
            disk->set (obj::known_type);
        }
        disk->announce ();
        if (I->yn ("Recover floppy disk?", disk->mCount > 1 ? "s" : "")) {
            disk = Hero.cr ()->removeOneObjectFromInventory (disk);
            disk->clear (obj::infected);
            if (disk->isA (kObjComputerVirus)) {
                disk->mIlkId = kObjBlankDisk;
            }
            Hero.cr ()->addObjectToInventory (disk, 0);
            return 0; /* Disinfection should take time. */
        } else { /* No point in disallowing that. */
            int ans = I->yn ("Run anyway?  (computer will not get infected)");
            if (ans == 0)
                return false;
        }
    } else if (is (obj::fooproof) and is (obj::known_fooproof)) {
        /* You know the computer is protected by antivirus?
           So you know the floppy disk has been scanned. */
        disk->set (obj::known_infected); /* Mark clean. */
    }

    return true; /* Nothing suspicious or warnings ignored. */
}

/* In each five minutes system will reserve 30 seconds for updates. */
#define INTERVAL (300 * FULLTURN)
#define UPDATES 30
static int
updatesToDo (shObject *obj)
{
    long int time_phase = (Clock                % INTERVAL) / FULLTURN;
    long int comp_phase = (obj->mLastEnergyBill % INTERVAL) / FULLTURN;
    int updates = time_phase + UPDATES - comp_phase;
    if (updates > UPDATES or updates <= 0)
        return 0;
    return updates;
}

void
shObject::kickComputer (void)
{   /* Kick it into future somewhat. */
    mLastEnergyBill += RNG (100, 200) * FULLTURN;
    clear (obj::toggled); /* Break recursive loop. */
}

/* Returns true OS has decided to troll you right now. */
bool
shObject::isUpdateTime ()
{ /* Good system does not lock up your computer during updates. */
    if (!isComputer ())
        return false;
    if (mEnhancement <= kAverageSys and updatesToDo (this))
        return true;

    return false;
}

void
shObject::systemUpdateMessage (void)
{   /* Hint!  Hint!  X-D */
    I->p ("Do not turn off or kick your computer.  Loading update %d of %d.",
        updatesToDo (this), UPDATES);
}
#undef INTERVAL
#undef UPDATES
