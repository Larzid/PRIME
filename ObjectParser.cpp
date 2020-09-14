#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "Global.h"
#include "Object.h"



static char *
consumeChance (char *buf, int *result)
{
    char *start = buf;
    int n = 0;
    *result = 100;
    while (isspace (*buf)) ++buf;
    while (isdigit (*buf)) {
        n = 10 * n + *buf - '0';
        ++buf;
    }
    if ('%' != *buf++) {
        return start;
    }
    if (n) {
        *result = n;
        if (!(0 == *buf or isspace (*buf))) {
            return start;
        }
    }
    while (isspace (*buf)) ++buf;
    return buf;
}


static char *
consumeArticle (char *buf)
{
    while (isspace (*buf)) ++buf;

    if (0 == strncmp ("the ", buf, 4)) {
        buf += 4;
    } else if (0 == strncmp ("a ", buf, 2)) {
        buf += 2;
    } else if (0 == strncmp ("an ", buf, 3)) {
        buf += 3;
    }
    while (isspace (*buf)) ++buf;
    return buf;
}


static char *
consumeCount (char *buf, int *result)
{
    int n = 0;
    int d = 0;
    while (isspace (*buf)) ++buf;
    while (isdigit (*buf)) {
        n = 10 * n + *buf - '0';
        ++buf;
    }
    if (n) {
        if ('d' == *buf) {
            ++buf;
            while (isdigit (*buf)) {
                d = 10 * d + *buf - '0';
                ++buf;
            }
            if (0 == d) {
                return NULL;
            }
            n = NDX (n, d);
        }
        if (!(0 == *buf or isspace (*buf))) {
            return NULL;
        }
        *result = n;
    } else {
        buf = consumeArticle (buf);
    }
    while (isspace (*buf)) ++buf;
    return buf;
}


static char *
consumeBugginess (char *buf, int *result)
{
    while (isspace (*buf)) ++buf;
    if (0 == strncmp ("buggy ", buf, 6)) {
        *result = -1;
        buf += 6;
    } else if (0 == strncmp ("debugged ", buf, 9)) {
        *result = 0;
        buf += 9;
    } else if (0 == strncmp ("optimized ", buf, 10)) {
        *result = +1;
        buf += 10;
    } else if (0 == strncmp ("not buggy ", buf, 10)) {
        *result = RNG (0, 1);
        buf += 10;
    } else if (0 == strncmp ("gray ", buf, 4)) {
        *result = -1;
        buf += 4;
    } else if (0 == strncmp ("silver ", buf, 6)) {
        *result = 0;
        buf += 6;
    } else if (0 == strncmp ("golden ", buf, 6)) {
        *result = +1;
        buf += 6;
    } else { /* Random. */
        *result = -2;
    }
    while (isspace (*buf)) ++buf;
    return buf;
}


static char *
consumeEnhancement (char *buf, int *result)
{
    int n = 0;
    while (isspace (*buf)) ++buf;
    if ('+' == *buf) {
        while (isdigit (*++buf)) {
            n = 10 * n + *buf - '0';
        }
        *result = n;
    } else if ('-' == *buf) {
        while (isdigit (*++buf)) {
            n = 10 * n + *buf - '0';
        }
        *result = -n;
    } else {
        *result = -22;
        return buf;
    }
    if (!(0 == *buf or isspace (*buf))) {
        return NULL;
    } else {
        return buf;
    }
}


static char *
consumeFooproof (char *buf, int *result)
{
    while (isspace (*buf)) ++buf;
    if (0 == strncmp ("fireproof ", buf, 10)) {
        *result = 1;
        buf += 10;
    } else if (0 == strncmp ("acidproof ", buf, 10)) {
        *result = 1;
        buf += 10;
    } else if (0 == strncmp ("protected ", buf, 10)) {
        *result = 1;
        buf += 10;
    } else if (0 == strncmp ("not protected ", buf, 14)) {
        *result = 0;
        buf += 14;      /* The two below for bofh mode. */
    } else if (0 == strncmp ("fooproof ", buf, 9)) {
        *result = 1;
        buf += 9;
    } else if (0 == strncmp ("not fooproof ", buf, 13)) {
        *result = 0;
        buf += 13;
    }
    while (isspace (*buf)) ++buf;
    return buf;
}


static char *
consumeCracked (char *buf, int *result)
{
    while (isspace (*buf)) ++buf;
    if (0 == strncmp ("cracked ", buf, 8)) {
        *result = 1;
        buf += 8;
    } else if (0 == strncmp ("not cracked ", buf, 12)) {
        *result = 0;
        buf += 12;
    }
    while (isspace (*buf)) ++buf;
    return buf;
}

static char *
consumeInfected (char *buf, int *result)
{
    while (isspace (*buf)) ++buf;
    if (0 == strncmp ("infected ", buf, 9)) {
        *result = 1;
        buf += 9;
    } else if (0 == strncmp ("clean ", buf, 6)) {
        *result = 0;
        buf += 6;
    }
    while (isspace (*buf)) ++buf;
    return buf;
}

/* Generating damaged items can be useful for thematic reasons. */
static char *
consumeDamage (char *buf, int *result)
{
    while (isspace (*buf)) ++buf;
    /* Check for damage level first. */
    if (0 == strncmp ("very ", buf, 5)) {
        *result = 2;
        buf += 5;
    } else if (0 == strncmp ("extremely ", buf, 10)) {
        *result = 3;
        buf += 10;
    } else {    /* Assume single point of damage. */
        *result = 1;
    }
    /* Damage type does not matter really. But for elegance: */
    if (0 == strncmp ("burnt ", buf, 6)) {
        buf += 6;
    } else if (0 == strncmp ("corroded ", buf, 9)) {
        buf += 9;   /* Meant for bofh mode. */
    } else if (0 == strncmp ("damaged ", buf, 8)) {
        buf += 8;
    } else if (0 == strncmp ("not damaged ", buf, 12)) {
        *result = 0;
        buf += 12;
    } else {
        *result = -22;
    }
    while (isspace (*buf)) ++buf;
    return buf;
}


static char *
consumeCharges (char *buf, int *result)
{
    char *start = buf;
    int n = 0;
    bool interpret = false;
    while (isspace (*buf))  ++buf;
    if ('(' != *buf++) {
        return start;
    }
    while (isdigit (*buf)) {
        n = 10 * n + *buf - '0';
        ++buf;
        interpret = true;
    }
    bool chrgstr = false;
    if (!strncmp (buf, " charge) ", 9)) {
        buf += 9;
        chrgstr = true;
    }
    if (!chrgstr and !strncmp (buf, " charges) ", 10)) {
        buf += 10;
        chrgstr = true;
    }
    if (!chrgstr)  return start;
    if (interpret)  *result = n;
    while (isspace (*buf))  ++buf;
    return buf;
}

static char *
consumeColor (char *buf, int *result)
{
    char *back = buf;
    while (isspace (*buf)) ++buf;
    if (0 == strncmp ("red ", buf, 4)) {
        *result = 0;
        buf += 4;
    } else if (0 == strncmp ("green ", buf, 6)) {
        *result = 1;
        buf += 6;
    } else if (0 == strncmp ("blue ", buf, 5)) {
        *result = 2;
        buf += 5;
    }
    while (isspace (*buf)) ++buf;
    if (0 != strncmp ("lightsaber", buf, 10)) {
        *result = -22; /* False alarm. */
        buf = back;
        /* Test strings need include "lightsaber" to protect against  */
    }   /* triggering on chaos power armors or other similar objects. */
    return buf;
}

static struct shIlkKey
{
    const char *mName;
    shObjId mId;
} lookup[kObjNumIlks];

int
compareIlks (const void *i1, const void *i2)
{
    struct shIlkKey *ilk1 = (struct shIlkKey *) i1;
    struct shIlkKey *ilk2 = (struct shIlkKey *) i2;
    return strcasecmp (ilk1->mName, ilk2->mName);
}

void
initializeParser ()
{   /* Ignore kObjNothing. */
    for (int i = 1; i < kObjNumIlks; ++i) {
        lookup[i-1].mName = AllIlks[i].mReal.mName;
        lookup[i-1].mId = AllIlks[i].mId;
    }
    qsort (lookup, kObjNumIlks - 1, sizeof (struct shIlkKey), compareIlks);
}

shObjId
consumeIlk (const char *ilkdesc)
{
    struct shIlkKey find;
    find.mName = ilkdesc;
    struct shIlkKey *key = (struct shIlkKey *) bsearch (&find, lookup,
        kObjNumIlks - 1, sizeof (struct shIlkKey), compareIlks);
    if (key) return key->mId;
    return kObjNothing;
}

/* This plural routine is stolen^H^H^H^H^H^H taken from NetHack objnam.c
 * file.  It is pretty much copied whole except handling of some japanese
 * and fantasy stuff was removed because PRIME is not going to need it.
 * Other modifications are minor.
 *
 * Big thanks to whole NetHack DevTeam and contributors.
 */
static char *
makePluralNH (const char *oldstr)
{
	const char vowels[] = "aeiouAEIOU";
	/* Note: cannot use strcmpi here -- it'd give MATZot, CAVEMeN,... */
	char *spot;
	char buf[SHBUFLEN];
	char *str = buf;
	const char *excess = (char *)0;
	int len;

	while (*oldstr==' ') oldstr++;
	if (!oldstr or !*oldstr) {
		strcpy(str, "s");
		return str;
	}
	strcpy(str, oldstr);

	/*
	 * Skip changing "pair of" to "pairs of".  According to Webster, usual
	 * English usage is use pairs for humans, e.g. 3 pairs of dancers,
	 * and pair for objects and non-humans, e.g. 3 pair of boots.  We don't
	 * refer to pairs of humans in this game so just skip to the bottom.
	 */
	if (!strncmp(str, "pair of ", 8))
		goto bottom;

	/* Search for common compounds, ex. lump of royal jelly */
	for(spot=str; *spot; spot++) {
		if (!strncmp(spot, " of ", 4)
				or !strncmp(spot, " labeled ", 9)
				or !strncmp(spot, " called ", 8)
				or !strncmp(spot, " named ", 7)
				or !strncmp(spot, " versus ", 8)
				or !strncmp(spot, " from ", 6)
				or !strncmp(spot, " in ", 4)
				or !strncmp(spot, " on ", 4)
				or !strncmp(spot, " a la ", 6)
				or !strncmp(spot, " with", 5)	/* " with "? */
				or !strncmp(spot, " de ", 4)
				or !strncmp(spot, " d'", 3)
				or !strncmp(spot, " du ", 4)) {
			excess = oldstr + (int) (spot - str);
			*spot = 0;
			break;
		}
	}
	spot--;
	while (*spot==' ') spot--; /* Strip blanks from end */
	*(spot+1) = 0;
	/* Now spot is the last character of the string */

	len = strlen(str);

	/* Single letters (note: letter() replaced by isalpha()) */
	if (len==1 or !isalpha(*spot)) {
		strcpy(spot+1, "'s");
		goto bottom;
	}

	/* Same singular and plural; mostly Japanese words */
	if ((len == 2 and !strcmp(str, "bo")) or
	    (len >= 4 and (!strcmp(spot-4, "teef") or
            !strcmp (spot-4, "fuel") or
	    (len >= 5 and (!strcmp(spot-5, "sheep") or
			!strcmp (spot-5, "ninja") or
			!strcmp (spot-5, "ronin") or
		(len >= 7 and (!strcmp(spot-7, "shuriken") or
            !strcmp (spot-7, "melnorme"))))))))
		goto bottom;

	/* man/men ("Wiped out all cavemen.") */
	if (len >= 3 and !strcmp(spot-2, "man") and
			(len<6 or strcmp(spot-5, "shaman")) and
			(len<5 or strcmp(spot-4, "human"))) {
		*(spot-1) = 'e';
		goto bottom;
	}

	/* tooth/teeth */
	if (len >= 5 and !strcmp(spot-4, "tooth")) {
		strcpy(spot-3, "eeth");
		goto bottom;
	}

	/* knife/knives, etc... */
	if (!strcmp(spot-1, "fe")) {
		strcpy(spot-1, "ves");
		goto bottom;
	} else if (*spot == 'f') {
		if (strchr("lr", *(spot-1)) or strchr(vowels, *(spot-1))) {
			strcpy(spot, "ves");
			goto bottom;
		} else if (len >= 5 and !strncmp(spot-4, "staf", 4)) {
			strcpy(spot-1, "ves");
			goto bottom;
		}
	}

	/* foot/feet (body part) */
	if (len >= 4 and !strcmp(spot-3, "foot")) {
		strcpy(spot-2, "eet");
		goto bottom;
	}

	/* ium/ia (mycelia, baluchitheria) */
	if (len >= 3 and !strcmp(spot-2, "ium")) {
		*(spot--) = (char)0;
		*spot = 'a';
		goto bottom;
	}

	/* algae, larvae, hyphae (another fungus part) */
	if ((len >= 4 and !strcmp(spot-3, "alga")) or
	    (len >= 5 and
	     (!strcmp(spot-4, "hypha") or !strcmp(spot-4, "larva")))) {
		strcpy(spot, "ae");
		goto bottom;
	}

	/* fungus/fungi, homunculus/homunculi, but buses, lotuses, wumpuses */
	if (len > 3 and !strcmp(spot-1, "us") and
	    (len < 5 or (strcmp(spot-4, "lotus") and strcmp(spot-4, "torus") and
			 (len < 6 or strcmp(spot-5, "wumpus"))))) {
		*(spot--) = (char)0;
		*spot = 'i';
		goto bottom;
	}

	/* vortex/vortices */
	if (len >= 6 and !strcmp(spot-3, "rtex")) {
		strcpy(spot-1, "ices");
		goto bottom;
	}

	/* sis/ses (nemesis) */
	if (len >= 3 and !strcmp(spot-2, "sis")) {
		*(spot-1) = 'e';
		goto bottom;
	}

	/* mouse/mice,louse/lice (not a monster, but possible in food names) */
	if (len >= 5 and !strcmp(spot-3, "ouse") and strchr("MmLl", *(spot-4))) {
		strcpy(spot-3, "ice");
		goto bottom;
	}

	/* child/children (for wise guys who give their food funny names) */
	if (len >= 5 and !strcmp(spot-4, "child")) {
		strcpy(spot, "dren");
		goto bottom;
	}

	/* note: -eau/-eaux (gateau, bordeau...) */
	/* note: ox/oxen, VAX/VAXen, goose/geese */

	/* Ends in z, x, s, ch, sh; add an "es" */
	if (strchr("zxs", *spot)
			or (len >= 2 and *spot=='h' and strchr("cs", *(spot-1)))
	/* Kludge to get "tomatoes" and "potatoes" right */
			or (len >= 4 and !strcmp(spot-2, "ato"))) {
		strcpy(spot+1, "es");
		goto bottom;
	}

	/* Ends in y preceded by consonant (note: also "qu") change to "ies" */
	if (*spot == 'y' and
	    (!strchr(vowels, *(spot-1)))) {
		strcpy(spot, "ies");
		goto bottom;
	}

	/* Default: append an 's' */
	strcpy(spot+1, "s");

bottom:
    if (excess) {
        while (*str) ++str;
        strcpy(str, excess);
        str = buf;
    }
	return str;
}

/* Wrapper for makePluralNH to avoid chopping up
   NetHack's original makeplural routine too much. */
void
makePlural (char *buf, int len)
{
    int blen = strlen (buf);
    if (blen + 1 >= len) {
        return;
    }

    char *newbuf = makePluralNH (buf);
    strcpy (buf, newbuf);
}

/* test for compound noun such as "canisters of superglue" */
const static char *compoundnouns[] = {
    " of ",
    " labeled ",
    " called ",
    " named ",
    NULL
};

/* MODIFIES: writes the singular form of the noun in buf into result */
char *
makeSingular (char *buf,
              char *result, int len) /* result: a buffer of length len */
{
    const char *t;
    char save;

    while (isspace (*buf)) ++buf;

    /* Check for each join word of compound nouns. */
    for (int i = 0; (t = compoundnouns[i]); ++i) {
        char *mid = strstr (buf + 1, t);
        if (mid) { /* overwrite the 's' */
            debug.log ("found compound %s", t);
            --mid;
            save = *mid;
            if ('s' == save) {
                *mid = 0;
                snprintf (result, len, "%s%s", buf, mid + 1);
                *mid = save;
            } else {
                snprintf (result, len, "%s", buf);
            }
            return result;
        }
    }

    /* assume the straightforward case of a noun ending in 's' */
    int blen = strlen (buf) - 1;
    while (blen and isspace(buf[blen])) {
        --blen;
    }
    if ('s' == buf[blen]) {
        snprintf (result, len, "%s", buf);
        result[blen] = 0;
    } else { /* assume it's singular already; copy verbatim */
        snprintf (result, len, "%s", buf);
    }
    return result;
}


shObject *
createObject (const char *desc)
{
    int count = -22;
    int bugginess = -2;
    int enhancement = -22;
    int fooproof = -22;
    int cracked = -22;
    int infected = -22;
    int charges = -22;
    int damage = -22;
    int color = -22;
    char ilkdesc[50] = "";
    shObject *obj;
    char buffer[256];
    char *str;
    int chance;

    /* make a copy of the string */

    strncpy (buffer, desc, 255);
    buffer[255] = 0;

    //for (str = buffer; *str; str++)
    //    *str = tolower (*str);
    str = buffer;

    debug.log ("parsing %s", str);

    str = consumeChance (str, &chance);
    //debug.log ("Chance %d, %s", chance, str);
    if (RNG(100) > chance or NULL == str) {
        return NULL;
    }
    str = consumeCount (str, &count);
    //debug.log ("Count %d, %s", count, str);
    if (!str) return NULL;
    str = consumeBugginess (str, &bugginess);
    //debug.log ("Buggy %d, %s", bugginess, str);
    if (!str) return NULL;
    str = consumeDamage (str, &damage);
    //debug.log ("Damage %d, %s", damage, str);
    if (!str) return NULL;
    str = consumeFooproof (str, &fooproof);
    //debug.log ("Fooproof %d, %s", fooproof, str);
    if (!str) return NULL;
    str = consumeInfected (str, &infected);
    //debug.log ("Infected %d, %s", infected, str);
    if (!str) return NULL;
    str = consumeCracked (str, &cracked);
    //debug.log ("Cracked %d, %s", cracked, str);
    if (!str) return NULL;
    str = consumeEnhancement (str, &enhancement);
    //debug.log ("Enhancement %d, %s", enhancement, str);
    if (!str) return NULL;
    str = consumeCharges (str, &charges);
    //debug.log ("Charges %d, %s", charges, str);
    if (!str) return NULL;
    str = consumeColor (str, &color);
    if (!str) return NULL;

    //debug.log ("remaining: %s", str);
    /* Try chopping 's' from name.  This is bad idea for 'knluckes' and
       'pincers' so if that results in object try again with original string. */
    makeSingular (str, ilkdesc, 50);
    shObjId id = consumeIlk (ilkdesc);
    if (id == kObjNothing) {
        while (isspace (*str))  ++str;
        id = consumeIlk (str);
        if (id == kObjNothing)
            return NULL;
    }

    obj = new shObject (id);

    if (obj) {
        if (count != -22 and obj->canMerge ()) {
            obj->mCount = count;
            if (obj->isA (kProjectile) and obj->isChargeable ())
                obj->mCharges += (count - 1) * obj->myIlk ()->mMaxCharges;
        }
        if (bugginess != -2 and !obj->isBugProof ())
            obj->mBugginess = bugginess;
        if (damage != -22 and obj->vulnerability () != kNoEnergy)
            obj->mDamage = damage;
        if (fooproof != -22 and obj->vulnerability () != kNoEnergy) {
            if (fooproof)  obj->set (obj::fooproof);
            else           obj->clear (obj::fooproof);
        }
        if (cracked != -22 and obj->isCrackable ()) {
            if (cracked)  obj->set (obj::cracked);
            else          obj->clear (obj::cracked);
        }
        if (infected != -22 and obj->isInfectable ()) {
            if (infected and !obj->is (obj::fooproof))
                obj->set (obj::infected);
            else
                obj->clear (obj::infected);
        }
        if (enhancement != -22 and obj->isEnhanceable ())
            obj->mEnhancement = enhancement;
        if (charges != -22 and obj->isChargeable ())
            obj->mCharges = charges;
        if (color != -22 and obj->isA (kObjLightSaber))
            obj->mColor = color;
        return obj;
    }
    return NULL;
}
