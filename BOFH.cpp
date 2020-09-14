/*  It was named God Mode commands before.  However, god mode is suitable for
 *  FPP games.  Obviously PRIME is not one of them.  NetHack uses wizard mode
 *  which fits much better but developers needed something more compatible
 *  with science fiction setting.  Hence Bastard Operator From Hell mode.
 */

#include "Global.h"
#include "Hero.h"
#include "Mutant.h"
#include "Intrinsic.h"
#include "Transport.h"

static void
debugData ()
{
    static const char *filename = "debugdata.txt";
    FILE *spoilerfile = fopen (filename, "w");

    fprintf (spoilerfile, "\n\n\n");
    int collision[40][120];
    memset (collision, 0, 40 * 120 * sizeof (int));
    for (int i = 0; i < kObjNumIlks; ++i) {
        shGlyph *gr = &AllIlks[i].mReal.mGlyph;
        shGlyph *ga = &AllIlks[i].mAppearance.mGlyph;
        shGlyph *gv = &AllIlks[i].mVague.mGlyph;
        ++collision[gr->mTileX][gr->mTileY];
        if (ga->mTileX != gr->mTileX or ga->mTileY != gr->mTileY)
            ++collision[ga->mTileX][ga->mTileY];
        if ((gv->mTileX != gr->mTileX or gv->mTileY != gr->mTileY) and
            (gv->mTileX != ga->mTileX or gv->mTileY != ga->mTileY))
            ++collision[gv->mTileX][gv->mTileY];
    }
    for (int y = 0; y < 120; ++y)
        for (int x = 0; x < 40; ++x) {
            if (collision[x][y] > 1 and x > 0) {
                fprintf (spoilerfile, "Tile (%d,%d) referenced %d times.\n",
                         x, y, collision[x][y]);
                for (int i = 0; i < kObjNumIlks; ++i) {
                    shGlyph *gr = &AllIlks[i].mReal.mGlyph;
                    shGlyph *ga = &AllIlks[i].mAppearance.mGlyph;
                    shGlyph *gv = &AllIlks[i].mVague.mGlyph;
                    if (gr->mTileX == x and gr->mTileY == y)
                        fprintf (spoilerfile, "real : %s\n", AllIlks[i].mReal.mName);
                    if (ga->mTileX == x and ga->mTileY == y)
                        fprintf (spoilerfile, "apprc: %s\n", AllIlks[i].mReal.mName);
                    if (gv->mTileX == x and gv->mTileY == y)
                        fprintf (spoilerfile, "vague: %s\n", AllIlks[i].mReal.mName);
                }
            }
        }

    fclose (spoilerfile);
    I->p ("Tables saved in %s.", filename);
}


void /* FIXME: BROKEN! */
showRooms ()
{
#if 0
    for (int y = 0; y < MAPMAXROWS; y++) {
        for (int x = 0; x < MAPMAXCOLUMNS; x++) {
            if (!Level->isInRoom (x, y)) {
                continue;
            }
            int c = ' ';
            int room = Level->getRoomID (x, y);
            if (room < 10) {
                c = '0' + room;
            } else if (room < 36) {
                c = 'a' + room - 10;
            } else if (room < 62) {
                c = 'A' + room - 36;
            } else {
                c = '*';
            }

            if (Level->stairsOK (x, y))
                c |= ColorMap[kPink];

            Level->drawSq (x, y);
        }
    }
    I->pauseXY (cr ()->mX, cr ()->mY);
#endif
}

void
switchDarkness ()
{
    int hx = Hero.cr ()->mX, hy = Hero.cr ()->mY;
    int room = Level->getRoomID (hx, hy);
    int sx, sy, ex, ey;
    if (room == 0) return;  /* We're in corridor or something. */
    Level->getRoomDimensions (room, &sx, &sy, &ex, &ey);
    /* Switch darkness. */
    if (Level->isLit (hx, hy, hx, hy)) {
        Level->darkRoom (sx, sy, ex, ey);
    } else {
        for (int y = sy; y <= ey; ++y)
            for (int x = sx; x <= ex; ++x)
                Level->setLit (x, y, 1, 1, 1, 1);
    }
}

static shFeature *
buildDoor (const int x, const int y)
{
    shFeature *door = shFeature::newDoor ();
    shMenu *doorFeatures = I->newMenu ("Please select door features:", shMenu::kMultiPick);
    doorFeatures->addIntItem ('c', "closed", 0xFFFF);
    doorFeatures->addIntItem ('a', "automatic opening", shFeature::kAutomatic);
    doorFeatures->addIntItem ('i', "inverted logic", shFeature::kInverted);
    doorFeatures->addIntItem ('m', "malfunction", shFeature::kBerserk);
    doorFeatures->addIntItem ('u', "unknown malfunction", 0xFFFE);
    doorFeatures->addIntItem ('l', "locked", shFeature::kLocked);
    doorFeatures->addIntItem ('b', "broken lock", shFeature::kLockBroken);
    doorFeatures->addIntItem ('f', "force field", shFeature::kMagneticallySealed);
    doorFeatures->addIntItem ('s', "alarm system", shFeature::kAlarmed);
    doorFeatures->addIntItem ('h', "horizontal", shFeature::kHorizontal);
    doorFeatures->addIntItem ('S', "retina scanner", shFeature::kLockRetina);
    doorFeatures->addIntItem ('A', "lock color: A", shFeature::kLock1);
    doorFeatures->addIntItem ('B', "lock color: B", shFeature::kLock2);
    doorFeatures->addIntItem ('C', "lock color: C", shFeature::kLock3);
    doorFeatures->addIntItem ('D', "lock color: D", shFeature::kLock4);
    doorFeatures->addIntItem ('M', "lock color: Master", shFeature::kLockMaster);

    int result, flags = 0;
    while (doorFeatures->getIntResult (&result)) {
        if (result == 0xFFFF) {
            door->mType = shFeature::kDoorClosed;
        } else if (result == 0xFFFE) {
            door->mTrapUnknown = 1;
        } else {
            flags |= result;
        }
    }
    delete doorFeatures;
    door->mDoor.mFlags = flags;
    door->mX = x;
    door->mY = y;
    return door;
}

static shMonId
monster_by_genus (const char genus)
{
    shMenu *menu = I->newMenu ("[~]# _", 0);
    char let = 'a';
    for (int i = 0; i < kMonNumberOf; ++i)
        if (MonIlks[i].mGlyph.mSym == genus)
            menu->addIntItem (let++, MonIlks[i].mName, MonIlks[i].mId);

    if (let == 'a') {
        I->p ("No monsters of %c genus exist.", genus);
        return kMonInvisible;
    }

    shMonId id;
    menu->getIntResult ((int *)(&id));
    delete menu;
    return id ? id : kMonInvisible;
}

static shMonId
monster_by_name (const char *name)
{
    for (int i = 0; i < kMonNumberOf; ++i)
        if (!strcasecmp (MonIlks[i].mName, name))
            return MonIlks[i].mId;

    I->p ("%s not found in monster list.", name);
    return kMonInvisible;
}

static void
wish_up_monster ()
{
    char *desc = GetBuf ();
    I->getStr (desc, 80, "Create what kind of monster?");
    int len = strlen (desc);
    if (!len)
        return;

    shMonId mon_id;
    if (len == 1 and desc[0] != 'I')
        mon_id = monster_by_genus (desc[0]);
    else
        mon_id = monster_by_name (desc);

    if (mon_id == kMonInvisible)
        return;

    shCreature *h = Hero.cr ();
    int x = h->mX, y = h->mY;
    Level->findNearbyUnoccupiedSquare (&x, &y);
    if (!I->getSquare ("Where should it spawn?  (select a location)",
                       &x, &y, -1))
    {
        return;
    }
    shCreature *monster = shCreature::monster (mon_id);
    if (-1 == Level->putCreature (monster, x, y))
        delete monster;
}

void
shHero::useBOFHPower ()
{
    shMenu *menu = I->newMenu ("[~]# _", 0);
    int choice;

    menu->addIntItem ('b', "buff up", 8);
    menu->addIntItem ('d', "diagnostics", 12);
    menu->addIntItem ('i', "identify items", 1);
    menu->addIntItem ('l', "gain level", 2);
    menu->addIntItem ('h', "fully heal, restore psi", 18);
    menu->addIntItem ('f', "create feature", 10);
    menu->addIntItem ('m', "create monster", 3);
    menu->addIntItem ('o', "create object", 4);
    menu->addIntItem ('r', "reveal map", 5);
    menu->addIntItem ('G', "debug file", 17);
    menu->addIntItem ('s', "monster spoilers", 6);
    menu->addIntItem ('t', "transport", 7);
    menu->addIntItem ('T', "level transport", 9);
    menu->addIntItem ('e', "excavate", 19);
    //menu->addIntItem ('R', "show RoomIDs", 11);
    menu->addIntItem ('D', "switch room darkness", 16);
    menu->addIntItem ('M', "choose mutations", 15);
    menu->addIntItem ('F', "alter story flag", 13);
    menu->addIntItem ('v', "toggle invisibility", 14);
    menu->addIntItem ('B', "switch body", 20);

    menu->getIntResult (&choice);
    delete menu;

    shCreature *h = mCreature;
    switch (choice) {
    case 1:
        identifyObjects (-1, 0);
        break;
    case 2:
        h->mXP += 1000;
        h->levelUp ();
        break;
    case 3: wish_up_monster (); break;
    case 4:
    {
        char *desc = GetBuf ();
        I->getStr (desc, 80, "Create what object?");
        if (!strlen (desc))  return;
        shObject *obj = createObject (desc);
        if (obj) {
            if (!h->intr (kBlind)) {
                obj->set (obj::known_appearance);
            }
            if (!h->addObjectToInventory (obj)) {
                I->p ("The object slips from your hands!");
                Level->putObject (obj, h->mX, h->mY);
            }
        } else {
            I->p ("%s not found in object list.", desc);
        }
        break;
    }
    case 5:
        Level->reveal (0);
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            for (int y = 0; y < MAPMAXROWS; ++y) {
                Level->mVisibility[x][y] = 1;
                Level->setLit (x, y);
            }
        }
        I->drawScreen ();
        I->p ("Map revealed.");
        break;
    case 6:
        monsterSpoilers ();
        break;
    case 7:
    {
        int x = -1, y = -1;
        bool success;
        if (I->getSquare ("Transport to what location?", &x, &y, -1))
            success = trn::BOFH (h, x, y);
        else
            break;
        if (!success)
            I->p ("Transport failed - site too densely occupied.");
        break;
    }
    case 8:
    {
        const char *zappo = "optimized (20 charges) disintegration ray gun";
        h->mMaxHP += 100;
        h->mHP = h->mMaxHP;
        h->addObjectToInventory (createObject (zappo));
        break;
    }
    case 9:
    {
        shMenu *lmenu = I->newMenu ("Warp to what level?", 0);
        shMapLevel *L;
        for (int i = 1; i < Maze.count (); i++) {
            L = Maze.get (i);
            char let = i <= 26 ? i + 'a' - 1 : i + 'A' - 27;
            lmenu->addPtrItem (let, L->mName, L, 1);
        }
        lmenu->getPtrResult ((const void **) &L);
        delete lmenu;
        if (L)  Level->warpCreature (h, L);
        break;
    }
    case 10:
    {
        shFeature::Type t;
        int x = h->mX, y = h->mY;
        shMenu *fmenu = I->newMenu ("Create what kind of feature?", 0);
        fmenu->addIntItem ('a', "acid pit trap", shFeature::kAcidPit, 1);
        fmenu->addIntItem ('p', "pit trap", shFeature::kPit, 1);
        fmenu->addIntItem ('s', "sewage pit trap", shFeature::kSewagePit, 1);
        fmenu->addIntItem ('r', "radiation trap", shFeature::kRadTrap, 1);
        fmenu->addIntItem ('A', "ACME sign", shFeature::kACMESign, 1);
        fmenu->addIntItem ('g', "floor grating", shFeature::kFloorGrating, 1);
        fmenu->addIntItem ('h', "hole", shFeature::kHole, 1);
        fmenu->addIntItem ('t', "trap door", shFeature::kTrapDoor, 1);
        fmenu->addIntItem ('v', "vat of sludge", shFeature::kVat, 1);
        fmenu->addIntItem ('d', "door", shFeature::kDoorOpen, 1);
        fmenu->getIntResult ((int *) &t);
        delete fmenu;
        if (!t) {
            return;
        }
        if (0 == I->getSquare ("Put it where? (select a location)",
                               &x, &y, -1))
        {
            return;
        }
        switch (t) {
        case shFeature::kVat:
            Level->addVat (x, y);
            break;
        case shFeature::kDoorOpen:
            Level->addFeature (buildDoor (x, y));
            break;
        default:
            Level->addTrap (x, y, t);
            break;
        }
        break;
    }
    case 11:
        showRooms ();
        return;
    case 12:
    {
        shCreature *c;
        int x = h->mX, y = h->mY;
        if (I->getSquare ("Diagnose whom?", &x, &y, -1)) {
            if ((c = Level->getCreature (x, y))) {
                c->doDiagnostics (0);
            } else {
                I->p ("There is no one here.");
            }
        }
        return;
    }
    case 13:
    {
        char name[50];
        int val;
        I->getStr (name, 30, "Check what story flag?");
        if (strlen (name) == 0) {
            I->p ("Cancelled.");
            return;
        }
        val = getStoryFlag(name);
        if (!val) I->p ("Such story flag has not been set.");
        else      I->p ("Flag has value %d.", val);
        if (I->yn ("Alter it?")) {
            char number[15];
            I->getStr (number, 10, "To what value?");
            int read = sscanf(number, "%d", &val);
            if (strlen (number) == 0 or !read) {
                I->p ("Cancelled.");
                return;
            }
            setStoryFlag(name, val);
            I->p ("Flag set.");
        }
        break;
    }
    case 14:
        h->mInnateIntrinsics.toggle (kInvisible);
        h->computeIntrinsics ();
        break;
    case 15:
    {
        shMenu *fmenu = I->newMenu ("Toggle mutations:", shMenu::kMultiPick);
        char letter = 'a';
        for (int power = kNoMutantPower; power < kMaxHeroPower; ++power)
            /* List implemented but not activated persistent powers. */
            if (MutantPowers[power].mFunc and
                h->mMutantPowers[power] != MUT_POWER_ON)
            {
                fmenu->addIntItem (letter++, MutantPowers[power].mName,
                    power, 1, h->mMutantPowers[power] > 0);
                if (letter == 'z' + 1)  letter = 'A';
                /* Remove powers.  Selected entries will be added back. */
                h->mMutantPowers[power] = 0;
            }
        int input;
        do {
            int index;
            input = fmenu->getIntResult (&index);
            if (index)  h->mMutantPowers[index] = 1;
        } while (input);
        delete fmenu;
        break;
    }
    case 16:
        switchDarkness ();
        break;
    case 17:
        debugData ();
        break;
    case 18:
    {   /* Unblind, restore health and psi drain. */
        int lost;
        if ((lost = h->mAbil.temp (abil::Psi)) < 0)
            h->mAbil.temp_mod (abil::Psi, -lost);

        h->fullHealing (100, 0);
        shCreature::TimeOut *t = h->getTimeOut (BLINDED);
        if (t) {
            t->mWhen = 0;
            h->checkTimeOuts ();
        }
        break;
    }
    case 19:
    {
        int x = h->mX, y = h->mY;
        if (!I->getSquare ("Where? (select a location)", &x, &y, -1))  return;
        Level->dig (x, y);
        break;
    }
    case 20:
    {
        int x = h->mX, y = h->mY;
        if (!I->getSquare ("With whom? (select a creature)", &x, &y, -1)) {
            I->nevermind ();
            return;
        }
        shCreature *c = Level->getCreature (x, y);
        if (!c or c->isHero ()) {
            I->nevermind ();
            return;
        }
        Hero.mCreature = c;
        break;
    }
    }
    I->drawScreen ();
}
