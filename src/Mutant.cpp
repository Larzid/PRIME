/*******************************

        Mutant Power Code

how to add a mutant power:
1. edit shMutantPower enum in Global.h
2. edit the little table in Mutant.h, and make sure the order is consistent
   with the enum!
3. write a usePower function, or startPower/stopPower functions
4. possibly write shCreature::Power function if you want monsters to use it too
5. add the appropriate skill to the Psion and maybe other character classes

********************************/

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Interface.h"
#include "Mutant.h"

bool
shMutantPowerData::isPersistent (void)
{
    return !!mOffFunc;
}

shSkillCode
shMutantPowerData::getCode (void)
{   /* The mutant power skill code is composed of three parts.
        - Faculty relied upon (first four bits)
        - Skill type (second four bits)
        - Ability used (third four bits)
    */
    /* Skill type is simplest - it is a Mutant Power. */
    int code = kMutantPower;
    /* Add used ability from powers table. */
    code |= mAbility << 8;
    /* Extract faculty number and add it. */
    code |= mSkill & kSkillNumberMask;
    return shSkillCode (code);
}

#define MUTPOWER_CURSOR 3 /* Tile number in kRowCursor for targeting powers. */
#define CANCEL_POWER -1 /* Value power functions return on abort by player. */

static int
shortRange (shCreature *c)
{
    return 5 + c->mCLevel / 4;
}

/* Returns: 1 if defender has better torc otherwise 0. */
static int /* Prints message if psionic attack was deflected. */
torcComparison (shCreature *attacker, shCreature *defender)
{
    int result = attacker->mImplants[shObjectIlk::kNeck] and
                 defender->mImplants[shObjectIlk::kNeck] and
    /* When more neck items appear remember to add isA (kObjTorc) check. */
                 defender->mImplants[shObjectIlk::kNeck]->mBugginess >
                 attacker->mImplants[shObjectIlk::kNeck]->mBugginess;
    if (result) {
        if (attacker->isHero ()) {
            I->p ("%s prevents you from making a psionic attack against %s.",
                YOUR (attacker->mImplants[shObjectIlk::kNeck]), THE (defender));
        } else if (defender->isHero ()) {
            I->p ("Circuitry of %s prevents a psionic assault against you.",
                YOUR (defender->mImplants[shObjectIlk::kNeck]));
            //TODO: reveal creature
            //Level->drawSqCreature (attacker->mX, attacker->mY);
        }
    }
    return result;
}

int
shCreature::telepathy (int on)
{
    mInnateIntrinsics.mod (kTelepathy, on ? +6 : -6);
    computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::tremorsense (int on)
{
    mInnateIntrinsics.mod (kTremorsense, on ? +9 : -9);
    computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::hypnosis (shDirection dir)
{
    int x = mX;
    int y = mY;
    shCreature *target;
    shAttack *gaze = &Attacks[kAttHypnoGaze];

    if (mLevel->moveForward (dir, &x, &y) and
        (target = mLevel->getCreature (x, y)))
    {
        if (torcComparison (this, target))  return HALFTURN;
        bool saved = !RNG (10);
        if (!saved) {
            int skill = getSkillModifier (kMutantPower, kHypnosis);
            gaze->mDamage[0].mLow = 2 + skill / 4;
            gaze->mDamage[0].mHigh = 10 + skill / 2;
        }

        if (target->isHero ()) {
            if (!target->canSee (this)) {
                /* You can't see the hypnotic gaze. */
            } else if (saved) {
                I->p ("You blink.");
            } else { /* Message handled by sufferDamage. */
                target->sufferDamage (kAttHypnoGaze);
            }
        } else {
            const char *t_buf;
            if (Hero.cr ()->canSee (target)) {
                t_buf = THE (target);
            } else {
                t_buf = "it";
            }
            if (!target->canSee (this)) {
                I->p ("%s doesn't seem to notice your gaze.", t_buf);
            } else if (!target->hasMind ()) {
                I->p ("%s is unaffected.", t_buf);
            } else if (saved) {
                I->p ("%s resists!", t_buf);
            } else {
                target->sufferDamage (kAttHypnoGaze);
            }
        }
    }
    return HALFTURN;
}

int
shCreature::shootWeb (shDirection dir)
{   /* In future, it should also not work while wearing gloves. */
    if (isUnderwater ()) {
        if (isHero ())  I->p ("Not while being submerged.");
        return 0;
    }
    /* Okay, we can do this.  Proceed. */
    const int elapsed = QUICKTURN;
    /* Get yourself out of pit almost for free card. */
    if (dir == kUp) {
        if (isHero ()) {  /* This use costs only one psi point. */
            mAbil.temp_mod (abil::Psi, 1); /* Refund one point. */
        }
        if (mTrapped.mInPit) {
            if (isHero ()) {
                I->p ("You lift yourself up with a web strand.");
            } else if (Hero.cr ()->canSee (this)) {
                shFeature *f = mLevel->getFeature (mX, mY);
                I->p ("%s shoots a web at the ceiling and gets out of %s.",
                    the (), f ? THE (f) : "something buggy");
            }
            mTrapped.mInPit = 0;
            mZ = 0;
        }
        return elapsed;
    }
    /* A directional shot at something. */
    int x, y, r;
    int maxrange = shortRange (this);
    int attackmod;

    int skill = getSkillModifier (kMutantPower, kShootWebs);

    shAttack *WebAttack = &Attacks[kAttShootWeb];
    WebAttack->mDamage[0].mLow  = 1 + mCLevel / 2 + skill / 2;
    WebAttack->mDamage[0].mHigh = 6 + mCLevel / 2 + skill;

    attackmod = mToHitModifier + skill;

    if (kUp == dir) {
        return elapsed;
    } else if (kDown == dir or kOrigin == dir) {
        resolveRangedAttack (WebAttack, NULL, attackmod, this);
        return elapsed;
    }

    x = mX;    y = mY;

    bool immediate = true; /* Is this first checked square? */
    while (maxrange--) {
        if (!Level->moveForward (dir, &x, &y)) {
            Level->moveForward (uTurn (dir), &x, &y);
            return elapsed;
        }

        if (Level->isOccupied (x, y)) {
            shCreature *c = Level->getCreature (x, y);
            /* May sail overhead unless web was shot from adjacent position. */
            if (c->mZ < 0 and RNG (2) and !immediate) {
                continue;
            }
            r = resolveRangedAttack (WebAttack, NULL, attackmod, c);
            if (r >= 0)
                return elapsed;
        }
        if (Level->isObstacle (x, y))  return elapsed;
        immediate = false;
    }

    return elapsed;
}

int
shCreature::opticBlast (shDirection dir)
{
    int elapsed = QUICKTURN;
    int skill = getSkillModifier (kMutantPower, kOpticBlast);

    shAttack *OpticBlast = &Attacks[kAttOpticBlast];
    OpticBlast->mDamage[0].mLow = 1 + skill + mCLevel;
    OpticBlast->mDamage[0].mHigh = 6 + skill * 4 + mCLevel;

    if (kUp == dir) {
        return elapsed;
    } else if (kDown == dir) {
        /* TODO: Affect items. */
        return elapsed;
    }

    int atkmod = skill + mToHitModifier;
    int res = Level->attackEffect (OpticBlast, NULL, mX, mY, dir, this, atkmod);
    return res < 0 ? res : elapsed;
}

int
shCreature::xRayVision (int on)
{
    assert (isHero ());
    if (on) {
        mInnateIntrinsics.mod (kXRayVision, +3);
        if (intr (kXRayVision) and
            intr (kPerilSensing) and
            mGoggles->is (obj::toggled))
        {
            I->p ("You can see through your darkened goggles now.");
        }
    } else {
        mInnateIntrinsics.mod (kXRayVision, -3);
        if (!intr (kXRayVision) and
            intr (kPerilSensing) and
            mGoggles->is (obj::toggled))
        {
            I->p ("You can no longer see through your darkened goggles.");
        }
    }
    computeIntrinsics ();
    return HALFTURN;
}

int
shCreature::mentalBlast (int x, int y)
{
    const int elapsed = SLOWTURN;
    shCreature *c = mLevel->getCreature (x, y);
    if (!c) {
        if (isHero ())
            I->p ("There seems to be no effect.");
        return elapsed;
    }

    bool shielded = c->intr (kBrainShielded);
    bool self = this == c;
    bool reveal = false; /* Reveal this to targeted creature? */

    const char *t_buf = THE (c);
    const char *an_buf = AN (c);

    /* Mindless and protected beings are completely immune. */
    if (!c->hasMind () or (shielded and !self)) {
        if (self and isHero ()) {
            I->p ("Impossible!  How can you blast self while having no mind?!");
            return elapsed;
        }
        if (shielded and c->isHero ()) {
            I->p ("Your scalp tingles.");
        } else if (Hero.cr ()->canSee (c)) {
            I->p ("%s is not affected.", t_buf);
        } else if (Hero.cr ()->canHearThoughts (c)) {
            I->p ("You sense no effect.");
        } else if (isHero ()) {
            I->p ("There seems to be no effect.");
        }
        return elapsed;
    }

    /* Next, a torc may block a mental blast before it even begins. */
    if (torcComparison (this, c)) {
        if (isHero ())
            I->p ("%s tingles briefly.", YOUR (mImplants[shObjectIlk::kNeck]));
        return elapsed;
    }

    /* Another chance to resist is the Excruciator. */
    if (c->hasInstalled (kObjExcruciator)) {
        /* Suffer backlash! */
        if (isHero ()) {
            I->p ("Upon contacting this mind you are subjected to great pains!");
        } else if (Hero.cr ()->canSee (this) or Hero.cr ()->canHearThoughts (this)) {
            I->p ("%s cringes in pain!", the ());
        }
        if (sufferDamage (kAttExcruciatorBacklash)) {
            char *buf = GetBuf ();
            snprintf (buf, SHBUFLEN, "Succumbed to Excruciator borne by %s",
                THE (c));
            die (kMisc, buf);
        }
        return elapsed;
    }

    /* Write message. */
    if (self) {
        I->p ("You blast yourself with psionic energy!");
    } else if (c->isHero ()) {
        I->p ("You are blasted by a wave of psionic energy!");
        reveal = true;
    } else {
        if (!canSee (c) and !Hero.cr ()->canHearThoughts (c)) {
            /* Make sure the creature is referred to by its actual name
               because by making contact with creature's mind hero now
               knows what it is.  For a brief moment at least.*/
            Level->setVisible (x, y, 1);
            an_buf = AN (c);
            t_buf = THE (c);
            Level->setVisible (x, y, 0);
            I->drawMem (mX, mY, this, NULL, NULL, NULL);
            I->refreshScreen ();
            I->p ("You blast %s with a wave of psionic energy.", an_buf);
        } else {
            I->p ("You blast %s with a wave of psionic energy.", t_buf);
        }
        c->newEnemy (this);
    }

    shAttack *atk = &Attacks[kAttMentalBlast];
    int skill = getSkillModifier (kMutantPower, kMentalBlast);
    int resist_mod = c->getSkillModifier (kConcentration) * 5;
    /* Psychic harm. */
    atk->mDamage[0].mLow = maxi (2, 2 + skill / 3);
    atk->mDamage[0].mHigh = maxi (8, skill * 3 / 2);
    /* Confusion. */
    atk->mDamage[1].mLow = maxi (0, 2 + skill / 4);
    atk->mDamage[1].mHigh = maxi (0, 6 + skill / 4);
    atk->mDamage[1].mChance = maxi (0, 60 + skill - resist_mod);

    /* Mental blast can serve as confusion on demand. */
    if (self)  atk->mDamage[1].mChance = 100;

    /* Ungooma protect from single mental blast. */
    if (c->hasInstalled (kObjUngooma)) {
        /* Track how many parasites 'c' has and where they are located. */
        shObjectIlk::Site ungooma_pos[NUMLOBES];
        int num_para = 0;
        FOR_ALL_LOBES (l) {
            if (c->mImplants[l] and c->mImplants[l]->isA (kObjUngooma))
                ungooma_pos[num_para++] = l;
        }
        /* Pick a random one. */
        shObjectIlk::Site site = ungooma_pos[RNG (num_para)];
        shObject *ungooma = c->mImplants[site];
        --num_para;
        /* Hero gets message about the kill. */
        if (c->isHero ()) {
            if (self) {
                I->p ("You slay %s in your %s.", ungooma->theQuick (),
                      describeImplantSite (site));
                /* Remaining Ungooma become angry. */
                if (num_para) {
                    FOR_ALL_LOBES (l) {
                        shObject *imp = c->mImplants[l];
                        if (imp and imp->isA (kObjUngooma))
                            imp->setBuggy ();
                    }
                    c->computeIntrinsics ();
                }
            } else {
                I->p ("%s%s dies repelling the attack.",
                      num_para ? "One of " : "", YOUR (ungooma));
            }
        } else {
            /* Simulate damage report by sufferDamage(). */
            I->nonlOnce ();
            I->p ("  (0 dam)");
        }
        c->doff (ungooma);
        c->removeObjectFromInventory (ungooma);
        delete ungooma;
        return elapsed;
    }

    /* Hurt target already. */
    if (c->sufferDamage (atk, this, NULL, 1, 1)) {
        if (self and isHero ()) {
            c->die (kSuicide);
        } else if (!c->isHero () and (isHero () or Hero.cr ()->canSee (c))) {
            I->p ("%s is killed!", t_buf);
        }
        char *buf = GetBuf ();
        snprintf (buf, SHBUFLEN, "%s with a mental blast", the ());
        c->die (kKilled, this, NULL, atk, buf);
    }

    if (reveal and c->isHero () and !c->canSee (this)) {
        I->drawMem (mX, mY, this, NULL, NULL, NULL);
        I->refreshScreen ();
        I->pauseXY (mX, mY, 0, 0);
        if (feat (kSessile))
            Level->remember (mX, mY, mIlkId);
    }

    return elapsed;
}

int
shCreature::regeneration ()
{
    if (isHero () and mHP == mMaxHP and Hero.getStoryFlag ("lost tail")) {
        Hero.resetStoryFlag ("lost tail");
        myIlk ()->mAttacks[3].mAttId = kAttXNTail;
        myIlk ()->mAttacks[3].mProb = 1;
        myIlk ()->mAttackSum += 1;
        I->p ("You regrow your tail!");
        return FULLTURN;
    }
    int skill = getSkillModifier (kMutantPower, kRegeneration);
    if (skill <= 0)
        ++mHP;
    else
        mHP += RNG (skill, skill * 2);
    if (mHP > mMaxHP)  mHP = mMaxHP;
    return FULLTURN;
}

/* Is this balanced?  Trade -4 cha for +4 str - is awesome for non-psions! */
int
shCreature::adrenalineControl (int on)
{
    mAbil.perm_mod (abil::Str, on ? +4 : -4);
    computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::haste (int on)
{   /* Implemented in shCreature::computeIntrinsics () */
    msg (fmt ("Your actions %scelerate.", on ? "ac" : "de")) or
        appear (fmt ("%s %s!", the (), on ? "speeds up" : "slows down"));
    computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::psionicStorm (int x, int y)
{
    if (isHero ())
        I->p ("You invoke psionic storm ...");

    mLevel->attackEffect (kAttPsionicStorm, NULL, x, y, kNoDirection, this, 0);

    return SLOWTURN;
}

extern void pacifyMelnorme (shCreature *); /* Services.cpp */

int
shCreature::theVoice (int x, int y)
{
    const int elapsed = SLOWTURN;
    static const struct shVoiceEffect
    {
        const char *command;
        shCondition effect;
    } voiceData[] =
    {
        {"%s %s%s: \"%s will not read the Nam-Shub!\"", kUnableCompute},
        {"%s %s%s: \"%s will not indulge %s mutant nature!\"", kUnableUsePsi},
        {"%s %s%s: \"%s will not show belligerence!\"", kUnableAttack},
        {"%s %s%s: \"%s will not feast upon the poor!\"", kGenerous}
    };
    shCreature *c = Level->getCreature (x, y);
    if (!c) {
        if (isHero ()) I->p ("There is no one here.");
        return 0;
    }
    if (!Level->existsLOS (mX, mY, x, y)) {
        if (isHero ()) I->p ("%s cannot hear you.", THE (c));
        return 0;
    }
    if (c->myIlk ()->mType != kHumanoid and c->myIlk ()->mType != kMutant and
        c->myIlk ()->mType != kOutsider and c->myIlk ()->mType != kCyborg)
    {
        if (isHero ()) I->p ("The Voice has no effect.");
        return elapsed;
    }
    /* Determine exact The Voice command to use. */
    int choice, options;
    if (c->isA (kMonMelnorme) and c->isHostile ()) {
        I->p ("You speak: \"You will not bear petty grudges!\"");
        pacifyMelnorme (c);
        I->p ("%s complies.", THE (c));
        return elapsed;
    } else {
        int possible[3];
        options = 0;
        if (!c->is (kUnableCompute) and
            (c->isHero () or c->isA (kMonUsenetTroll) or c->isA (kMonBOFH)))
        {
            possible[options++] = 0;
        }
        if (!c->is (kUnableUsePsi)) {
            int haspowers = 0;
            if (c->isHero ()) {
                for (int i = kNoMutantPower + 1; i < kMaxHeroPower; ++i) {
                    if (c->mMutantPowers[i]) {
                        haspowers = 1;
                        break;
                    }
                }
            } else {
                for (int i = kNoMutantPower + 1; i < kMaxMutantPower; ++i) {
                    if (c->myIlk ()->mPowers[i]) {
                        haspowers = 1;
                        break;
                    }
                }
            }
            if (haspowers)  possible[options++] = 1;
        }
        if (!c->is (kUnableAttack)) {
            possible[options++] = 2;
        }
        if ((c->isA (kMonLawyer) or c->isA (kMonMelnorme))
            and !c->is (kGenerous))
        {
            possible[options++] = 2;
            if (RNG (3) or !c->isHostile ()) {
                possible[0] = 3;
                choice = 3;
                options = 1;
            }
        }
        if (options)  choice = possible[RNG (options)];
    }
    if (!options) {
        if (isHero ()) {
            I->p ("You have fully used The Voice against %s already.",
                  c == this ? "yourself" : THE (c));
        }
        return elapsed;
    }
    bool self = this == c;
    I->p (voiceData[choice].command, the (), self ? "vow" : "speak",
        isHero () ? "" : "s", self ? "I" : "you", self ? "my" : "your");

    int skill = getSkillModifier (kMutantPower, kTheVoice);
    int high = maxi (3, 3 + skill);
    if (RNG (100) < skill * 5) {
        appear (fmt ("%s disobeys.", THE (c)));
    } else {
        c->inflict (voiceData[choice].effect, FULLTURN * RNG (3, high));
    }
    return elapsed;
}

int
shCreature::magneticMayhem (int x, int y)
{
    int skill = getSkillModifier (kMutantPower, kMagMayhem);
    shAttack *atk = &Attacks[kAttMagMayhem];
    atk->mDamage[0].mLow = maxi (1, 3 + skill / 2);
    atk->mDamage[0].mHigh = maxi (3, 5 + skill);
    mLevel->attackEffect (atk, NULL, x, y, kOrigin, this, skill / 3);
    return FULLTURN;
}

int
shCreature::flight (int on)
{
    mInnateIntrinsics.set (kFlying, !!on);
    computeIntrinsics ();
    return FULLTURN;
}

bool
shCreature::terrify (int howlong, int power)
{
    if (!isAlive ())
        return false;
    int skill = getSkillModifier (kConcentration) - power;
    bool defend = RNG (100) < skill * 4;
    if (!intr (kBrainShielded) and !is (kConfused) and !is (kStunned) and !defend) {
        if (isHero ()) {
            I->p ("You suddenly feel very afraid!");
        } else if (Hero.cr ()->canSee (this) or Hero.cr ()->canHearThoughts (this)) {
            I->p ("%s seems terrified!", the ());
        }
        inflict (kFrightened, FULLTURN * (howlong / 2));
        if (isHero ())
            inflict (kFleeing, FULLTURN * howlong);
        return true;
    } else {
        appear (fmt ("%s tremble%s for a moment.", the (), isHero () ? "" : "s"));
        return false;
    }
}

/* Returns amount of force remaining after ripping webs if any are present. */
static int
kineticRipWebs (shCreature *attacker, shCreature *target, int &force)
{
    if (target->mTrapped.mWebbed) {
        bool seen = Hero.cr ()->canSee (target);
        int ripped = mini (force, target->mTrapped.mWebbed);
        force -= ripped;
        target->mTrapped.mWebbed -= ripped;
        if (!seen)  return force;
        if (target->mTrapped.mWebbed) {
            const char *who = attacker == target ? "yourself" : THE (target);
            I->p ("%s weaken%s the webs holding %s.  (-%d)", THE (attacker),
                  attacker->isHero () ? "" : "s", who, ripped);
        } else {
            I->p ("%s rip%s the webs holding %s to shreds!", THE (attacker),
                  attacker->isHero () ? "" : "s", THE (target));
        }
    }
    return force;
}

static void
kineticShoveDown (shCreature *attacker, shCreature *c, int force,
    bool fall = false)
{
    bool seen = Hero.cr ()->canSee (c);
    shFeature *f = c->mLevel->getFeature (c->mX, c->mY);
    if (c->intr (kFlying) or c->mZ == 1) {
        /* Monster can fall into feature. */
        if (f and
            (f->mType == shFeature::kVat or f->isPit () or f->isHole () or
             f->mType == shFeature::kFloorGrating))
        {
            const char *feature = f->mTrapUnknown ? AN (f) : THE (f);
            f->mTrapUnknown = 0;
            if (f->isPit ()) {
                /* The checkTraps routine supplies its own messages. */
            } else if (seen and f->mType == shFeature::kFloorGrating)
                I->p ("%s break%s through %s!", THE (c),
                      c->isHero () ? "" : "s", feature);
            else if (seen and !fall)
                I->p ("%s shove%s %s into %s!", THE (attacker),
                      attacker->isHero () ? "" : "s", THE (c), feature);
            else if (seen)
                I->p ("%s fall%s into %s!", THE (c),
                      c->isHero () ? "" : "s", feature);
            if (f->mType == shFeature::kFloorGrating) {
                f->mType = shFeature::kBrokenGrating;
                if (c->sufferDamage (kAttKineticSmash)) {
                    c->pDeathMessage (THE (c), kSlain);
                    c->die (kSlain, attacker, NULL, NULL);
                    return;
                }
            }
            if (f->isHole ()) {
                if (c->intr (kFlying)) {
                    if (seen)  I->p ("%s flies back immediately!", THE (c));
                } else {
                    c->mLevel->checkTraps (c->mX, c->mY, 100);
                }
                return;
            }
            /* Do trap stuff like installing corrosion or drowning timer. */
            if (f->isPit ()) {
                c->mIntrinsics.set (kFlying, false);
                c->mLevel->checkTraps (c->mX, c->mY, 100);
                c->mIntrinsics.set (kFlying, true);
            /* Collision with ground or feature. */
            } else if (c->sufferDamage (kAttKineticSmash)) {
                c->pDeathMessage (THE (c), kSlain);
                c->die (kSlain, attacker, NULL, NULL);
            /* Survived the fall?  Submergence in vat corrodes. */
            } else if (f->mType == shFeature::kVat) {
                if (c->sufferDamage (kAttVatCorrosion)) {
                    c->pDeathMessage (THE (c), kSlain);
                    c->die (kSlain, attacker, NULL, NULL);
                }
            }
            /* Fall from height causes large splash. */
            if (f->mType == shFeature::kAcidPit) {
                if (seen)  I->p ("Acid splashes around!");
                f->splash ();
            }
        } else { /* Slam monster into the ground. */
            if (!fall and (attacker->isHero () or seen))
                I->p ("%s slam%s %s at %s!", THE (attacker),
                    attacker->isHero () ? "" : "s", THE (c),
                    THE (c->mLevel->getSquare (c->mX, c->mY)));
            else if (fall and seen)
                I->p ("%s crashe%s into %s!", THE (c), c->isHero () ? "" : "s",
                    THE (c->mLevel->getSquare (c->mX, c->mY)));
            if (c->sufferDamage (kAttKineticSmash)) {
                c->pDeathMessage (THE (c), kSlain);
                c->die (kSlain, attacker, NULL, NULL);
            }
        }
    } else {
        if (f and (f->isPit () or f->isHole ())) {
            if (c->mTrapped.mInPit) {
                if (attacker->isHero ())
                    I->p ("You push %s deeper into %s.", THE (c), THE (f));
                else if (c->isHero ())
                    I->p ("You are pushed deeper into %s.", THE (c), THE (f));
                int delay = maxi (1, RNG (force / 2, force) + RNG (4));
                c->mTrapped.mInPit += delay;
            } else {
                if (!fall and attacker->isHero ())
                    I->p ("You push %s down %s!", THE (c), THE (f));
                else if (seen)
                    I->p ("%s %sfalls down %s!", THE (c),
                          !fall ? "suddenly " : "", THE (f));
                c->mLevel->checkTraps (c->mX, c->mY, 100);
            }
        } else {
            I->p ("Nothing happens.");
        }
    }
}

static void
kineticShoveUp (shCreature *attacker, shCreature *c, int force)
{
    bool seen = Hero.cr ()->canSee (c);
    c->mTrapped.mInPit = 0;
    c->mTrapped.mDrowning = 0;
    c->mZ = 0;
    if (seen)
        I->p ("%s shove%s %s into the ceiling!", THE (attacker),
              attacker->isHero () ? "" : "s", THE (c));
    /* WHAM!  SMASH! */
    if (c->sufferDamage (kAttKineticSmash)) {
        c->pDeathMessage (THE (c), kSlain);
        c->die (kSlain, attacker, NULL, NULL);
    }
    /* Treat like erupting geyser. */
    shFeature *f = c->mLevel->getFeature (c->mX, c->mY);
    if (!f) {
        ;
    } else if (f->mType == shFeature::kSewagePit and seen) {
        I->p ("%s %s soaked by sewage torrent.", THE (c), c->are ());
    } else if (f->mType == shFeature::kAcidPit) {
        I->p ("%s %s hit by acid torrent.", THE (c), c->are ());
        if (c->sufferDamage (kAttAcidPitBath)) {
            c->pDeathMessage (THE (c), kSlain);
            c->die (kSlain, attacker, NULL, NULL);
        }
    }
    if (c->mState == kDead and c->feat (kExplodes)) {
        /* Exploding monster has no remains that can fall down. */
    } else if (!c->intr (kFlying)) {
        /* Flying monsters can avoid falling down. */
        /* Height change needed for shove down to apply fall routine. */
        c->mZ = 1;
        kineticShoveDown (attacker, c, force, true);
    }
}

static int
telekineticRange (int base, int skill)
{
    /* Range 3 costs 2 more skill points.  Range 4 costs 3 more skill points. */
    while (skill >= base) {
        skill -= base;
        ++base;
    }
    return base;
}

/* Something ran or was shoved into active magnetically sealed door. */
static void
forceFieldContact (shCreature *c, int base, shCreature *thrower = NULL)
{
    shAttack *atk = &Attacks[kAttForceFieldContact];
    atk->mDamage[0].mLow = 1 + base * 2;
    atk->mDamage[0].mHigh = 1 + base * 3;
    if (c->sufferDamage (kAttForceFieldContact, thrower)) {
        c->appear (fmt ("%s %s fried into a crisp!", THE (c), c->are ()));
        c->die (kKilled, "contact with a strong force field");
    }
}

/* Something smashes glass panel headfirst at (x,y). */
static bool  /* Returns true if this was fatal. */
smashGlassPanel (shCreature *c, int x, int y)
{
    shSquare *s = c->mLevel->getSquare (x, y);
    if (c->isHero () or Hero.cr ()->canSee (x, y)) {
        I->p ("%s break%s through %s!", THE (c),
              c->isHero () ? "" : "s", THE (s));
    } else {
        bool nearby = distance (Hero.cr (), x, y) <= 30;
        bool far = distance (Hero.cr (), x, y) >= 180;
        I->p ("You hear glass shatter%s.",
              nearby ? " nearby" : far ? " in the distance" : "");
    }
    s->mTerr = kFloor;
    if (c->sufferDamage (kAttBreakingPanel)) {
        c->pDeathMessage (THE (c), kSlain);
        c->die (kSlain);
        return true;
    }
    return false;
}

/* Hurt poor 'c' for some damage.  Returns true on kill. */
static bool
kineticDamage (shCreature *c, shCreature *attacker, int base, bool quiet = false)
{
    shAttack *atk = &Attacks[kAttKineticSmash];
    /* Hurt amount. */
    atk->mDamage[0].mLow = 1 + base;
    atk->mDamage[0].mHigh = 10 + base * 2;
    /* Stun duration. */
    atk->mDamage[1].mLow = 1 + base / 4;
    atk->mDamage[1].mHigh = 2 + base / 3;
    if (!quiet and Hero.cr ()->canSee (c))
        I->p ("%s take%s damage on impact.", THE (c), c->isHero () ? "" : "s");
    if (c->sufferDamage (kAttKineticSmash)) {
        if (Hero.cr ()->canSee (c))
            c->pDeathMessage (THE (c), kSlain);
        c->die (kSlain, attacker, NULL, NULL);
        return true;
    }
    return false;
}

/* Return true if flight was interrupted. */
static bool
featureCollision (shCreature *c, shFeature *f, shCreature *thrower, int dmg)
{
    const char *feat = Hero.cr ()->canSee (f->mX, f->mY) ? THE (f) : "something";
    c->appear (fmt ("%s smash%s into %s!", THE (c), c->isHero () ? "" : "es", feat));
    /* These are never fun. */
    if (f->isMagneticallySealed ()) {
        forceFieldContact (c, dmg);
        return true;
    }
    bool kill = kineticDamage (c, thrower, dmg);
    /* Bash open.  TODO: Small creatures should be not always do this. */
    if (f->isDoor ()) {
        if (f->isLockedDoor ()) {
            f->mDoor.mFlags |= shFeature::kLockBroken;
            f->mDoor.mFlags &= ~shFeature::kLocked;
        }
        f->mType = shFeature::kDoorOpen;
        return kill;
    }
    return true;
}

/* Call when c1 is thrown at c2.  Return code:
    0 - continue flight
    1 - continue flight but impede it
    2 - stop flight
    3 - teleport and change direction */
static int
creatureCollision (shCreature *c1, shCreature *c2, shCreature *thrower, int dmg)
{
    /* Two same type slimes make a bigger slime!  None may be frozen. */
    if (c1->mGlyph.mSym == kSymSlime and c1->mIlkId == c2->mIlkId and
        !c1->is (kFrozen) and !c2->is (kFrozen))
    {   /* More HP and better abilities. */
        c1->mHP += c2->mHP + 4;
        c1->mMaxHP += c2->mMaxHP + 4;
        c1->mCLevel += c2->mCLevel + 1;
        FOR_ALL_ABILITIES (abil) {
            int a1 = c1->mAbil._totl.get (abil);
            int a2 = c2->mAbil._totl.get (abil);
            int m1 = c1->mAbil._max.get (abil);
            int m2 = c2->mAbil._max.get (abil);
            c1->mAbil._totl.set (abil, maxi (a1, a2));
            c1->mAbil._max.set (abil, maxi (m1, m2));
        }
        /* Better agility might have changed its speed. */
        c1->computeIntrinsics ();
        c2->die (kSuicide);
        /* Now that c2 is no longer there doors nearby should react
           but without acknowledging c1's temporary presence. */
        c1->mLevel->mCreatures[c1->mX][c1->mY] = NULL;
        c1->mLevel->checkDoors (c2->mX, c2->mY);
        c1->mLevel->mCreatures[c1->mX][c1->mY] = c1;
        /* Slurrp! */
        if (Hero.cr ()->canSee (c1) and Hero.cr ()->canSee (c2)) {
            I->p ("The %ss merge!", THE (c1));
        } else {
            I->p ("You hear a slurping sound.");
        }
        return 1;
    }
    /* Flying into a monolith teleports c1. */
    if (c2->isA (kMonMonolith)) {
        /* Heroes may touch it that way. */
        if (c1->isHero () and !c1->mHelmet) {
            Hero.touchMonolith (c2);
            return 0;
        }
        if (c1->isHero ())
            I->p ("You enter %s and emerge elsewhere!", THE (c2));
        else if (Hero.cr ()->canSee (c1) and Hero.cr ()->canSee (c2))
            I->p ("%s enters %s and disappears!", THE (c1), THE (c2));
        return 3;
    }
    /* Hit both with damage but be wary that one may kill the other
       upon dying by exploding after death or similar effect. */
    if (Hero.cr ()->canSee (c1) or Hero.cr ()->canSee (c2))
        I->p ("%s collides with %s!", THE (c1), THE (c2));
    kineticDamage (c1, thrower, dmg);
    if (c2->mState == kActing)  kineticDamage (c2, thrower, dmg);
    if (c2->mState == kActing)  c2->newEnemy (thrower);
    return c1->mState == kActing and c2->mState == kDead ? 1 : 2;
}

void
shCreature::getShoved (shCreature *attacker, shDirection dir, int range)
{
    int oldX = mX, oldY = mY;
    int x = mX, y = mY;
    bool seen = Hero.cr ()->canSee (this);
    int flown = 0;
    /* Throwing from pit. */
    if (isInPit () and range > 0) {
        --range;
        mZ = 0;
        mTrapped.mInPit = 0;
        mTrapped.mDrowning = 0;
    }
    /* The thrown creature flies above traps. */
    bool canfly = intr (kFlying);
    if (!canfly)
        mIntrinsics.set (kFlying, true);
    while (range-- > 0) {
        /* This is try to move out of the map. */
        /* TODO: Let players fall out of the map. */
        if (!mLevel->moveForward (dir, &x, &y)) {
            if (seen)
                I->p ("%s thud%s into map edge!", THE (this),
                      isHero () ? "" : "s");
            kineticDamage (this, attacker, flown * 2, true);
            break;
        }
        shCreature *c = mLevel->getCreature (x, y);
        /* Crash into a monster. */
        if (c) {
            int result = creatureCollision (this, c, attacker, flown * 2);
            /* See comment above creatureCollision definition. */
            if (result == 1)  --range;
            if (result == 2)  break;
            if (result == 3) {
                /* Creature was teleported.  Update position. */
                if (mLevel->findUnoccupiedSquare (&x, &y) == -1)
                    break;
                mLevel->moveCreature (this, x, y, true);
                seen = Hero.cr ()->canSee (this);
                ++flown;
                /* Flight direction may change. */
                dir = shDirection (RNG (8));
                continue;
            }
        }
        /* Crash (and maybe fly through) an obstacle. */
        if (mLevel->isObstacle (x, y)) {
            shFeature *f = mLevel->getFeature (x, y);
            /* Feature may slow down or stop the flight. */
            if (f and f->isObstacle ()) {
                if (featureCollision (this, f, attacker, flown * 2))
                    break;
                else
                    --range;
            }
            shSquare *s = mLevel->getSquare (x, y);
            if (!mLevel->isObstacle (x, y)) {
                /* Free pass. */
            } else if (s->mTerr == kGlassPanel) {
                --range;
                if (smashGlassPanel (this, x, y))
                    break;
            } else {
                /* Smash into everything else and stop charge. */
                if (isHero () or Hero.cr ()->canSee (x, y))
                    I->p ("%s smash%s into %s!", THE (this),
                          isHero () ? "" : "es", THE (s));
                kineticDamage (this, attacker, flown * 2, true);
                break;
            }
        }
        /* Fast move does not trigger traps or automatic doors. */
        mLevel->moveCreature (this, x, y, true);
        seen = Hero.cr ()->canSee (this);
        /* Count each square flown through. */
        ++flown;
        /* Traps that work on flying entities will have easier time with you. */
        mLevel->checkTraps (mX, mY, 10);
        /* Rip webs? */
        if (mTrapped.mWebbed)
            break;
    }
    if (!canfly)
        mIntrinsics.set (kFlying, false);
    /* Doors react only at final stage of movement. */
    mLevel->checkDoors (oldX, oldY);
    mLevel->checkDoors (mX, mY);
    mLevel->checkTraps (mX, mY);
}

static void
kineticCloseDoor (shFeature *door, shCreature *closer)
{
    bool nearby = distance (closer, door->mX, door->mY) < 100;
    bool seen = Hero.cr ()->canSee (door->mX, door->mY);
    shCreature *c = closer->mLevel->getCreature (door->mX, door->mY);
    if (c) {
        /* Attack someone! */
        if (seen) {
            if (closer->isHero ())
                I->p ("%s slam %s on %s!", THE (closer), THE (door), THE (c));
            else
                I->p ("%s slams on %s for no particular reason!", THE (door), THE (c));
        }
        bool kill = kineticDamage (c, closer, 0, true);
        /* Does the door go berserk? */
        if (door->isAutomaticDoor () and (!RNG (12) or (kill and !RNG (4)))) {
            door->mDoor.mFlags |= shFeature::kBerserk;
            if (seen) {
                const char *quips[] = {
                    "%s enjoyed that greatly and wants more.",
                    "%s starts thirsting for blood!",
                    "%s thinks this is too much fun.",
                    "%s would gladly do this again.",
                    "You teach %s evil."
                };
                const int nquips = sizeof (quips) / sizeof (const char *);
                int chosen = RNG (nquips - 1 + closer->isHero ());
                I->p (quips[chosen], THE (door));
            }
        }
        /* It might have been only creature in sensor range. */
        if (kill)  closer->mLevel->checkDoors (door->mX, door->mY);
    } else {
        /* Just close the door. */
        if (closer->isHero ()) {
            if (seen)
                I->p ("You close %s.", THE (door));
            else if (nearby)
                I->p ("You hear a door whoosh closed.");
        } else if (seen) {
            I->p ("Suddenly %s closes by itself!", THE (door));
        }
        /* Automatic door might open immediately. */
        if (!door->isAutomaticDoor ()) {
            door->mType = shFeature::kDoorClosed;
        } else {
            if (seen)
                I->p ("%s immediately opens.", THE (door));
            else if (nearby)
                I->p ("You hear a door whoosh open.");
        }
    }
}

static void
kineticOpenDoor (shFeature *door, shCreature *opener)
{
    if (door->isMagneticallySealed ()) {
        if (opener->isHero ())
            I->p ("Force field interferes with your power.");
    } else if (!door->isLockedDoor ()) {
        bool nearby = distance (opener, door->mX, door->mY) < 100;
        bool seen = Hero.cr ()->canSee (door->mX, door->mY);
        /* Open the door.Abductor*/
        if (opener->isHero ()) {
            if (seen)
                I->p ("You open %s.", THE (door));
            else if (nearby)
                I->p ("You hear a door whoosh open.");
        } else if (seen) {
            I->p ("Suddenly %s opens by itself!", THE (door));
        }
        if (!door->isAutomaticDoor ()) {
            door->mType = shFeature::kDoorOpen;
        } else {
            /* Automatic doors revert immediately.  However, let
               the hero catch sight of what is behind it for a moment. */
            if (opener->isHero ()) {
                door->mType = shFeature::kDoorOpen;
                opener->mLevel->computeVisibility (opener);
                I->pauseXY (door->mX, door->mY, 300);
                door->mType = shFeature::kDoorClosed;
            }
            if (seen)
                I->p ("%s immediately closes.", THE (door));
            else if (nearby)
                I->p ("You hear a door whoosh closed.");
        }
    } else if (opener->isHero ()) {
        I->p ("%s seems to be locked.", THE (door));
    }
}

static void
kineticCrashDoor (shFeature *door, shDirection dir, shCreature *attacker, int force)
{
    if (force < 3) {
        if (attacker->isHero ())
            I->p ("You are too weeak to rip this door from its frame.");
        return;
    }
    int x = door->mX, y = door->mY;
    if (Hero.cr ()->canSee (door->mX, door->mY))
        I->p ("%s crashes open!", THE (door));
    attacker->mLevel->removeFeature (door);
    if (!attacker->mLevel->moveForward (dir, &x, &y))
        return;
    shCreature *c = attacker->mLevel->getCreature (x, y);
    if (!c)  return;
    if (Hero.cr ()->canSee (x, y))
        I->p ("%s is crushed by the falling door!", THE (c));
    if (c->sufferDamage (kAttFallingDoor)) {
        c->pDeathMessage (THE (c), kSlain);
        c->die (kSlain, attacker, NULL, &Attacks[kAttFallingDoor]);
    }
}

int
shCreature::kinesis (int x, int y, shDirection dir)
{
    assert (isHero ());
    const int elapsed = SLOWTURN;

    int skill = getSkillModifier (kMutantPower, kKinesis);
    shAttack *atk = &Attacks[kAttKineticSmash];
    atk->mDamage[0].mLow = 1;
    atk->mDamage[0].mHigh = 10;
    atk->mDamage[1].mHigh = 2;
    int range = telekineticRange (RNG (1, 3), skill);

    shCreature *c = mLevel->getCreature (x, y);
    shFeature *f = mLevel->getFeature (x, y);
    bool seen = c and Hero.cr ()->canSee (c);
    if (c) {
        /* Cannot be pushed around. */
        if (c->isA (kMonMonolith)) {
            if (isHero ())  I->p ("%s does not budge.", THE (c));
            return elapsed;
        }
        /* Smart bomb is triggered by any movement. */
        if (c->isA (kMonSmartBomb)) {
            if (seen)  c->pDeathMessage (THE (c), kSlain);
            c->die (kSlain, this, NULL, NULL);
            return elapsed;
        }
        /* No shoving plants around. */
        if (c->isA (kMonManEatingPlant) and c->feat (kSessile)) {
            if (dir == kUp) {
                if (isHero () and seen) {
                    I->p ("You uproot %s!", THE (c));
                } else if (seen) {
                    I->p ("%s is uprooted!", THE (c));
                }
                /* No longer immobile! */
                c->mFeats &= ~kSessile;
                /* Go looking for something to do. */
                c->mStrategy = shCreature::kWander;
                c->mTactic = shCreature::kReady;
            } else if (isHero () and seen) {
                I->p ("You tug at %s but it seems to be firmly rooted.", THE (c));
            }
            return elapsed;
        }
        /* Act on the creature itself. */
        if (dir == kOrigin) {
            if (c->isA (kMonGreenTomato) or c->isA (kMonRedTomato)) {
                if (seen)
                    I->p ("%s %s squished!", THE (c), c->are ());
                c->die (kSlain, this, NULL, NULL);
                return elapsed;
            } else if (c->isA (kMonKamikazeGoblin)) {
                if (isHero ())
                    I->p ("You activate %s's detonator.", THE (c));
                if (seen)
                    I->p ("%s makes a priceless `oh shit' face.", THE (c));
                c->die (kSlain, "remote detonator activation");
                return elapsed;
            } else if (f and f->isOpenDoor ()) {
                kineticCloseDoor (f, this);
            } else if (isHero ()) {
                I->p ("%s does not seem to be that squishy.", THE (c));
            }
            return elapsed;
        }
        range = kineticRipWebs (this, c, range);
        /* All force might be spent ripping the webs. */
        if (!range)  return elapsed;

        c->newEnemy (this);
        if (dir == kDown)  kineticShoveDown (this, c, range * 2 / 3);
        else if (dir == kUp)  kineticShoveUp (this, c, range * 2 / 3);
        else c->getShoved (this, dir, range);
        return elapsed;
    }
    if (f) {
        bool ignore = false;
        int tohit = skill / 2;
        f->mTrapUnknown = 0;
        switch (f->mType) {
        case shFeature::kAcidPit:
            if (Hero.cr ()->canSee (x, y)) {
                I->p ("%s move%s some acid.", the (), isHero () ? "" : "s");
            }
            mLevel->attackEffect (kAttAcidWave, NULL, x, y, dir, this, tohit);
            break;
        case shFeature::kSewagePit:
            if (Hero.cr ()->canSee (x, y)) {
                I->p ("%s move%s some water.", the (), isHero () ? "" : "s");
            }
            mLevel->attackEffect (kAttSquirtWater, NULL, x, y, dir, this, tohit);
            break;
        case shFeature::kDoorClosed:
            if (dir == kOrigin)
                kineticOpenDoor (f, this);
            else
                kineticCrashDoor (f, dir, this, range);
            break;
        case shFeature::kDoorOpen:
            if (dir != kOrigin)
                ignore = true;
            else
                kineticCloseDoor (f, this);
            break;
        case shFeature::kVat:
            if (isHero ())  I->p ("%s is too heavy to move.", THE (f));
            break;
        case shFeature::kFloorGrating:
        case shFeature::kBrokenGrating:
        default:
            ignore = true;
            break;
        }
        if (!ignore)  return elapsed;
    }
    /* Hurl object lying on the floor. */
    shObjectVector *v = mLevel->getObjects (x, y);
    if (!v)  return elapsed;
    shObject *obj;
    if (v->count () == 1)
        obj = v->get (0);
    else
        obj = quickPickItem (v, "hurl", 0);
    /* No cancelling.  It would be unfair to allow cancelling throwing
       items but not to reconsider shoving monsters around. */
    if (!obj)  obj = v->get (RNG (v->count ()));
    shAttackId attid = obj->isThrownWeapon () ? obj->myIlk ()->mMissileAttack :
        kAttImprovisedThrow;
    int tohit = skill / 3;
    if (obj->mCount > 1) {
        shObject *single = obj->split (1);
        projectile (single, obj, x, y, dir, &Attacks[attid], tohit, range);
    } else {
        v->remove (obj);
        if (!v->count ()) {
            delete v;
            mLevel->mObjects[x][y] = NULL;
        }
        projectile (obj, NULL, x, y, dir, &Attacks[attid], tohit, range);
    }
    return elapsed;
}

static int
useKinesis (shCreature *h)
{
    int x = -1, y = -1;
    shDirection dir;
    if (!I->getSquare ("What do you want to hurl?  (select a location)",
                       &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
        I->nevermind ();
    else if (!h->mLevel->existsLOS (h->mX, h->mY, x, y) and
             !h->mLevel->existsLOS (x, y, h->mX, h->mY))
        I->p ("You need a clear path.");
    else if ((dir = I->getDirection ()) == kNoDirection)
        ;
    else {
        h->kinesis (x, y, dir);
        return SLOWTURN;
    }
    return CANCEL_POWER;
}

static int
useShockwave (shCreature *h)
{   /* In general this power is meant to be semi-reliable panic button. */
    I->p ("You emit a shockwave!");
    int skill = h->getSkillModifier (kMutantPower, kShockwave);
    shAttack *atk = &Attacks[kAttShockwave];
    atk->mDamage[0].mLow = maxi (1, skill - 10);
    atk->mDamage[0].mHigh = maxi (7, 7 + skill);
    shDirection dirs[8];
    for (int i = 0; i < 8; ++i)
        dirs[i] = shDirection (i);
    shuffle (dirs, 8, sizeof (shDirection));
    for (int i = 0; i < 8; ++i) {
        int x = h->mX, y = h->mY;
        if (!h->mLevel->moveForward (dirs[i], &x, &y))
            continue;
        shCreature *c = h->mLevel->getCreature (x, y);
        if (!c)  continue;
        if (h->canSee (c))
            I->p ("%s is blasted!", THE (c));
        if (c->sufferDamage (kAttShockwave)) {
            c->pDeathMessage (THE (c), kKilled);
            c->die (kKilled, h, NULL, &Attacks[kAttShockwave]);
        } else {
            int range = telekineticRange (RNG (1, 2), skill);
            c->getShoved (h, dirs[i], range);
        }
    }
    return HALFTURN;
}

static int
useDigestion (shCreature *h)
{
    if (h->isHero () and Hero.getStoryFlag ("superglued tongue")) {
        I->p ("You can't eat anything with this stupid canister "
              "glued to your mouth!");
        return CANCEL_POWER;
    }

    shObject *obj =
        h->quickPickItem (h->mInventory, "eat", shMenu::kCategorizeObjects);

    if (!obj)  return CANCEL_POWER;

    bool ok = false;
    const char *your_obj = YOUR (obj);
    int skill = h->getSkillModifier (kMutantPower, kDigestion);
    /* Objects on your head can't be eaten conveniently, but otherwise
       we'll assume that digestion comes along with super flexibility
       required to eat belts, boots, armor, etc.
     */
    if (obj->isA (kImplant) and obj->is (obj::worn) and !obj->isA (kObjTorc)) {
        I->p ("You'll have to uninstall that first.");
    } else if ((h->mHelmet == obj and skill < 6) or
               (h->mGoggles == obj and skill < 3))
    {
        I->p ("You'll have to take it off first.");
    /* No screwing yourself over a miskeypress. */
    } else if (!obj->is (obj::known_appearance) and
       (obj->isA (kObjTheOrgasmatron) or
        obj->isA (kObjFakeOrgasmatron1) or
        obj->isA (kObjFakeOrgasmatron2) or
        obj->isA (kObjFakeOrgasmatron3) or
        obj->isA (kObjFakeOrgasmatron4) or
        obj->isA (kObjFakeOrgasmatron5)))
    {
        I->p ("Hero, it *might* be the Bizarro Orgasmatron.  You may need *that* to win!");
        I->p ("Go lose the game some other (and more creative) way.");
    } else if (obj->isA (kObjTheOrgasmatron)) {
        I->p ("Hero, this is the Bizarro Orgasmatron!  You need *that* to win!");
        I->p ("Go lose the game some other (and more creative) way.");
        /* Knowingly eating other Orgasmatrons is fine. */
    } else {
        if (h->mHelmet == obj or h->mGoggles == obj) {
            I->p ("You snatch %s with elongated tongue.", YOUR (obj));
            your_obj = "it";
        }
        ok = true;
    }
    if (!ok)  return CANCEL_POWER;

    /* A way to remove buggy bio armor. */
    if (obj->isA (kObjBioArmor) and obj->is (obj::worn)) {
        I->p ("%s gets very scared and stops clinging to you.", YOUR (obj));
        h->doff (obj);
        return HALFTURN;
    }

    /* Okay!  We can dine on the item. */
    I->p ("You devour %s.", your_obj);
    shObject *stack = NULL;
    if (obj->mCount > 1) {
        stack = obj;
        obj = h->removeOneObjectFromInventory (obj);
    } else {
        h->removeObjectFromInventory (obj);
    }

    /* Eat one floppy from a stack to see if they are infected. */
    if (obj->is (obj::infected) and !h->mResistances[kSickening]) {
        h->inflict (kSickened, RNG (15, 40) * FULLTURN);
        if (stack and !stack->is (obj::known_infected)) {
            stack->set (obj::known_infected);
            stack->announce ();
        }
    }
    delete obj;
    return FULLTURN;
}

static int
useAdrenaline (shCreature *h)
{
    I->p ("Blood rushes to your muscles!");
    return h->adrenalineControl (1);
}

static int
stopAdrenaline (shCreature *h)
{
    I->p ("You feel exhausted.");
    return h->adrenalineControl (0);
}

static int
useHaste (shCreature *h)
{
    return h->haste (1);
}

static int
stopHaste (shCreature *h)
{
    return h->haste (0);
}

static int
useTelepathy (shCreature *h)
{
    I->p ("You extend the outer layers of your mind.");
    return h->telepathy (1);
}

static int
stopTelepathy (shCreature *h)
{
    I->p ("Your consciousness contracts.");
    return h->telepathy (0);
}

static int
useTremorsense (shCreature *h)
{
    return h->tremorsense (1);
}

static int
stopTremorsense (shCreature *h)
{
    return h->tremorsense (0);
}

static int
useOpticBlast (shCreature *h)
{
    if ((h->intr (kPerilSensing) and h->mGoggles->is (obj::toggled)) or
        (h->mGoggles and h->mGoggles->isA (kObjBlindfold)))
    {
        if (!h->mGoggles->is (obj::fooproof)) {
            I->p ("You incinerate %s!", YOUR (h->mGoggles));
            if (h->sufferDamage (kAttBurningGoggles)) {
                h->shCreature::die (kMisc, "Burned his face off");
            }
            h->removeObjectFromInventory (h->mGoggles);
        } else {
            I->p ("You try to ignite %s without result.", YOUR (h->mGoggles));
        }
        return FULLTURN;
    } else if (h->intr (kBlind)) {
        I->p ("But you are blind!");
        return CANCEL_POWER;
    }
    shDirection dir = I->getDirection ();
    if (dir == kNoDirection) {
        return CANCEL_POWER;
    } else if (dir == kOrigin) {
        I->p ("If you had an eye looking inwards you could manage this.");
        /* Warning: when third eye implant appears allow this! */
        return CANCEL_POWER;
    } else {
        return h->opticBlast (dir);
    }
}

static int
useRegeneration (shCreature *h)
{
    if (h->mHP == h->mMaxHP) {
        I->p ("What for?");
        return CANCEL_POWER;
    }
    return h->regeneration ();
}

static int
useRestoration (shCreature *h)
{
    if (!h->needsRestoration ()) {
        I->p ("You aren't in need of restoration.");
        return CANCEL_POWER;
    }
    int skill = h->getSkillModifier (kMutantPower, kRestoration);
    int low = maxi (1, 1 + skill / 5);
    int high = maxi (2, 2 + skill / 3);
    return h->restoration (RNG (low, high));
}

static int
useHypnosis (shCreature *h)
{
    shDirection dir = I->getDirection ();
    if (kNoDirection == dir) {
        return CANCEL_POWER;
    } else if (kOrigin == dir) {
        I->p ("You need a mirror for that.");
        return CANCEL_POWER;
    } else {
        return h->hypnosis (dir);
    }
}

static int
useXRayVision (shCreature *h)
{
    return h->xRayVision (1);
}

static int
stopXRayVision (shCreature *h)
{
    return h->xRayVision (0);
}

static int
useAutolysis (shCreature *h)
{
    h->msg ("You begin harnessing metacreative energy of your body.");
    return LONGTURN;
}

static int
stopAutolysis (shCreature *h)
{
    h->msg ("You switch to relying on your psionic prowess alone.");
    return 0;
}

static int
useTeleport (shCreature *h)
{
    int skill = h->getSkillModifier (kMutantPower, kTeleport);
    h->transport (-1, -1, (skill - 5) * 10, h->is (kConfused));
    if (!h->is (kConfused) and RNG (10) >= skill) {
        I->p ("You feel disoriented.");
        h->inflict (kConfused, NDX (2, 2) * FULLTURN);
    }
    return HALFTURN;
}

static int
useIllumination (shCreature *h)
{
    int skill = h->getSkillModifier (kMutantPower, kIllumination);
    int radius = maxi (1, 3 + skill);

    for (int x = h->mX - radius; x <= h->mX + radius; x++)
        for (int y = h->mY - radius; y <= h->mY + radius; y++)
            if (Level->isInBounds (x, y) and Level->isInLOS (x, y)
                and distance (x, y, h->mX, h->mY) < radius * 5)
            {
                Level->setLit (x, y, 1, 1, 1, 1);
            }

    return HALFTURN;
}

static int
useWeb (shCreature *h)
{
    shDirection dir = I->getDirection ();
    if (dir == kNoDirection)
        return CANCEL_POWER;
    return h->shootWeb (dir);
}

/* The two below are pretty similar.  When third of ilk appears */
static int /* try joining them into one. */
useMentalBlast (shCreature *h)
{
    int x = -1, y = -1;
    if (!I->getSquare ("What do you want to blast?  (select a location)",
                           &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
    {
        I->nevermind ();
        return CANCEL_POWER;
    }
    return h->mentalBlast (x, y);
}

static int
useTheVoice (shCreature *h)
{
    int x = -1, y = -1;
    if (!I->getSquare ("Who do you want to command?  (select a location)",
                       &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
    {
        return CANCEL_POWER;
    }
    return h->theVoice (x, y);
}

static int
useMayhem (shCreature *h)
{
    int x = -1, y = -1;
    if (!I->getSquare ("Discharge where?  (select a location)",
                       &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
    {
        return CANCEL_POWER;
    }
    return h->magneticMayhem (x, y);
}

static int
useFlight (shCreature *h)
{   /* Be vary of vacuum in the future. */
    I->p ("You gain extraordinary control over your motions.");
    h->mInnateResistances[kElectrical] += 2;
    return h->flight (1);
}

static int
stopFlight (shCreature *h)
{   /* FIXME: wholly unappropriate if one loses concentration. */
    if (h->mZ == 1) {
        I->p ("You gracefully descend.");
        h->mZ = 0;
        h->mLevel->checkTraps (h->mX, h->mY);
    }
    h->mInnateResistances[kElectrical] -= 2;
    return h->flight (0);
}

static int
useUnderstanding (shCreature *h)
{
    h->mInnateIntrinsics.set (kTranslation, true);
    h->computeIntrinsics ();
    return 0;
}

static int
stopUnderstanding (shCreature *h)
{
    h->mInnateIntrinsics.set (kTranslation, false);
    h->computeIntrinsics ();
    return 0;
}

static int
useGammaSight (shCreature *h)
{
    h->computeIntrinsics ();
    I->drawScreen ();
    return 0;
}

static int
usePsionicStorm (shCreature *h)
{
    int x = -1, y = -1;
    if (!I->getSquare ("Where do you want to invoke the storm?"
            " (select a location)", &x, &y, 100, 0, MUTPOWER_CURSOR))
    {
        return CANCEL_POWER;
    } else {
        int elapsed = h->psionicStorm (x, y);
        /* One Psi point is damaged, the rest is drained. */
        int drain = maxi (0, h->mAbil.totl (abil::Psi) - MutantPowers[kPsionicStorm].mLevel - 1);
        if (h->mMutantPowers[kAutolysis] == MUT_POWER_ON)
            drain /= 2;
        h->sufferAbilityDamage (abil::Psi, 1);
        h->mAbil.temp_mod (abil::Psi, -drain);
        h->mPsionicStormFatigue += RNG (1, 2);
        return elapsed;
    }
}

static int
usePsiVomit (shCreature *h)
{
    shDirection dir = I->getDirection ();
    if (dir == kNoDirection)  return CANCEL_POWER;
    I->p ("Bleargh!");
    int skill = h->getSkillModifier (kMutantPower, kPsiVomit);
    int bonus = h->mAbil.Psi () / 2;
    h->mAbil.temp_mod (abil::Psi, -h->mAbil.Psi ());
    shAttack *atk = &Attacks[kAttPsiVomit];
    atk->mDamage[0].mLow = maxi (1, 1 + bonus + skill / 2);
    atk->mDamage[0].mHigh = 10 + bonus + skill;
    atk->mRange = NDX (2, 2) + skill / 5;
    h->mLevel->attackEffect (atk, NULL, h->mX, h->mY, dir, h, skill / 3);
    return FULLTURN;
}

static int
useCauseFear (shCreature *h)
{
    int x = -1, y = -1;
    shCreature *c = NULL;
    if (!I->getSquare ("In whom do you want to instill fear?"
         " (select a location)", &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
    {
        I->nevermind ();
        return CANCEL_POWER;
    }
    c = h->mLevel->getCreature (x, y);
    if (!c or (!h->canSee (c) and !h->canHearThoughts (c))) {
        I->p ("You need to target someone you can see or hear thoughts.");
    } else if (!c->isAlive ()) {
        I->p ("%s cannot fear.", THE (c));
    } else {
        int skill = h->getSkillModifier (kMutantPower, kCauseFear);
        skill = maxi (-3, skill);
        c->terrify (NDX (2, 4 + skill / 4), skill);
        return SLOWTURN;
    }
    return CANCEL_POWER;
}

static int
useImbue (shCreature *)
{
    return FULLTURN;
}

static int
useAwareness (shCreature *h)
{
    if (h->usesPower (kBGAwareness))
        h->msg ("You no longer understand your body so intimately.");
    else
        h->msg ("You gain control and understanding of your body at celluar level.");
    return FULLTURN;
}

static int
useDeepSight (shCreature *h)
{
    h->computeSkills ();
    return 0;
}

static int
useWarFlow (shCreature *h)
{   /* Implemented as direct mToHitModifier increase in `computeIntrinsics'. */
    h->computeIntrinsics ();
    return 0;
}

int
shCreature::charge (shDirection dir)
{
    const int elapsed = FULLTURN;
    bool seen = Hero.cr ()->canSee (this);
    int skill = getSkillModifier (kMutantPower, kCharge);
    int tohit = skill / 2;
    int todam = skill / 4;
    int range = telekineticRange (2, skill);
    /* Charging upwards is a way to quickly escape pit traps. */
    if (dir == kUp) {
        shFeature *f = mLevel->getFeature (mX, mY);
        if (range >= mTrapped.mInPit) {
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
            if (seen) {
                if (f and f->mType == shFeature::kSewagePit)
                    I->p ("%s quickly swim%s to the surface!", THE (this),
                          isHero() ? "" : "s");
                else if (f)
                    I->p ("%s rush%s out of %s!", THE (this),
                          isHero() ? "" : "es", THE (f));
                else
                    I->p ("You see something buggy happen to %s.", THE (this));
            }
        } else {
            mTrapped.mInPit -= range;
            if (seen) {
                if (f and f->mType == shFeature::kSewagePit)
                    I->p ("%s quickly swim%s towards the surface!", THE (this),
                          isHero() ? "" : "s");
                else if (f)
                    I->p ("%s rush%s to get out of %s!", THE (this),
                          isHero() ? "" : "es", THE (f));
                else
                    I->p ("You see something buggy happen to %s.", THE (this));
            }
        }
        return elapsed;
    }
    shAttack *stun = &Attacks[kAttHellbentStun];
    if (seen)
        I->p ("%s put%s on a burst of speed!", THE (this), isHero () ? "" : "s");
    int oldX = mX, oldY = mY;
    int x = mX, y = mY;
    int bonus = is (kSpeedy) * 3;
    while (range-- > 0) {
        /* This is try to move out of the map. */
        /* TODO: Let players fall out of the map. */
        if (!mLevel->moveForward (dir, &x, &y)) {
            if (seen)
                I->p ("%s balk%s and stop%s!", THE (this),
                      isHero () ? "" : "s", isHero () ? "" : "s");
            return elapsed;
        }
        shCreature *c = mLevel->getCreature (x, y);
        /* Run into first encountered monster. */
        if (c) {
            int elapsed;
            mToHitModifier += bonus / 2 + tohit;
            mDamageModifier += bonus + todam;
            if (isHero ())
                meleeAttack (mWeapon, dir);
            else
                doAttack (c, &elapsed);
            computeIntrinsics (); /* Clear bonus. */
            break;
        }
        /* Run into first obstacle. */
        if (mLevel->isObstacle (x, y)) {
            shFeature *f = mLevel->getFeature (x, y);
            /* Run into doors, stop on everything else. */
            if (f and f->isObstacle ()) {
                if (f->mType == shFeature::kDoorClosed) {
                    /* Attempt to bash the door down. */
                    bool see_f = Hero.cr ()->canSee (x, y);
                    const char *f_desc;
                    if (see_f)
                        if (f->isMagneticallySealed ())
                            f_desc = "force field";
                        else
                            f_desc = THE (f);
                    else
                        f_desc = "something";
                    if (isHero () or seen or see_f)
                        I->p ("%s smash%s into %s!", THE (this),
                              isHero () ? "" : "es", f_desc);

                    if (f->isMagneticallySealed ()) {
                        /* Not a wise thing in general. */
                        forceFieldContact (this, bonus);
                        break;
                    } else {
                        int base = mAbil.Str () + bonus + f->mSportingChance;
                        if (RNG (100) < base * 5) {
                            if (see_f)
                                I->p ("The door crashes open!");
                            if (f->isLockedDoor ()) {
                                f->mDoor.mFlags |= shFeature::kLockBroken;
                                f->mDoor.mFlags &= ~shFeature::kLocked;
                            }
                            f->mType = shFeature::kDoorOpen;
                            --range;
                            /* TODO: Chance for door going berserk.
                               "The door swears revenge!" */
                        } else if (RNG (2)) {
                            if (see_f)
                                I->p ("The door holds firm.");
                            f->mSportingChance += bonus;
                            break;
                        } else {
                            if (seen)
                                I->p ("%s %s off-balance.", THE (this), are ());
                            f->mSportingChance += bonus / 2;
                            stun->mDamage[0].mLow = 1;
                            stun->mDamage[0].mHigh = 2 + bonus;
                            sufferDamage (stun);
                            break;
                        }
                    }
                } else {
                    if (isHero () or Hero.cr ()->canSee (x, y))
                        I->p ("%s run%s into %s!", THE (this),
                              isHero () ? "" : "s", THE (f));
                    stun->mDamage[0].mLow = 2;
                    stun->mDamage[0].mHigh = 2 * (2 + bonus);
                    sufferDamage (stun);
                    break;
                }
            }
            shSquare *s = mLevel->getSquare (x, y);
            if (!mLevel->isObstacle (x, y)) {
                /* Free pass. */
            } else if (bonus and s->mTerr == kGlassPanel) {
                /* Smash glass panels after at least one step. */
                --range;
                if (smashGlassPanel (this, x, y))
                    break;
            } else {
                /* Smash into everything else and stop charge. */
                if (isHero () or Hero.cr ()->canSee (x, y))
                    I->p ("%s run%s into %s!", THE (this),
                          isHero () ? "" : "s", THE (s));
                stun->mDamage[0].mLow = 2;
                stun->mDamage[0].mHigh = 2 * (2 + bonus);
                sufferDamage (stun);
                break;
            }
        }
        /* Fast move does not trigger traps or automatic doors. */
        mLevel->moveCreature (this, x, y, true);
        seen = Hero.cr ()->canSee (this);
        /* Charge bonus for each step made. */
        bonus += 2;
        /* It is harder to avoid traps while running. */
        mLevel->checkTraps (mX, mY, 10);
        /* Trap holds the charging creature. */
        if (mTrapped.mInPit or mTrapped.mWebbed)
            break;
    }
    /* Doors react only at final stage of movement. */
    mLevel->checkDoors (oldX, oldY);
    mLevel->checkDoors (mX, mY);
    return elapsed;
}

static int
useCharge (shCreature *h)
{   /* Several conditions need to be satisfied first. */
    if (h->is (kFrightened)) {
        I->p ("You are too afraid to make bold moves!");
    } else if (h->is (kViolated)) {
        I->p ("You cannot run with your current predicament.");
    } else if (h->is (kSlowed)) {
        I->p ("You are unable to run when slowed.");
    } else if (h->is (kStrained)) {
        I->p ("You are too burdened to run.");
    } else {
        /* Okay, now choose valid direction to charge. */
        shDirection dir = I->getDirection ();
        if (dir == kNoDirection or dir == kDown or dir == kOrigin or
            (dir == kUp and !h->mTrapped.mInPit))
        {
            /* Cancel. */
        } else
            return h->charge (dir);
    }
    return CANCEL_POWER;
}

int
shMutantPowerData::maintainCost (void)
{
    if (!isPersistent ())
        return 0;

    if (mFunc == useImbue)
        return 0;

    return mLevel;
}

const char *
getMutantPowerName (shMutantPower id)
{
    return MutantPowers[id].mName;
}

int
shCreature::getMutantPowerChance (int power)
{
    if (power == kPsiVomit)
        return mAbil.temp (abil::Psi) >= 0 ? 100 : 0;

    /* Chance to use is based off faculty skill bonus, character level bonus,
       base value, psionic aura and mutant power level malus. */
    int chance = (getSkillModifier (MutantPowers[power].mSkill)
         + mini (mCLevel, 10) /* At higher levels anyone could use any power. */
         + mPsiModifier + 15 - 2 * MutantPowers[power].mLevel) * 5;

    /* Too powerful for current character level. */
    if (MutantPowers[power].mLevel * 2 > mCLevel + 1)
        chance -= 20;

    if (power == kPsionicStorm)
        chance -= 5 * mPsionicStormFatigue;

    if (isHero () and Psion == Hero.mProfession)
        chance += mCLevel * 5;

    chance = mini (maxi (0, chance), 100);

    return chance;
}

int
shCreature::stopMutantPower (shMutantPower id)
{
    /* Allow released Psi points to regenerate over time. */
    int cost = MutantPowers[id].maintainCost ();
    mAbil.perm_mod (abil::Psi, +cost);
    mAbil.temp_mod (abil::Psi, -cost);
    mMutantPowers[id] = MUT_POWER_PRESENT;
    return MutantPowers[id].mOffFunc (this);
}

static void
describePower (const int pow)
{
    extern const char *getCodename (const char *); /* Help.cpp */
    const char *name = getCodename (MutantPowers[pow].mName);

    shTextViewer *viewer = new shTextViewer (name);
    viewer->show ();
    delete viewer;
}

/* returns: elapsed time */
int
shCreature::useMutantPower ()
{
    assert (isHero ());
    if (!isHero ())  return FULLTURN;

    shMenu *menu = I->newMenu ("Use which mutant power?", 0);
    menu->attachHelp (describePower);
    int n = 0;
    char buf[50];

    if (is (kUnableUsePsi)) {
        msg ("Mere thought of this seems disguisting to you.");
        return 0;
    }
    menu->addHeader ("       Power                  Level Chance");
    for (int i = kNoMutantPower; i < kMaxHeroPower; ++i) {
        if (mMutantPowers[i]) {
            int chance = getMutantPowerChance (i);
            int l = snprintf (buf, 50, "%-23s  %d    ",
                          MutantPowers[i].mName, MutantPowers[i].mLevel);
            if (mMutantPowers[i] == MUT_POWER_ON) {
                snprintf (&buf[l], 10, "(on)");
            } else {
                snprintf (&buf[l], 10, "%3d%%", chance);
            }
            menu->addIntItem (MutantPowers[i].mLetter, buf, i, 1);
            ++n;
        }
    }
    if (n == 0) {
        I->p ("You don't have any mutant powers.");
        return 0;
    }
    int i = 0;
    menu->getIntResult (&i, NULL);
    delete menu;
    if (i) {
        /* Deactivating a power, always succeed. */
        if (mMutantPowers[i] == MUT_POWER_ON)
            return stopMutantPower ((shMutantPower) i);

        bool persistent = MutantPowers[i].isPersistent ();
        int cost = persistent ? MutantPowers[i].maintainCost ()
                              : MutantPowers[i].mLevel;
        /* For persistent powers only raw Psi counts. */
        int pool = persistent ? mAbil.curr (abil::Psi) : mAbil.totl (abil::Psi);
        if (pool <= cost) {
            I->p ("You are too weak to use that power.");
            return 0;
        }

        int chance = getMutantPowerChance (i);
        chance += 100 * BOFH; /* Easier debugging of powers. */
        if (chance == 0) {
            I->p ("No point in trying this.");
            return 0;
        }

        if (i == kRegeneration) {
            if (is (kSickened)) {
                I->p ("Your organism is busy fighting sickness.");
                return 0;
            } else if (is (kPlagued)) {
                I->p ("Plague inhibits your regenerative powers.");
                return 0;
            }
        }

        /* Autolysis decreases cost of some powers but takes HP. */
        bool autolysis = mMutantPowers[kAutolysis] == MUT_POWER_ON;
        if (autolysis and !persistent and i != kRegeneration) {
            int net_cost = cost / 2 + cost % 2;
            int supplied = cost - net_cost;
            int level = MutantPowers[i].mLevel;
            int hp_loss = supplied * level * level;
            int skill = getSkillModifier (kMutantPower, kAutolysis);
            hp_loss = maxi (hp_loss / 2, hp_loss - skill);
            if (hp_loss >= mHP) {
                I->p ("You just wrung out last drop of metacreative energy in your body.");
                shCreature::die (kKilled, "spontaneous tissue breakdown");
                return 0;
            } else {
                mHP -= hp_loss;
            }
            cost = net_cost;
        }

        int roll = RNG (100);
        debug.log ("mutant power attempt: rolled %d %s %d.",
                   roll, roll < chance ? "<" : ">=", chance);

        /* Successful use of mutant power. */
        if (roll < chance) {
            int ret = MutantPowers[i].mFunc (this);
            if (ret == CANCEL_POWER)
                return 0;

            if (persistent) {
                mAbil.perm_mod (abil::Psi, -cost);
                mMutantPowers[i] = MUT_POWER_ON;
            } else {
                mAbil.temp_mod (abil::Psi, -cost);
            }
            return ret;
        /* Fail.  TODO: This should have consequences if chances were low. */
        } else {
            /* Failing to activate imbue extracts a price in spite of zero activation cost. */
            if (i == kImbue)  cost = 1;
            mAbil.temp_mod (abil::Psi, -cost);
            I->p ("You fail to use your power successfully.");
            return HALFTURN;
        }
    }
    return 0;
}

/* Certain distractions (such as getting stunned) may cause your */
void    /* concentration to falter and powers to be stopped. */
shCreature::checkConcentration ()
{
    if (!isHero ())  return;
    int failed = 0;
    int basesk = getSkillModifier (kConcentration);

    for (int i = kNoMutantPower; i < kMaxHeroPower; i++) {
        int sk = basesk + RNG (1, 20);
        if (mMutantPowers[i] != MUT_POWER_ON or sk >= 15)
            continue;

        if (!failed)
            I->p ("You lose your concentration.");

        ++failed;
        int cost = MutantPowers[i].maintainCost ();
        mAbil.perm_mod (abil::Psi, +cost);
        mAbil.temp_mod (abil::Psi, -cost);
        mMutantPowers[i] = MUT_POWER_PRESENT;
        MutantPowers[i].mOffFunc (this);
    }
    if (failed)
        I->p ("You fail to maintain %d continuous mutant power%s.",
            failed, failed > 1 ? "s" : "");
}



shMutantPower
shCreature::getMutantPower (shMutantPower power, int silent)
{
    shMutantPower gained = kNoMutantPower;
    if (kNoMutantPower == power) {
        int attempts = 10;
        while (attempts--) {
            power = (shMutantPower) RNG (kNoMutantPower + 1, kMaxHeroPower - 1);
            if (!mMutantPowers[power] and MutantPowers[power].mFunc and
                mCLevel + 3 >= MutantPowers[power].mLevel and
                /* Those powers need character to have eyes: */
                (!mInnateIntrinsics.get (kBlind) or
                 (kOpticBlast != power and kXRayVisionPower != power))
                )
            {
                mMutantPowers[power] = MUT_POWER_PRESENT;
                gained = power;
                break;
            }
        }
    } else {
        mMutantPowers[power] = MUT_POWER_PRESENT;
    }
    if (gained != kNoMutantPower and !silent) {
        msg ("Your brain is warping!");
        msg (fmt ("You gain the \"%s\" mutant power!",
             MutantPowers[power].mName));
    }
    return gained;
}

void
shCreature::lose_mutant_power (shMutantPower power, bool silent)
{
    if (mMutantPowers[power] == MUT_POWER_ON)
        MutantPowers[power].mOffFunc (this);

    mMutantPowers[power] = MUT_POWER_ABSENT;
    if (!silent)
        msg (fmt ("You lose the \"%s\" mutant power!",
             MutantPowers[power].mName));
}



/* returns elapsed time, 0 if no power used, -1 if creature dies */

int
shCreature::monUseMutantPower ()
{
    int x0, y0, x, y, res;
    shCreature *h = Hero.cr ();

    if (mSpellTimer > Clock) {
        return 0;
    }

    const char *who = the ();
    /* Guide: blocking a mutant power and printing a message causes four
       turns of delay before next attempt. Blocking a power silently causes
       only two turns of delay so player has a chance of seeing effects of
       The Voice. */
    bool forbidden = is (kUnableUsePsi);

    for (int attempts = myIlk ()->mNumPowers * 2; attempts; --attempts) {
        int choice = RNG (myIlk ()->mNumPowers);
        shMutantPower power = myIlk ()->mPowers[choice];

        switch (power) {
        case kHypnosis:
            if (!areAdjacent (this, h) or h->isHelpless () or
                h->intr (kBlind))
            {
                continue;
            }
            if (forbidden) {
                if (h->canSee (this)) {
                    I->p ("%s gazes at you but immediately flinches.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            if (h->canSee (this)) {
                I->p ("%s gazes into your eyes.", who);
            }
            mSpellTimer = Clock + 10 * FULLTURN;
            return hypnosis (vectorDirection (mX, mY, h->mX, h->mY));
        case kOpticBlast:
            if (!canSee (h))
                continue;
            {
                shDirection dir = linedUpDirection (this, h);
                if (kNoDirection == dir)
                    continue;
                if (forbidden) {
                    if (h->canSee (this)) {
                        I->p ("%s stares at you intently but immediately flinches.", who);
                    }
                    mSpellTimer = Clock + 4 * FULLTURN;
                    return FULLTURN;
                }
                if (h->canSee (this)) {
                    I->p ("%s shoots a laser beam out of %s!",
                          who, her ("eyes"));
                } else {
                    I->p ("Someone shoots a laser beam out of %s!",
                          her ("eyes"));
                    Level->feelSq (mX, mY);
                }
                return opticBlast (dir);
            }
        case kMentalBlast:
            if (!canSee (h) and
                (!intr (kTelepathy) or distance (h, mX, mY) >= 60))
            {
                continue;
            }
            if (forbidden) {
                if (h->canSee (this) and numHands ()) {
                    I->p ("%s begins to concentrate but immediately stops.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            if (h->canSee (this) and numHands ()) {
                I->p ("%s concentrates.", who);
            }
            mSpellTimer = Clock + RNG (8, 10) * FULLTURN;
            return mentalBlast (h->mX, h->mY);
        case kRegeneration:
            if (mHP >= mMaxHP or is (kSickened))
                continue;
            if (forbidden) {
                mSpellTimer = Clock + 2 * FULLTURN;
                return 0;
            }
            if (h->canSee (this)) {
                if (numHands ())
                    I->p ("%s concentrates.", who);
                I->p ("%s looks better.", who);
            }
            mSpellTimer = Clock + 2 * FULLTURN;
            return regeneration ();
        case kTeleport:
            if ((mHP < mMaxHP / 4 or is (kFleeing)) and canSee (h)) {
                /* escape to safety */
                x = -1, y = -1;
                /* make double teleport less likely */
                mSpellTimer = Clock + RNG (10 * FULLTURN);
            } else if (intr (kTelepathy) and mHP == mMaxHP and !RNG (10)) {
                /* teleport in to make trouble */
                x = h->mX;
                y = h->mY;
                if (mLevel->findNearbyUnoccupiedSquare (&x, &y)) {
                    return 0;
                }
                mSpellTimer = Clock + FULLTURN;
            } else {
                continue;
            }
            if (forbidden) {
                if (h->canSee (this)) {
                    I->p ("%s flickers for a moment.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            res = transport (x, y);
            return (-1 == res) ? -1 : FULLTURN;
        case kPsionicStorm:
            if (distance (mX, mY, h->mX, h->mY) <= 5 * 4 + 2)
                continue; /* Too close, would catch self in blast range. */
            if (forbidden) {
                if (h->canSee (this)) {
                    I->p ("%s seems to be seething with anger.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            mSpellTimer = Clock + RNG (10 * FULLTURN);
            return this->psionicStorm (h->mX, h->mY);
        case kTheVoice:
            if (!canSee (h) or !Level->existsLOS (mX, mY, h->mX, h->mY))
                continue;
            return theVoice (h->mX, h->mY);
        case kTerrify:
            if (h->is (kFrightened) or !canSee (h) or
                distance (h, mX, mY) >= 40)
            {
                continue;
            }
            if (forbidden) {
                if (h->canSee (this)) {
                    I->p ("%s begins to concentrate but immediately stops it.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            if (h->canSee (this) and numHands ()) {
                I->p ("%s concentrates.", who);
            }
            h->terrify (NDX (2, 10), 3);
            mSpellTimer = Clock + 10 * FULLTURN;
            return FULLTURN;
        case kDarkness:
            if (RNG (4)) /* a rarely used power */
                continue;
            if (!canSee (h)) {
                continue;
            }
            if (forbidden) {
                mSpellTimer = Clock + 2 * FULLTURN;
                return FULLTURN;
            }
            if (mLevel->isLit (mX, mY, h->mX, h->mY)) {
                x0 = mX;
                y0 = mY;
                if (h->canSee (this))
                    I->p ("%s is surrounded by darkness!", who);
            } else if (mLevel->isLit (h->mX, h->mY, h->mX, h->mY)) {
                x0 = h->mX;
                y0 = h->mY;
                I->p ("You are surrounded by darkness!");
            } else {
                continue;
            }
            for (y = y0 - 5; y <= y0 + 5; y++) {
                for (x = x0 - 5; x <= x0 + 5; x++) {
                    if (distance (x, y, x0, y0) <= 25 and
                        mLevel->existsLOS (x, y, x0, y0))
                    {
                        mLevel->setLit (x, y, -1, -1, -1, -1);
                    }
                }
            }
            if (h->canSee (this)) {
                I->drawScreen ();

            }
            mSpellTimer = Clock + 2 * FULLTURN;
            return FULLTURN;
        case kCeaseAndDesist:
            if (!areAdjacent (this, h) or h->isHelpless ())
                continue;
            I->p ("%s reads a cease and desist letter to you!", who);
            if (h->intr (kBrainShielded) or !RNG (4)) {
                I->p("But you ignore it.");
            }  else {
                h->inflict (kParalyzed, FULLTURN * NDX (2, 6));
            }
            mSpellTimer = Clock + 2 * FULLTURN;
            return FULLTURN;
        case kSeizeEvidence:
            if (!areAdjacent (this, h) or is (kFleeing))
                continue;
            /* Frozen backpack is inaccessible. */
            if (h->is (kFrozen))
                continue;
            /* Hard for lawyer to seize your stuff unless you're helpless. */
            if ((h->is (kParalyzed) or h->is (kAsleep)) and RNG (4))
                continue;

            { /* else */
                I->p ("%s rifles through your pack!", who);

                shObjectVector v;
                selectObjectsByFlag (&v, h->mInventory, obj::cracked);
                if (v.count ()) {
                    int i = selectObjects (&v, h->mInventory, computer);
                    I->p ("%s seizes your pirated software%s", who,
                          i > 1 ? " and your computers!" :
                          i == 1 ? " and your computer!" : "!");
                    for (i = 0; i < v.count (); ++i) {
                        shObject *obj = v.get (i);
                        h->removeObjectFromInventory (obj);
                        addObjectToInventory (obj);
                    }
                    inflict (kFleeing, RNG (50 * FULLTURN));
                }
            }
            mSpellTimer = Clock + FULLTURN;
            return LONGTURN;
        case kSueForDamages:
            if (!areAdjacent (this, h) or is (kFleeing))
                continue;

            I->p ("%s sues you for damages!", who);
            x = h->loseMoney (NDX (mCLevel, 100));
            if (x) {
                gainMoney (x);
            } else {
                I->p ("But you're broke!");
                inflict (kFleeing, RNG (50 * FULLTURN));
            }
            mSpellTimer = Clock + FULLTURN;
            return FULLTURN;
        case kSummonWitness:
            continue; /* FIXME: BROKEN POWER! */
#if 0
            if (!areAdjacent (this, h) or is (kFleeing))
                continue;
            {
                shMonId ilk;

                do {
                    ilk = pickAMonsterIlk(RNG((Level->mDLevel+mCLevel+1)/2));
                } while (!ilk);
                shCreature *mon = monster (ilk);
                x = h->mX;
                y = h->mY;
                if (Level->findNearbyUnoccupiedSquare (&x, &y)) {
                    /* nowhere to put it */
                    delete mon;
                    res = transport (-1, -1);
                    return (-1 == res) ? -1 : FULLTURN;
                }
                if (Level->putCreature (mon, x, y)) {
                    /* shouldn't happen */
                    return FULLTURN;
                }
                mon->mDisposition = kHostile;
                I->p ("%s summons a hostile witness!", who);
                mSpellTimer = Clock + FULLTURN;
                return FULLTURN;
            }
#endif
        case kLaunchMissile:
            // FIXME: This is unprepared to work as pet power.
            if (areAdjacent (this, h) or !canSee (h))
                continue;
            {
                int nx = h->mX - this->mX;
                if (nx)
                    nx /= abs (nx);
                nx += this->mX;
                int ny = h->mY - this->mY;
                if (ny)
                    ny /= abs (ny);
                ny += this->mY;
                if (!Level->isFloor (nx, ny) or Level->isOccupied (nx, ny))
                    continue;
                shCreature *msl = monster (kMonSmartMissile);
                msl->mConditions |= kNoExp;
                Level->putCreature (msl, nx, ny);
                if (isPet ()) {
                    msl->makePet ();
                }
                if (h->canSee (msl)) {
                    I->p ("%s launches %s!", THE (this), AN (msl));
                } else {
                    I->p ("You hear a whoosh.");
                }
                mSpellTimer = Clock + 3 * FULLTURN;
                return FULLTURN;
            }

        default:
            break;
        }
    }
    return 0;
}

using namespace abil;
#define FAR kMFFarsenses
#define TEL kMFTelekinesis
#define CRE kMFCreativity
#define RED kMFRedaction
#define COE kMFCoercion
#define TRA kMFTranslation
/* Make sure these agree with the enum defined in Mutant.h: */
shMutantPowerData MutantPowers[kMaxHeroPower] =
/* LVL LETR ABIL SKILL      NAME         PROCEDURES */
{ { 0, ' ', Psi, kUninitializedSkill, "empty", NULL, NULL },
  { 1, 'i', Con, CRE, "Illumination", useIllumination, NULL },
  { 1, 'd', Con, TRA, "Digestion", useDigestion, NULL },
  { 1, 'h', Psi, COE, "Hypnosis", useHypnosis, NULL },
  { 1, 'r', Con, RED, "Regeneration", useRegeneration, NULL },
  { 1, 'G', Con, FAR, "Gammasensitivity", useGammaSight, useGammaSight },
  { 1, 'v', Con, CRE, "Psi Vomit", usePsiVomit, NULL },
  { 1, 'D', Con, FAR, "Deepsight", useDeepSight, useDeepSight },
  { 2, 'o', Psi, CRE, "Optic Blast", useOpticBlast, NULL },
  { 2, 'H', Psi, RED, "Haste", useHaste, stopHaste },
  { 2, 'T', Int, FAR, "Telepathy", useTelepathy, stopTelepathy },
  { 2, 'R', Con, FAR, "Tremorsense", useTremorsense, stopTremorsense },
  { 2, 'w', Con, CRE, "Web", useWeb, NULL },
  { 2, 'c', Con, TEL, "Hellbent Charge", useCharge, NULL },
  { 2, 'W', Int, TEL, "War Flow", useWarFlow, useWarFlow },
  { 3, 'm', Psi, COE, "Mental Blast", useMentalBlast, NULL},
  { 3, 'n', Con, RED, "Restoration", useRestoration, NULL },
  { 3, 'U', Int, RED, "Understanding", useUnderstanding, stopUnderstanding },
  { 3, 'I', Psi, CRE, "Imbue", useImbue, useImbue },
  { 3, 'k', Psi, TEL, "Psychokinetic Shove", useKinesis, NULL },
  { 3, 'f', Psi, COE, "Cause Fear", useCauseFear, NULL },
  { 4, 'A', Con, RED, "Adrenaline Control", useAdrenaline, stopAdrenaline },
  { 4, 'X', Con, FAR, "X-Ray Vision", useXRayVision, stopXRayVision },
  { 4, 'V', Int, COE, "The Voice", useTheVoice, NULL },
  { 4, 'e', Psi, CRE, "Magnetic Mayhem", useMayhem, NULL },
  { 5, 't', Psi, TRA, "Teleport", useTeleport, NULL },
  { 5, 'B', Psi, RED, "Bene Gesserit Awareness", useAwareness, useAwareness },
  { 5, 's', Psi, TEL, "Shockwave", useShockwave, NULL },
  { 6, 'L', Con, RED, "Autolysis", useAutolysis, stopAutolysis },
  { 6, 'F', Psi, TEL, "Flight", useFlight, stopFlight },
  { 8, 'p', Psi, CRE, "Psionic Storm", usePsionicStorm, NULL }
};
#undef FAR
#undef TEL
#undef CRE
#undef RED
#undef COE
#undef TRA
