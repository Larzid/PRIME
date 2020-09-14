/*******************************

Skill Management Code

each character has access to a variety of skills

********************************/

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Interface.h"

int
shSkill::getValue () const
{
    return mRanks + mBonus;
}

const char *
shSkill::getName () const
{
    if (mPower)  return getMutantPowerName (mPower);

    switch (mCode) {
    /* Combat */
    case kGrenade:       return "Thrown Weapons";
    case kHandgun:       return "Handguns";
    case kLightGun:      return "Light Guns";
    case kHeavyGun:      return "Heavy Guns";
    case kUnarmedCombat: return "Unarmed Combat";
    case kMeleeWeapon:   return "Basic Melee Weapons";
    case kSword:         return "Swords";
    case kPowerArmor:    return "Power Armor Training";
    /* Adventuring */
    case kConcentration: return "Concentration";
    case kOpenLock:      return "Pick Locks";
    case kRepair:        return "Repair";
    case kSearch:        return "Search";
    case kHacking:       return "Programming";
    case kSpot:          return "Spot";
    /* Metapsychic faculties */
    case kMFFarsenses:   return "Farsenses";
    case kMFTelekinesis: return "Telekinesis";
    case kMFCreativity:  return "Creativity";
    case kMFRedaction:   return "Redaction";
    case kMFCoercion:    return "Coercion";
    case kMFTranslation: return "Translation";

    default:
        return "Unknown!";
    }
}


#if 0
static int
compareSkills (shSkill **a, shSkill **b)
{
    shSkill *s1 = * (shSkill **) a;
    shSkill *s2 = * (shSkill **) b;

    if (s1->mCode < s2->mCode) {
        return -1;
    } else if (s1->mCode > s2->mCode) {
        return 1;
    } else if (s1->mPower < s2->mPower) {
        return -1;
    } else {
        return 1;
    }
}
#endif


void
shSkill::getDesc (char *buf, int len) const
{
    const char *abilname = NULL;

    switch (kSkillAbilityMask & mCode) {
    case kStrSkill: abilname = "Str"; break;
    case kConSkill: abilname = "Con"; break;
    case kAgiSkill: abilname = "Agi"; break;
    case kDexSkill: abilname = "Dex"; break;
    case kIntSkill: abilname = "Int"; break;
    case kPsiSkill: abilname = "Psi"; break;
    }

    int abil;
    if (mCode == kMutantPower)
        abil = MutantPowers[mPower].mAbility;
    else
        abil = Hero.cr ()->mAbil.totl (SKILL_KEY_ABILITY (mCode));
    int mod = mBonus + ABILITY_MODIFIER (abil);
    int max = mAccess * (Hero.cr ()->mCLevel + 1) / 4;
    snprintf (buf, len, "%-26s  %s  %2d/%2d %+3d %+3d  ",
              getName (), abilname, mRanks, max, mod, mod+mRanks);
}


static const char *
prepareHeader (const char *hdr, bool choice)
{
    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN, "%s  %-22s  Abil. Ranks Mod Tot ",
              choice ? "Pick    " : "", hdr);
    return buf;
}


static void
describeSkill (const void *skill)
{
    extern const char *getCodename (const char *); /* Help.cpp */
    assert (skill);
    const shSkill *s = (shSkill *) (skill);
    const char *desc = s->getName ();
    const char *buf = getCodename (desc);

    shTextViewer *viewer = new shTextViewer (buf);
    viewer->show ();
    delete viewer;
}

void
shCreature::editSkills ()
{
    char prompt[50];
    int i;
    int flags = shMenu::kSelectIsPlusOne | shMenu::kCountAllowed | shMenu::kShowCount;
    int navail = 0;
    char buf[70];
    shSkill *skill;
    shSkillCode lastcode = kUninitializedSkill;

//    mSkills.sort (&compareSkills);

    do {
        if (mSkillPoints > 1) {
            snprintf (prompt, 50, "You may make %d skill advancements:",
                      mSkillPoints);
            flags |= shMenu::kMultiPick;
        } else if (mSkillPoints == 1) {
            snprintf (prompt, 50, "You may advance a skill:");
            flags |= shMenu::kMultiPick;
        } else {
            snprintf (prompt, 50, "Skills");
            flags |= shMenu::kNoPick;
        }

        shMenu *menu = I->newMenu (prompt, flags);
        //menu->attachHelp ("skills.txt");
        menu->attachHelp (describeSkill);
        char letter = 'a';

        for (i = 0; i < mSkills.count (); i++) {
            skill = mSkills.get (i);
            skill->getDesc (buf, 70);

            int category = skill->mCode & kSkillTypeMask;
            if (category == kMutantPower and !mMutantPowers[skill->mPower])
                continue; /* Learn the power first. */

            /* Choose header if appropriate. */
            if (category != kMutantPower) {
                if ((lastcode & kSkillTypeMask) != category) {
                    const char *header;
                    switch (category) {
                    case kWeaponSkill:
                        header = "Combat"; break;
                    case kAdventureSkill:
                        header = "Adventuring"; break;
                    case kMetapsychicFaculty:
                        header = "Metapsychic Faculties"; break;
                    default:
                        header = "UNKNOWN"; break;
                    }
                    menu->addHeader (prepareHeader (header, true));
                }
            } else {
                const int mask = (kSkillTypeMask | kSkillNumberMask);
                if ((lastcode & mask) != (skill->mCode & mask)) {
                    char *buf = GetBuf ();
                    /* Transform mutant power skill to metapsychic faculty. */
                    int code = (skill->mCode & kSkillNumberMask) | kMetapsychicFaculty | kPsiSkill;
                    shSkill *temp = new shSkill (shSkillCode (code));
                    snprintf (buf, SHBUFLEN, "Power (%s)", temp->getName ());
                    delete temp;
                    menu->addHeader (prepareHeader (buf, true));
                }
            }

            lastcode = skill->mCode;

            int max = skill->mAccess*(mCLevel+1)/4;
            if (mSkillPoints and max > skill->mRanks) {
                menu->addPtrItem (letter++, buf, skill,
                    mini (mSkillPoints, max - skill->mRanks));
                navail++;
                if (letter > 'z') letter = 'A';
            } else {
                menu->addPtrItem (0, buf, skill);
            }
        }
        int advanced = 0;
        do {
            int count;
            i = menu->getPtrResult ((const void **) &skill, &count);
            if (skill) {
                if (count > mSkillPoints) count = mSkillPoints;
                mSkillPoints -= count;
                skill->mRanks += count;
                advanced++;
            }
        } while (i and mSkillPoints);
        delete menu;
        if (!advanced)
            break;
        computeSkills ();
        computeIntrinsics (); /* For improving Haste for example. */
        I->drawSideWin (this);
    } while (mSkillPoints);
    /* Do computation again to be sure. */
    // TODO: figure out if this paranoia is really needed.
    computeSkills ();
    computeIntrinsics ();
}

void
shCreature::computeSkills ()
{
    for (int i = 0; i < mSkills.count (); ++i) {
        shSkill *s = mSkills.get (i);
        s->mBonus = 0;
    }
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        obj->applyConferredSkills (this);
    }
    if (usesPower (kDeepsight)) {
        int skill = getSkillModifier (kMutantPower, kDeepsight);
        int mod = maxi (1, 1 + skill / 4);
        shSkill *s = getSkill (kRepair);
        s->mBonus += mod;
        s = getSkill (kOpenLock);
        s->mBonus += mod;
        s = getSkill (kHacking);
        s->mBonus += mod;
    }
}

shSkill *
shCreature::getSkill (shSkillCode c, shMutantPower power)
{
    if (power and mMutantPowers[power] == MUT_POWER_ABSENT)
        return NULL;

    for (int i = 0; i < mSkills.count (); ++i) {
        shSkill *s = mSkills.get (i);
        if (power) {
            if (power == s->mPower)
                return s;
        } else if (c == s->mCode)
            return s;
    }
    return NULL;
}


int
shCreature::getSkillModifier (shSkillCode c,
                              shMutantPower power)
{
    int result = 0;
    shSkill *skill;
    abil::Index ability;

    if (c == kMutantPower) {
        skill = getSkill (c, power);
        ability = MutantPowers[power].mAbility;
    } else {
        skill = getSkill (c);
        ability = SKILL_KEY_ABILITY (c);
    }
    result += ABILITY_MODIFIER (mAbil.totl (ability));
    if (skill) {
        result += skill->mRanks + skill->mBonus;
    }
    if (is (kSickened)) {
        result -= 2;
    }
    return result;
}


int
shCreature::getWeaponSkillModifier (shObjectIlk *ilk, shAttack *atk)
{
    shSkillCode c = kNoSkillCode;
    shSkill *skill = NULL;
    abil::Index ability = abil::Dex;
    int result = 0;

    if (NULL == ilk) { /* Barehanded. */
        c = kUnarmedCombat;
    } else {
        c = ilk->getSkillForAttack (atk);
        if (c == kNoSkillCode and atk->isMeleeAttack ()) {
            /* Improvised melee weapon. */
            ability = abil::Str;
            result -= 4;
        }
    }
    if (c != kNoSkillCode) {
        ability = SKILL_KEY_ABILITY (c);
        skill = getSkill (c);
    }
    result += mToHitModifier;
    result += ABILITY_MODIFIER (mAbil.totl (ability));
    /* I have no idea why deviations from kMedium give bonuses. -- MB */
    switch (getSize ()) {
    case kFine: result += 8; break;
    case kDiminutive: result += 4; break;
    case kTiny: result += 2; break;
    case kSmall: result += 1; break;
    case kMedium: break;
    case kLarge: result += 1; break;
    case kHuge: result += 2; break;
    case kGigantic: result += 4; break;
    case kColossal: result += 8; break;
    default:
        I->p ("getWeaponSkillModifier: unknown size!");
        I->p ("Please file a bug report.");
        return -4;
    }

    if (NULL == skill or (0 == skill->mRanks and 0 >= skill->mBonus)) {
        /* inflict a slight penalty for lacking weapon skill,
           since we're not using SRD Feats */
        result -= 2;
    } else {
        result += skill->mRanks + skill->mBonus;
    }
    if (is (kSickened)) {
        result -= 2;
    }
    return result;
}


void
shCreature::gainRank (shSkillCode c, int howmany, shMutantPower power)
{
    shSkill *skill = getSkill (c, power);
    if (!skill) {
        skill = new shSkill (c, power);
        mSkills.add (skill);
    }
    skill->mRanks += howmany;
}


void
shCreature::addSkill (shSkillCode c, int access, shMutantPower power)
{
    shSkill *skill = new shSkill (c, power);
    skill->mAccess = access;
    mSkills.add (skill);
}
