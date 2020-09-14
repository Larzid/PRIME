/**********************************************

  Routines for detailed description and lore.

**********************************************/

#include "Creature.h"
#include "Monster.h"
#include "Object.h"
#include "Lore.h"
#include "Hero.h"
#include "ObjectIlk.h"
#include "MonsterIlk.h"
#include "Game.h"
#include <ctype.h>

enum lore_cat
{
    lore_obj,
    lore_mon
};

const encyclopedia_entry*
findLoreEntry (const char *entry, lore_cat type)
{
    const encyclopedia_book *book = glb_encyclopedia[type];
    int n = book->numentries;
    int i = 0;
    while (i < n and strcmp (entry, book->entries[i]->name))
        ++i;
    if (i >= n) {
        return NULL;
    }
    return book->entries[i];
}

/* This routine lifted from DeadCold with permission. */
#define SYL syl[RNG (syllabes)]
static const char *
randomPlanet ()
{
    static const char *syl[] =
    {
        "us","ur","an","ai","pia","ae","ga","nep","it","er",
        "jup","sat","plu","to","ven","ry","gar","del","phi","esc",
        "any","ron","comp","vul","can","ea","a","e","i","o",
        "u","y","cy","ber","tron","nec","ro","mun","da","mon",
        "heim","tal","larn","cad","ia","tuo","mas","bis","kup","mor"
    };
    const int syllabes = sizeof (syl) / sizeof (char *);
    static const char *desig[] =
    {
        "II","III","IV","V","VI","III","IV","V",
        "Alpha","Beta","Gamma","Delta","Prime","Omega","Neo"
    };
    const int numdesig = sizeof (desig) / sizeof (char *);
    char *buf = GetBuf ();
    /* A basic name is two syllables stuck together. */
    sprintf (buf, "%s%s", SYL, SYL);
    buf[0] = toupper (buf[0]);
    /* Uncommon names may have 3 syllables. */
    if ((!RNG (3) and strlen (buf) < 6) or !RNG (10)) {
        strcat (buf, SYL);
    } /* Short names may have a second part. This is uncommon. */
    if (strlen (buf) < 8 and !RNG (23)) {
        strcat (buf, " ");
        strcat (buf, SYL);
        if (RNG (3))
            strcat (buf, SYL);
    } else if ((unsigned) RNG (15) > strlen (buf)) {
        strcat (buf, " ");
        strcat (buf, desig[RNG (numdesig)]);
    }
    return buf;
}
#undef SYL

const char *
randomWorld ()
{
    static const char *planets[] =
    {
        "B-612","Trantos","Coruscant","Tatooine","Delta Pavonis",
        "Aurora","the Ringworld","Phobos","Fribbulus Xax","Arrakis",
        "Limbic System","Gnosticus IV","Asturiax","Giedi Prime",
        "Caladan","Deimos","IX","Sigma Draconis VII","Salusa Secunda",
        "Richese","Aiur","Tarsonis","Mar Sara","Chau Sara","Bhekar Ro",
        "Umoja","Char","Zerus"
    };
    const int numplanets = sizeof (planets) / sizeof (char *);
    if (RNG (3)) return randomPlanet ();
    else         return planets[RNG (numplanets)];
}

const char *
randomRace ()
{
    static const char *races[] =
    {
        "Klingon","Imperial","Fremen","Shikadi","Puppeteer","Oankali",
        "Hiver","Protoss","Zerg","Reticulan","Reptilian","Arcturian",
        "Ur-Quan Kzer-Za","Ur-Quan Kohr-Ah","Hutt","Mandalorean",
        "Predator","Tleilaxan","Melnorme","Keel-Verezy","Human"
    };
    const int numraces = sizeof (races) / sizeof (char *);
    return races[RNG (numraces)];
}

void
fakeProgramLore (shMenu *lore, const char *ilkname)
{ /* Inspired by BOSS floppy disk labels. */
    static const char *noun[] =
    {
        "Programs","Routines","Subroutines","Functions","Procedures",
        "Libraries","Modules","Classes","Binaries","Units","Systems","Addons",
        "Patches","Packages","Software","Executables","Commands","Scripts"
    };
    const int numnouns = sizeof (noun) / sizeof (char *);
    static const char *verb[] =
    {
        "annihilate","analyze","assemble","augment","avoid","build","cleanse",
        "compile","construct","copy","crack","create","delete","detect",
        "derezz","destroy","elude","enhance","enslave","empower","extract",
        "fool","fork","generate","kill","probe","scan","sneak past","spawn",
        "subjugate","summon","survive","terrify","tickle","torture","transform",
        "transport","trash","terminate","unpack","verify"
    };
    const int numverbs = sizeof (verb) / sizeof (char *);
    static const char *numeral[] =
    {
        "a bunch of","a pack of","a lot of","a few","several","some","many",
        "lots of","two","three","four","five","ten","dozens of","tens of",
        "horde of","multitude of"
    };
    const int numnumerals = sizeof (numeral) / sizeof (char *);
    static const char *adjective[] =
    {
        "addictive","alien","amazing","beautiful","bloodthirsty","chaotic",
        "confused","curious","cute","deadly","elite","entrenched","evil",
        "famous","fortified","fresh","friendly","golden","gorgeous","hostile",
        "indifferent","lame","loathsome","lost","meek","molecular","new",
        "obsolete","outdated","peaceful","post-warranty","powerful",
        "radioactive","rainbow","super","tame","unidentified","unwilling",
        "vile","wary","wild"
    };
    const int numadjectives = sizeof (adjective) / sizeof (char *);
    char *buf = GetBuf ();
    char *plural = GetBuf ();
    snprintf (buf, SHBUFLEN, "Download CRACKED software for your %s!", ilkname);
    lore->addText (buf);
    lore->addText ("");
    for (int i = 0; i < 6; ++i) {
        const char *name = NULL;
        if (RNG (2)) {
            int id = RNG (kObjEnergyCell, kObjWreck);
            shObjectIlk *obj = &AllIlks[id];
            if (obj) {
                name = !RNG (20) ? obj->mVague.mName  :
                        RNG  (2) ? obj->mAppearance.mName : obj->mReal.mName;
            } else {
                debug.log ("Error picking a random item (id:%d)", id);
            }
        } else {
            int id = RNG (kMonEarthling, kMonShodan);
            shMonsterIlk *mon = &MonIlks[id];
            if (mon) {
                name = mon->mName;
                if (id == kMonCreepingCredits) {
                    /* Name already in plural. */
                    char *tmp = GetBuf ();
                    strncpy (tmp, name, SHBUFLEN);
                    tmp[15] = '\0'; /* Delete 's'. */
                    name = tmp;
                }
            } else {
                debug.log ("Error picking a random monster (id:%d)", id);
            }
        }
        if (!name) continue; /* Skip a line on error. */
        snprintf (plural, SHBUFLEN, "%s", name);
        makePlural (plural, SHBUFLEN);
        if (RNG (2)) {
            snprintf (buf, SHBUFLEN, "%s %s %s %s %s %s.",
                noun [RNG (numnouns)], RNG (2) ? "to" : "that",
                verb [RNG (numverbs)], numeral [RNG (numnumerals)],
                adjective [RNG (numadjectives)], plural);
        } else {
            snprintf (buf, SHBUFLEN, "%s %s %s %s %s.",
                noun [RNG (numnouns)], RNG (2) ? "to" : "that",
                verb [RNG (numverbs)], adjective [RNG (numadjectives)], plural);
        }
        if (strlen (buf) > 75) {
            --i; /* Retry. */
        } else {
            lore->addText (buf);
        }
    }
    lore->addText ("");
    lore->addText ("(All software provided as is)");
}

/* Replace substrings within asterisks. */
const char *
processLoreLine (const char *src, const char *arg = NULL)
{
    const char *what;
    const char *with;
    do {
        with = NULL;
        if ((what = strstr (src, "*arg*"))) {
            with = arg;
        } else if ((what = strstr (src, "*Arg*"))) {
            char *tmp = GetBuf ();
            strncpy (tmp, arg, SHBUFLEN);
            tmp[0] = toupper (tmp[0]);
            with = tmp;
        } else if ((what = strstr (src, "*args*"))) {
            char *tmp = GetBuf ();
            strncpy (tmp, arg, SHBUFLEN);
            makePlural (tmp, SHBUFLEN);
            with = tmp;
        } else if ((what = strstr (src, "*Args*"))) {
            char *tmp = GetBuf ();
            strncpy (tmp, arg, SHBUFLEN);
            tmp[0] = toupper (tmp[0]);
            makePlural (tmp, SHBUFLEN);
            with = tmp;
        } else if ((what = strstr (src, "*randomrace*"))) {
            with = randomRace ();
        } else if ((what = strstr (src, "*randomworld*"))) {
            with = randomWorld ();
        } else if ((what = strstr (src, "*profession*"))) {
            with = Hero.mProfession->mName;
        } else if ((what = strstr (src, "*heroname*"))) {
            with = Hero.cr ()->mName;
        }
        if (what) {
            char *buf = GetBuf ();
            char *line = GetBuf ();
            int length = (what - src);
            strncpy (buf, src, length);
            buf[length] = '\0'; /* Buf might have had contents. */
            ++what;
            while (*what != '*') ++what;
            ++what;
            snprintf (line, SHBUFLEN, "%s%s%s", buf, with, what);
            src = line;
        }
    } while (what);
    return src;
}

void
warpnetScam (shMenu *lore, const char *arg, const char *header, int numsel,
             const char *ads[], int numads, const char *footer[], int numfooter)
{
    lore->addText (processLoreLine (header, arg));
    lore->addText ("");
    shuffle (ads, numads, sizeof (const char *));
    for (int i = 0; i < numsel; ++i) {
        const char *tmp = processLoreLine (ads[i], arg);
        lore->addText (tmp);
    }
    lore->addText ("");
    for (int i = 0; i < numfooter; ++i) {
        lore->addText (footer[i]);
    }
}

void
fakeWeaponLore (shMenu *lore, const char *wpnname)
{
    static const char *header =
        "*Arg* paradise!   Please visit our sponsored warplinks:";
    static const char *ads[] =
    {
        "*Arg* superior quality upgrade kits!",
        "*Arg* training course and license - 50%% off!",
        "Second-hand Vintage Pre-War *args*!",
        "Fight *arg* battles at *randomworld*!",
        "Surplus *args* at *randomrace* Fleet outlet!"
    };
    const int numads = sizeof (ads) / sizeof (void *);
    static const char *footer[] =
    {
        "(Are you owner of this Dakkapedia article?",
        "Please contact MegaWarpload Hosting Solutions)"
    };
    warpnetScam (lore, wpnname, header, 3, ads, numads, footer, 2);
}

void
fakeGenericLore (shMenu *lore, const char *objname)
{
    static const char *header =
        "Congratulations!  You've won one of the following:";
    static const char *ads[] =
    {
        "FREE Gold-plated *args*!",
        "FREE *Arg* supply voucher for a year!",
        "FREE Hot *randomrace* babes playing with *args* video!",
        "FREE Shipment of *args* to *randomworld*!"
    };
    const int numads = sizeof (ads) / sizeof (void *);
    static const char *footer[] =
    {
        "(By clicking on any of the above warplinks, I,",
        "the user, hereby release PoonTangMedia (Inc.) of any",
        "and all legal liabilities potentially arising thereafter)"
    };
    warpnetScam (lore, objname, header, 3, ads, numads, footer, 3);
}

void
fakeMonsterLore (shMenu *lore, const char *monname)
{
    static const char *header = "Everything *arg*! - Search results:";
    static const char *ads[] =
    {
        "*Arg* petting-zoo near your space-hab!",
        "*Arg* specialized vets!",
        "*Arg* made-to-order costumes!",
        "*Arg* hunting safari at *randomworld*!",
        "*Arg* traditional *randomrace* recipes!"
    };
    const int numads = sizeof (ads) / sizeof (void *);
    static const char *footer[] =
    {
        "(Were you expecting something else on this page?",
        "contact Kim's Warpnet Emporium to lease this WRL)"
    };
    warpnetScam (lore, monname, header, 3, ads, numads, footer, 2);
}

static const encyclopedia_entry *ray_gun_preamble = NULL;
static const encyclopedia_entry *unidentified_ray_gun = NULL;

void
initializeLore ()
{
    const encyclopedia_book *objects = glb_encyclopedia[0];
    int n = objects->numentries;
    int i = 0;
    while (i < n and strcmp ("ray gun preamble", objects->entries[i]->name))
        ++i;
    if (i < n) ray_gun_preamble = objects->entries[i];
    i = 0;
    while (i < n and strcmp ("unidentified ray gun", objects->entries[i]->name))
        ++i;
    if (i < n) unidentified_ray_gun = objects->entries[i];
}

static const char *
nameFooproof (shEnergyType e)
{
    switch (e) {
    case kCorrosive: return "acidproof";
    case kBurning: return "fireproof";
    default: return "bugged";
    }
}

void
analyzeObject (shMenu *lore, shObject *obj)
{
    char *buf = GetBuf ();
    if (BOFH) { /* For debugging purposes. */
        const int props = 8;
        char status[props+1] = "--------";
        static obj::Flag test[] =
        {
            obj::known_appearance,
            obj::known_bugginess,
            obj::known_charges,
            obj::known_enhancement,
            obj::known_infected,
            obj::known_cracked,
            obj::known_type,
            obj::known_fooproof
        };
        for (int i = 0; i < props; ++i)
            if (obj->is (test[i]))
                status[i] = '+';

        snprintf (buf, SHBUFLEN, "Knowledge status: %s", status);
        lore->addText (buf);
        snprintf (buf, SHBUFLEN, "Dmg: %d Bug: %d Flags: %d",
                  obj->mDamage, obj->mBugginess, obj->mFlags);
        lore->addText (buf);
    }

    int pbrk = 0;
    shObjectIlk *ilk = obj->myIlk ();
    int hasAttacks = ilk->mMeleeAttack or ilk->mMissileAttack or
                     ilk->mGunAttack or ilk->mZapAttack;
    if (hasAttacks and obj->is (obj::known_type)) {
        pbrk = 1;

        /* Present melee, missile, gun and zap attacks. */
        for (int pass = 1; pass <= 4; ++pass) {
            shSkill *s = NULL;
            shAttackId atkid = kAttDummy;
            switch (pass) {
            case 1:
                if (ilk->mMeleeAttack) {
                    s = Hero.cr ()->getSkill (ilk->mMeleeSkill);
                    if (s)
                        snprintf (buf, SHBUFLEN, "Skill: %s", s->getName ());
                    else
                        snprintf (buf, SHBUFLEN, "Skill: improvised weapon");
                    atkid = ilk->mMeleeAttack;
                }
                break;
            case 2:
                s = Hero.cr ()->getSkill (kGrenade);
                snprintf (buf, SHBUFLEN, "Skill: %s", s->getName ());
                if (ilk->mMissileAttack) {
                    atkid = ilk->mMissileAttack;
                }
                break;
            case 3:
                if (ilk->mGunAttack) {
                    shSkill *s = Hero.cr ()->getSkill (ilk->mGunSkill);
                    if (s)
                        snprintf (buf, SHBUFLEN, "Skill: %s", s->getName ());
                    else
                        snprintf (buf, SHBUFLEN, "Skill:  HELP!  A BUG!");
                    atkid = ilk->mGunAttack;
                }
                break;
            case 4:
                if (ilk->mZapAttack) {
                    snprintf (buf, SHBUFLEN, "Zappable.");
                    atkid = ilk->mZapAttack;
                }
            }
            if (!atkid) continue;
            shAttack *atk = &Attacks[atkid];
            lore->addText (buf);
            if (pass >= 3) {
                if (atk->mEffect == shAttack::kSingle) {
                    snprintf (buf, SHBUFLEN, "Range: long");
                } else {
                    snprintf (buf, SHBUFLEN, "Range: %d", atk->mRange);
                }
                lore->addText (buf);
                if (atk->mEffect == shAttack::kBurst) {
                    snprintf (buf, SHBUFLEN, "Radius: %d", atk->mRadius);
                    lore->addText (buf);
                }
            }
            snprintf (buf, SHBUFLEN, "%s time: %s",
                pass == 1 ? "Attack" :
                pass == 2 ? "Throwing" :
                pass == 3 ? "Firing" : "Zapping",
                atk->mAttackTime < QUICKTURN ? "very short" :
                atk->mAttackTime <  FULLTURN ? "short" :
                atk->mAttackTime == FULLTURN ? "normal" :
                atk->mAttackTime <= SLOWTURN ? "long" : "very long");
            lore->addText (buf);
            snprintf (buf, SHBUFLEN, "Effect (damage): %s %s", atk->noun (),
                atk->damStr ());
            lore->addText (buf);
            lore->addText ("");
        } /* Show used ammunition. */
        if (ilk->mAmmoType != kObjNothing) {
            shObjectIlk *ammoilk = &AllIlks[ilk->mAmmoType];
            snprintf (buf, SHBUFLEN, "Ammunition used: %s (%d%s)",
                ammoilk->mReal.mName, ilk->mAmmoBurst,
                obj->isSelectiveFireWeapon () ? "/1" : "");
            lore->addText (buf);
        }
    } else if (obj->isA (kArmor) and obj->is (obj::known_appearance)) {
        if (pbrk) lore->addPageBreak ();
        pbrk = 1;
        shObjectIlk *ilk = obj->myIlk ();
        if (obj->isEnhanceable ()) {
            snprintf (buf, SHBUFLEN, "Max enhancement: %+d",
                ilk->mMaxEnhancement);
        } else {
            snprintf (buf, SHBUFLEN, "(not enhanceable)");
        }
        lore->addText (buf);
        if (obj->is (obj::known_enhancement)) {
            snprintf (buf, SHBUFLEN, "Base AC: %d   Provided AC: %d",
                ilk->mArmorBonus,
                maxi (0, ilk->mArmorBonus - obj->mDamage + obj->mEnhancement));
        } else {
            snprintf (buf, SHBUFLEN, "Base AC: %d", ilk->mArmorBonus);
        }
        lore->addText (buf);
        if (ilk->mSpeedBoost and obj->is (obj::known_type)) {
            snprintf (buf, SHBUFLEN, "Speed mod: %d", ilk->mSpeedBoost);
            lore->addText (buf);
        }
        if (obj->isPoweredArmor ()) {
            lore->addText ("");
            lore->addText ("This armor piece is self powered - it will not encumber you if you wear it.");
        }
        /* Information not shown because right now the fact hardly matters.
        if (obj->isSealedArmor ()) {
            lore->addText ("");
            lore->addText ("This armor piece is vacuum sealed.");
        }
        */
    } else if (obj->isA (kArmor)) {
        lore->addText ("You must know appearance of this armor to tell its properties.");
        lore->addPageBreak ();
        return;
    }
    if (obj->is (obj::known_appearance) and obj->isA (kCanister)) {
        snprintf (buf, SHBUFLEN, "Material: %s", matname[ilk->mMaterial]);
        lore->addText (buf);
        switch (ilk->mMaterial) {
        case kGlass:
            lore->addText ("Glass items are fragile and prone to shattering.");
            break;
        case kLead:
            lore->addText ("Lead items safely contain radiation.");
            break;
        default: break;
        }
    }
    /* Vulnerabilities, fooproofness and infectedness. */
    shEnergyType vuln = obj->vulnerability ();
    if (vuln and (obj->isA (kWeapon) or obj->isA (kImplant) or
                  obj->isA (kArmor)))
    {
        lore->addText ("");
        if (obj->is (obj::fooproof)) {
            snprintf (buf, SHBUFLEN, "This object is %s.",
                      nameFooproof (vuln));
            lore->addText (buf);
        } else {
            snprintf (buf, SHBUFLEN, "This object is vulnerable to %s.",
                      energyDescription (vuln));
            lore->addText (buf);
        }
    }
    if (obj->isInfectable ()) {
        lore->addText ("This object type is vulnerable to computer viruses.");
        if (obj->has_subtype (computer)) {
            if (obj->is (obj::fooproof)) {
                lore->addText ("The computer is protected by an antivirus.");
            } else {
                lore->addText ("The computer is not protected by an antivirus.");
                if (obj->is (obj::known_infected | obj::infected)) {
                    lore->addText ("To make matters worse it has a virus.");
                } else if (obj->is (obj::known_infected)) {
                    lore->addText ("Fortunately it has not been infected.  Yet.");
                } else {
                    lore->addText ("However, it might have a virus.");
                }
            }
        } else if (obj->isA (kFloppyDisk) and !obj->isA (kObjBlankDisk)) {
            snprintf (buf, SHBUFLEN, "This object is vulnerable to %s.",
                      energyDescription (kBurning));
            lore->addText (buf);
            if (!obj->is (obj::known_cracked)) {
                lore->addText ("The software could be cracked.");
            } else if (obj->is (obj::cracked)) {
                lore->addText ("The software is cracked.");
            } else {
                lore->addText ("The software is perfectly legal.");
            }
            if (!obj->is (obj::known_infected)) {
                lore->addText ("The software could be infected.");
            } else if (obj->is (obj::infected)) {
                lore->addText ("The software is infected.");
            } else {
                lore->addText ("The software is not infected.");
            }
        }
    }
    if (obj->isA (kCanister) and (!obj->isA (kObjLNO) or !obj->is (obj::known_type))) {
        snprintf (buf, SHBUFLEN, "This object is vulnerable to %s.",
                  energyDescription (kFreezing));
        lore->addText (buf);
    }

    if (obj->isKnownRadioactive ()) {
        lore->addText ("This object is RADIOACTIVE.");
    } else {
        lore->addText ("This object does not seem to be radioactive.");
    }

    if (!obj->is (obj::known_type)) {
        lore->addText ("Identify this item to learn more about it.");
        lore->addPageBreak ();
        return;
    }
    lore->addText ("");
    if (obj->isA (kWeapon)) {
        shObjectIlk *ilk = obj->myIlk ();
        snprintf (buf, SHBUFLEN, "To hit mod: %+d", ilk->mToHitModifier);
        lore->addText (buf);
        snprintf (buf, SHBUFLEN, "Damage mod: %+d",
            (obj->is (obj::known_enhancement) ? obj->mEnhancement : 0)
            - 2 * obj->mDamage);
        lore->addText (buf);
    } else if (obj->isA (kArmor)) {
        shObjectIlk *ilk = obj->myIlk ();
        snprintf (buf, SHBUFLEN, "Psionic mod: %+d", obj->getPsiModifier ());
        lore->addText (buf);
        snprintf (buf, SHBUFLEN, "To hit mod: %+d", ilk->mToHitModifier);
        lore->addText (buf);
        snprintf (buf, SHBUFLEN, "Damage mod: %+d", ilk->mDamageModifier);
        lore->addText (buf);
        lore->addText ("");
        for (int i = 0; i < kMaxEnergyType; ++i) {
            int resist;
            if ((resist = ilk->mResistances[i]) > 0) {
                snprintf (buf, SHBUFLEN, "Grants %s resistance to %s.",
                    resist <=  3 ? "mild" :
                    resist <=  5 ? "some" :
                    resist <= 10 ? "significant" :
                    resist <= 25 ? "strong" :
                    resist <= 50 ? "very strong" : "extreme",
                    energyDescription ((shEnergyType) i));
                lore->addText (buf);
            }
        }
        lore->addText ("");
        const abil::Set *a = obj->getAbilityModifiers ();
        if (a) {
            FOR_ALL_ABILITIES (i) {
                int mod = a->get (i);
                if (mod) {
                    snprintf (buf, SHBUFLEN, "Modifies your %s by %d.",
                        abil::name (Hero.cr ()->myIlk ()->mType, i), mod);
                    lore->addText (buf);
                }
            }
            lore->addText ("");
        }
    }
    /* FIXME: Broken by new intrinsic system.
    int iarr[4];
    iarr[0] = obj->myIlk ()->mCarriedIntrinsics;
    iarr[1] = obj->myIlk ()->mWornIntrinsics;
    iarr[2] = obj->myIlk ()->mWieldedIntrinsics;
    iarr[3] = obj->myIlk ()->mActiveIntrinsics;
    for (int k = 0; k < 4; ++k) {
        bool any = false;
        if (iarr[k]) {
            any = true;
            const char *verb = NULL;
            switch (k) {
            case 0: verb = "carried"; break;
            case 1: verb = "worn"; break;
            case 2: verb = "wielded"; break;
            case 3: verb = "active"; break;
            }
            snprintf (buf, SHBUFLEN, "Grants following intrinsics when %s:", verb);
            lore->addText (buf);
            for (unsigned int i = 1; i <= kNightVision; i<<=1) {
                if (iarr[k] & i) {
                    snprintf (buf, SHBUFLEN, " - %s", nameIntrinsic (i));
                    lore->addText (buf);
                }
            }
        }
        if (any) lore->addText ("");
    }
    */
    snprintf (buf, SHBUFLEN, "Standard market value: $%d", obj->myIlk ()->mCost);
    lore->addText (buf);
    lore->addPageBreak ();
}

void
addTorcLore (shMenu *lore, const encyclopedia_entry *entry, int state)
{
    int mode = 0;
    for (int l = 0; l < entry->numlines; ++l) {
        if (entry->lines[l][0] == ':') {
            if (!strncmp (":all", entry->lines[l], 4) or
                (!strncmp (":golden", entry->lines[l], 7) and state == +1) or
                (!strncmp (":silver", entry->lines[l], 7) and state == 0) or
                (!strncmp (":gray", entry->lines[l], 5) and state == -1))
                 mode = 1;
            else mode = 0;
        } else if (mode) {
            lore->addText (entry->lines[l]);
        }
    }
}

void
addEntry (shMenu *lore, const encyclopedia_entry *entry)
{
    for (int l = 0; l < entry->numlines; ++l) {
        if (!strcmp ("*PAGE BREAK*", entry->lines[l])) {
            lore->addPageBreak ();
        } else if (!strncmp ("*REF:", entry->lines[l], 5)) {
            const char *ref = entry->lines[l] + 5;
            const encyclopedia_entry *new_entry = findLoreEntry (ref, lore_obj);
            if (new_entry) {
                if (l) /* Page break looks silly before first line. */
                    lore->addPageBreak ();
                addEntry (lore, new_entry);
            }
        } else {
            lore->addText (entry->lines[l]);
        }
    }
}

void
shObject::showLore ()
{
    shMenu *lore = I->newMenu (inv (), shMenu::kNoPick);
    analyzeObject (lore, this);
    const char *objname = apparent ()->mName;
    const encyclopedia_entry *objlore = findLoreEntry (objname, lore_obj);
    if (isA (kRayGun)) {
        /* Print ray gun preamble. */
        for (int l = 0; l < ray_gun_preamble->numlines; ++l) {
            lore->addText (ray_gun_preamble->lines[l]);
        }
        lore->addText ("");
    }
    if (!objlore and !isA (kRayGun)) {
        if  (isA (kFloppyDisk)) fakeProgramLore (lore, objname);
        else if (isA (kWeapon)) fakeWeaponLore  (lore, objname);
        else if (has_subtype (keycard))   /* Skip fake lore. */;
        else                    fakeGenericLore (lore, objname);
    } else if (!objlore) {
        addEntry (lore, unidentified_ray_gun);
    } else if (mIlkId == kObjTorc) {
        addTorcLore (lore, objlore, mBugginess);
        lore->finish ();
        delete lore;
        return;
    } else {
        addEntry (lore, objlore);
    }
    lore->finish ();
    delete lore;
}

void
shCreature::showLore ()
{
    shMenu *lore = I->newMenu (myIlk ()->mName, shMenu::kNoPick);
    shMonsterIlk *ilk = myIlk ();
    /* First, raw specimen data. */
    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN,
        "HP: %dd%d (%d-%d, avg. %.2f)  base AC: %d  base speed: %d",
        ilk->mHitDice, 8, ilk->mHitDice, ilk->mHitDice*8, ilk->mHitDice*9.0/2,
        ilk->mNaturalArmorBonus + 10 + ABILITY_MODIFIER (ilk->mAgi)
        + getSizeACMod (getSize ()), ilk->mSpeed);
    lore->addText (buf);
    lore->addText ("");
    for (int j = 0; j < 2; ++j) {
        shMonsterIlk::shAttackData *data;
        if (!j) {
            data = ilk->mAttacks;
            snprintf (buf, SHBUFLEN, "Attacks:%s",
                data[0].mAttId ? "" : " none");
        } else {
            data = ilk->mRangedAttacks;
            snprintf (buf, SHBUFLEN, "Ranged attacks:%s",
                data[0].mAttId ? "" : " none");
        }
        lore->addText (buf);
        for (int i = 0; i < MAXATTACKS; ++i) {
            shAttack *a = &Attacks[data[i].mAttId];
            if (a->mType == shAttack::kNoAttack)  continue;
            snprintf (buf, SHBUFLEN, " - %s (%s) %s",
                a->noun (), a->mAttackTime >  1500 ? "very slow" :
                            a->mAttackTime >  1000 ? "slow" :
                            a->mAttackTime == 1000 ? "normal" :
                            a->mAttackTime >= 750 ? "quick" :
                            a->mAttackTime >= 500 ? "fast" : "blazing",
                a->damStr ());
            lore->addText (buf);
        }
    }
    lore->addPageBreak ();
    /* Next, lore and fluff. */
    const encyclopedia_entry *monlore;
    if (Hero.cr ()->is (kXenosHunter) and isXenos ()) {
        monlore = findLoreEntry ("xenos scum", lore_mon);
        for (int l = 0; l < monlore->numlines; ++l) {
            lore->addText (monlore->lines[l]);
        }
        lore->addPageBreak ();
    }
    monlore = findLoreEntry (myIlk ()->mName, lore_mon);
    if (!monlore) {
        fakeMonsterLore (lore, myIlk ()->mName);
    } else {
        for (int l = 0; l < monlore->numlines; ++l) {
            const char *line = processLoreLine (monlore->lines[l]);
            lore->addText (line);
        }
    }
    lore->finish ();
    delete lore;
}
