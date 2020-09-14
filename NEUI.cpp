#define NOAUDIO
#include "noteye.h"
#include "noteye-curses.h"
#undef NOAUDIO

#include <ctype.h>
#include <string.h>

#include "NEUI.h"
#include "Hero.h"
#include "Effect.h"

shInterface *
startNotEye (int argc, char **argv)
{
    return new shNotEyeInterface (argc, argv);
}

extern bool mapOn;
extern bool minimapOn;
bool altHeld;

int
altstate (lua_State *ls)
{
    noteye::checkArg (ls, 1, "altstate");
    int alt = noteye::noteye_argInt (ls, 1);
    altHeld = !!alt;
    /* FIXME: Part of "solution" to bug blanking PC glyph.
              For more information see shNotEyeInterface::cursorOnXY () */
    if (altHeld)
        I->cursorAt (80, 0);
    return 0;
}

int
isminimapon (lua_State *ls)
{
    noteye::noteye_retBool (ls, minimapOn);
    return 1;
}

int
ismapon (lua_State *ls)
{
    if (I) {
        noteye::noteye_retBool (ls, mapOn and !altHeld);
    } else {
        noteye::noteye_retBool (ls, false);
    }
    return 1;
}

int
pcloc (lua_State *ls)
{
    noteye::noteye_table_new (ls);
    if (Hero.cr ()) {
        noteye::noteye_table_setInt (ls, "x", Hero.cr ()->mX);
        noteye::noteye_table_setInt (ls, "y", Hero.cr ()->mY);
    } else {
        /* Values used for centering the logo. */
        noteye::noteye_table_setInt (ls, "x", 8);
        noteye::noteye_table_setInt (ls, "y", 5);
    }
    return 1;
}

int
mapcontents (lua_State *ls)
{
    noteye::checkArg (ls, 2, "map_contents");
    int x = noteye::noteye_argInt (ls, 1);
    int y = noteye::noteye_argInt (ls, 2);
    noteye::noteye_table_new (ls);

    shNotEyeInterface *UI = (shNotEyeInterface *) (I);
    /* Tile contents not changed from last call.  Use cached image. */
    if (!UI->mCache[x][y].needs_update) {
        noteye::noteye_table_setInt (ls, "usecache", 1);
        return 1;
    }

    char buf[10];
    for (int i = 0; i < UI->mCache[x][y].tiledata->count (); ++i) {
        sprintf (buf, "%d", i+1);
        noteye::noteye_table_setInt (ls, buf, UI->mCache[x][y].tiledata->get (i));
    }
    UI->mCache[x][y].needs_update = false;
    return 1;
}

int
minimap (lua_State *ls)
{
    noteye::checkArg (ls, 2, "minimap");
    int x = noteye::noteye_argInt (ls, 1);
    int y = noteye::noteye_argInt (ls, 2);

    if (!Level or !Hero.cr ()) { /* Wait a minute, NotEye! */
        noteye::noteye_retInt (ls, kBlack);
        return 1;
    }

    shCreature *h = Hero.cr ();
    shCreature *c = Level->getCreature (x, y);
    shFeature *f = Level->getMemory (x, y).mFeat ?
        Level->getKnownFeature (x, y) : NULL;
    bool seen = Level->getMemory (x, y).mTerr != kMaxTerrainType;
    if (c and c->isHero ()) {
        noteye::noteye_retInt (ls, kYellow);
    } else if (c and h->canSee (c) and c->isPet ()) {
        noteye::noteye_retInt (ls, kGreen);
    } else if (c and h->canSee (c)) {
        noteye::noteye_retInt (ls, kOrange);
    } else if (c and h->isAwareOf (c)) {
        noteye::noteye_retInt (ls, kRed);
    } else if (seen) {
        if (f and f->isStairs ()) {
            noteye::noteye_retInt (ls, kWhite);
        } else if (f and f->isTrap () and !f->mTrapUnknown) {
            noteye::noteye_retInt (ls, kMagenta);
        } else if (f and f->isDoor () and !f->mTrapUnknown) {
            noteye::noteye_retInt (ls, kNavy);
        } else if (f and !f->mTrapUnknown) {
            noteye::noteye_retInt (ls, kLime);
        } else if (Level->getMemory (x, y).mObj) {
            noteye::noteye_retInt (ls, kCyan);
        } else if (!Level->appearsToBeFloor (x, y)) {
            noteye::noteye_retInt (ls, kGray);
        } else {
            noteye::noteye_retInt (ls, kDarkGray);
        }
    } else {
        noteye::noteye_retInt (ls, kBlack);
    }

    return 1;
}

int
grenadedata (lua_State *ls)
{
    extern shFlavor Flavors[];
    const int grenade_offset = 20; /* First free tile in grenade row. */
    noteye::checkArg (ls, 1, "grenade");
    int i = kFFirstGrenade + noteye::noteye_argInt (ls, 1) - grenade_offset;
    noteye::noteye_retInt (ls, Flavors[i].mVague.mGlyph.mTileX);
    noteye::noteye_retInt (ls, Flavors[i].mAppearance.mGlyph.mColor);
    return 2;
}

int
gamecore (lua_State *ls)
{
    noteye::checkArg (ls, 2, "game_core");
    noteye::InternalProcess *IP = noteye::byId<noteye::InternalProcess> (noteye::noteye_argInt (ls, 1), ls);
    noteye::noteye_setinternal (IP, ls, 2);
    mainLoop ();
    IP->exitcode = 0;
    IP->isActive = false;
    return 0;
}

shNotEyeInterface::shNotEyeInterface (int argc, char **argv)
{
    noteye::noteye_args (argc, argv);
    noteye::noteye_init ();

    noteye::noteye_globalfun ("map_contents", mapcontents);
    noteye::noteye_globalfun ("minimap", minimap);
    noteye::noteye_globalfun ("get_pc_coords", pcloc);
    noteye::noteye_globalfun ("is_map_on", ismapon);
    noteye::noteye_globalfun ("is_minimap_on", isminimapon);
    noteye::noteye_globalfun ("alt_state", altstate);
    noteye::noteye_globalfun ("grenade", grenadedata);
    noteye::noteye_globalfun ("game_core", gamecore);

    ColorMap[kBlack] = 0;
    ColorMap[kBlue] = 1;
    ColorMap[kGreen] = 2;
    ColorMap[kCyan] = 3;
    ColorMap[kRed] = 4;
    ColorMap[kMagenta] = 5;
    ColorMap[kBrown] = 6;
    ColorMap[kGray] = 7;
    ColorMap[kDarkGray] = 7;
    ColorMap[kNavy] = 9;
    ColorMap[kLime] = 10;
    ColorMap[kAqua] = 11;
    ColorMap[kOrange] = 12;
    ColorMap[kPink] = 13;
    ColorMap[kYellow] = 14;
    ColorMap[kWhite] = 15;

    mWin[kMain].x1 = 0;
    mWin[kMain].y1 = 0;
    mWin[kMain].x1 = 64;
    mWin[kMain].y1 = 20;
    mWin[kSide].x1 = 64;
    mWin[kSide].y1 = 0;
    mWin[kSide].x2 = 80;
    mWin[kSide].y2 = 20;
    mWin[kLog].x1 = 0;
    mWin[kLog].y1 = 20;
    mWin[kLog].x2 = 80;
    mWin[kLog].y2 = 25;

    for (int y = 0; y < MAPMAXROWS; ++y)
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            mCache[x][y].needs_update = true;
            mCache[x][y].tiledata = new shVector<int>;
        }

    for (int i = 0; i < kMaxWin; ++i) {
        mWin[i].cx = mWin[i].x1;
        mWin[i].cy = mWin[i].y1;
    }

    mColor = kGray;

    mLogSize = 5;
    mLogRow = 0;
    mHistoryIdx = 0;
    mHistoryWrapped = 0;
    mNoNewline = 0;
    mLogSCount = 0;
    mPause = 0;

    mapOn = true;
    minimapOn = true;
}

shNotEyeInterface::~shNotEyeInterface ()
{
    noteye::noteye_finishinternal (1);
    //noteye_uifinish ();
}

void
shNotEyeInterface::runMainLoop ()
{
#ifdef OSX
    setenv("NOTEYEDIR", DATADIR, 1);
    setenv("LUA_PATH", USERDIR, 1);
#endif
    noteye::noteye_run ("lua/prime.lua", true);
}

void
shNotEyeInterface::shCache::add (int x, int y, int spatial, int recolor)
{
    tiledata->add (x);
    tiledata->add (y);
    tiledata->add (spatial);
    tiledata->add (recolor);
}

void
shNotEyeInterface::shCache::dellast ()
{
    int k = tiledata->count () - 1;
    for (int i = 0; i < 4; ++i)
        tiledata->removeByIndex (k--);
}

static bool tempWinClear;

void
shNotEyeInterface::newWin (Window win)
{
    if (win == kTemp)
        tempWinClear = mapOn;
    mapOn = false;
    mWin[win].x1 = 0;
    mWin[win].y1 = 0;
    mWin[win].x2 = 80;
    mWin[win].y2 = 25;
    if (win == kTemp)
        clearWin (win);
}


void
shNotEyeInterface::delWin (Window win)
{
    clearWin (win);
    if (win == kTemp and tempWinClear)  mapOn = true;
    drawScreen ();
}


void
shNotEyeInterface::moveWin (Window win, int x1, int y1, int x2, int y2)
{
    mWin[win].x1 = x1;
    mWin[win].x2 = x2;
    mWin[win].y1 = y1;
    mWin[win].y2 = y2;
}


void
shNotEyeInterface::clearWin (Window win)
{
    for (int y = mWin[win].y1; y < mWin[win].y2; ++y)
        for (int x = mWin[win].x1; x < mWin[win].x2; ++x)
            noteye::noteye_mvaddch (y, x, ' ');
}


void
shNotEyeInterface::refreshWin (Window)
{}


int
shNotEyeInterface::getMaxLines ()
{
    return 25;
}


int
shNotEyeInterface::getMaxColumns ()
{
    return 80;
}


void
shNotEyeInterface::assign (int key, Command cmd)
{
    mKey2Cmd[key] = cmd;
}


void
shNotEyeInterface::resetKeybindings ()
{
    for (int i = 0; i < MAXKEYS; i++)
        mKey2Cmd[i] = kNoCommand;
}


static const struct shNameToKey KeyNames[] =
{
    {"space", ' '},
    {"tab", '\t'},
    {"backspace", 8},
    {"enter", 10},
    {"escape", 27},
    {"down arrow", D_DOWN},
    {"up arrow", D_UP},
    {"left arrow", D_LEFT},
    {"right arrow", D_RIGHT},
    {"keypad 1", D_END},
    {"keypad 2", D_DOWN},
    {"keypad 3", D_PGDN},
    {"keypad 4", D_LEFT},
    {"keypad 5", D_CTR},
    {"keypad 6", D_RIGHT},
    {"keypad 7", D_HOME},
    {"keypad 8", D_UP},
    {"keypad 9", D_PGUP},
    {"home", D_HOME},
    {"end", D_END},
    {"page up", D_PGUP},
    {"page down", D_PGDN},
    {"fn 1", KEY_F0 + 1},
    {"fn 2", KEY_F0 + 2},
    {"fn 3", KEY_F0 + 3},
    {"fn 4", KEY_F0 + 4},
    {"fn 5", KEY_F0 + 5},
    {"fn 6", KEY_F0 + 6},
    {"fn 7", KEY_F0 + 7},
    {"fn 8", KEY_F0 + 8},
    {"fn 9", KEY_F0 + 9},
    {"fn 10", KEY_F0 + 10},
    {"fn 11", KEY_F0 + 11},
    {"fn 12", KEY_F0 + 12}
};
const int n_keys = sizeof (KeyNames) / sizeof (shNameToKey);

void
shNotEyeInterface::readKeybindings (const char *fname)
{
    I->readKeybindings (fname, n_keys, KeyNames);
}


static const char *
keyCodeToString (int code)
{
    if (code >= KEY_F0 + 1 and code <= KEY_F0 + 12) {
        char *buf = GetBuf ();
        sprintf (buf, "F%d", code - KEY_F0 + 2);
        return buf;
    }

    for (int i = 0; i < n_keys; ++i)
        if (KeyNames[i].key == code)
            return KeyNames[i].name;

    if (code < ' ') {
        char *buf = GetBuf ();
        sprintf (buf, "ctrl %c", code + 64);
        return buf;
    } else {
        char *buf = GetBuf ();
        sprintf (buf, "%c", code);
        return buf;
    }
    return "(unkn)";
}


const char *
shNotEyeInterface::getKeyForCommand (Command cmd)
{   /* Find key corresponding to command. */
    int i;
    for (i = 0; i < MAXKEYS and cmd != mKey2Cmd[i]; ++i)
        ;
    if (i == MAXKEYS) /* Unassigned? This is bad. */
        return NULL;  /* Safe, will not lead to crash. */
    return keyCodeToString (i);
}


shVector<const char *> *
shNotEyeInterface::getKeysForCommand (Command cmd)
{
    shVector<const char *> *keys = new shVector<const char *> ();
    for (int i = 0; i < MAXKEYS; ++i) {
        if (cmd == mKey2Cmd[i])  keys->add (keyCodeToString (i));
    }
    return keys;
}


shInterface::Command
shNotEyeInterface::keyToCommand (int key)
{
    return mKey2Cmd[key];
}


bool
shNotEyeInterface::has_vi_keys ()
{
    return mKey2Cmd['l'] == kMoveE;
}

bool
shNotEyeInterface::has_vi_fire_keys ()
{
    return mKey2Cmd[0x1f & 'l'] == kFireE;
}


int
shNotEyeInterface::getChar ()
{
    return noteye::noteye_getch ();
}


int
shNotEyeInterface::getSpecialChar (SpecialKey *sk)
{
    int key = getChar ();
    switch (key) {
    case 27: *sk = kEscape; break;
    case 10: *sk = kEnter; break;
    case ' ': *sk = kSpace; break;
    case 8: *sk = kBackSpace; break;
    case D_HOME: *sk = kHome; break;
    case D_END:  *sk = kEnd; break;
    case D_PGUP: *sk = kPgUp; break;
    case D_PGDN: *sk = kPgDn; break;
    case D_UP: *sk = kUpArrow; break;
    case D_DOWN: *sk = kDownArrow; break;
    case D_LEFT: *sk = kLeftArrow; break;
    case D_RIGHT: *sk = kRightArrow; break;
    case D_CTR: *sk = kCenter; break;
    default: *sk = kNoSpecialKey; break;
    }
    return key;
}


int
shNotEyeInterface::getStr (char *buf, int len, const char *prompt,
                    const char *dflt /* = NULL */ ) /* Default suggestion. */
{
    char msg[80];
    int pos = 0;
    int savehistidx = mHistoryIdx;

    snprintf (msg, 80, "%s ", prompt);
    msg[79] = 0;
    p (msg);
    buf[0] = 0;

    if (dflt) {
        strncpy (buf, dflt, len);
        buf[len-1] = 0;
        winPrint (kLog, buf);
        pos = strlen (buf);
    }

    while (1) {
        drawLog ();
        int c = getChar ();
        if (isprint (c)) {
            if (pos >= len - 1) {
                continue;
            }
            buf[pos++] = c;
            winPutchar (kLog, c);
        } else if ('\n' == c or '\r' == c) {
            break;
        } else if (8 == c and pos) {
            pos--;
            int y, x;
            winGetYX (kLog, &y, &x);
            winPrint (kLog, y, x - 1, " ");
            winGoToYX (kLog, y, x - 1);
        } else if (27 == c) { /* Escape. */
            pos = 0;
            break;
        }/* else {
            debug.log ("getstr: unhandled char %d", c);
        }*/
    }

    buf[pos] = 0;
    snprintf (msg, 80, "%s %s", prompt, buf);
    msg[79] = 0;
    strcpy (&mLogHistory[savehistidx*80], msg);

    return pos;
}


int
shNotEyeInterface::diag (const char *format, ...)
{
    va_list ap;
    va_start (ap, format);
    char dbgbuf[100];
    vsnprintf (dbgbuf, 100, format, ap);
    debug.log ("%s", dbgbuf);
    va_end (ap);
    return 0;
}


void
shNotEyeInterface::cursorOnXY (int x, int y, int curstype)
{
    const int spCursor = spFlat + spMonst + spICeil + spIWallR + spIWallL;
    static bool curson = false; /* Is special cursor being displayed? */
    static int crx, cry; /* Last special cursor position on map. */
    if (curson) {
        /* Any cursors are assumed to be last on stack. */
        int cnt = mCache[crx][cry].tiledata->count ();
        for (int i = 1; i <= 4; ++i) /* This has to reverse shCache::add. */
            mCache[crx][cry].tiledata->removeByIndex (--cnt);
        mCache[crx][cry].needs_update = true;
        curson = false;
    }
    if (curstype) {
        curson = true;
        mCache[x][y].add (curstype, kRowCursor, spCursor);
        mCache[x][y].needs_update = true;
        crx = x;  cry = y;
    }
    /* FIXME: There is a bug causing the spot under cursor to be blanked out.
       I cannot find it at the moment, so here is a workaround. -- MB */
    if (!altHeld)
        noteye::noteye_move (y, x);
    else
        noteye::noteye_move (0, 80);
}


void
shNotEyeInterface::winGoToYX (Window win, int y, int x)
{
    noteye::noteye_move (mWin[win].cy = mWin[win].y1 + y, mWin[win].cx = mWin[win].x1 + x);
}


void
shNotEyeInterface::winGetYX (Window win, int *y, int *x)
{
    *y = mWin[win].cy - mWin[win].y1;  *x = mWin[win].cx - mWin[win].x1;
}


void
shNotEyeInterface::pageLog ()
{
    mLogSCount = 0;
    mLogRow = 0;
    for (int i = 0; i < 5; ++i) {
        winGoToYX (kLog, i, 0);
        noteye::noteye_clrtoeol ();
    }
    winGoToYX (kLog, 0, 0);
    drawLog ();
}


void
shNotEyeInterface::winPutchar (Window win, const char c)
{
    if (c == '\n') {
        noteye::noteye_move (mWin[win].cy += 1, mWin[win].cx = 0);
    } else {
        noteye::noteye_mvaddch (mWin[win].cy, mWin[win].cx++, c);
    }
}


void
shNotEyeInterface::winPrint (Window win, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    noteye::noteye_mvaddstr (mWin[win].cy, mWin[win].cx, linebuf);
    mWin[win].cx += strlen (linebuf);
}


void
shNotEyeInterface::winPrint (Window win, int y, int x, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    noteye::noteye_mvaddstr (mWin[win].y1 + y, mWin[win].x1 + x, linebuf);
    mWin[win].cy = mWin[win].y1 + y;
    mWin[win].cx = mWin[win].x1 + x + strlen (linebuf);
}


void
shNotEyeInterface::setWinColor (Window, shColor fg, shColor bg)
{
    noteye::setTextAttr (fg, bg);
}


void
shNotEyeInterface::doScreenShot (FILE *)
{   /* TODO */   }

void
shNotEyeInterface::showVersion ()
{
    const int BX = 11, EX = 23;
    const int BY = 44, EY = 50;

    for (int y = BY; y <= EY; ++y) {
        for (int x = BX; x <= EX; ++x) {
            mCache[x - BX][y - BY].needs_update = true;
            mCache[x - BX][y - BY].add (x, y, spFlat + spIFloor);
        }
    }
    I->p ("PRIME "PRIME_VERSION" - Astral Navigator");
    I->p ("Necklace of the Eye version "NOTEYEVERSION);
    noteye::noteye_getch ();
    pageLog ();
    for (int y = BY; y <= EY; ++y) {
        for (int x = BX; x <= EX; ++x) {
            mCache[x - BX][y - BY].needs_update = true;
            mCache[x - BX][y - BY].tiledata-> reset ();
        }
    }
}


void
shNotEyeInterface::drawScreen ()
{
    Level->draw ();
    drawSideWin (Hero.cr ());
    //noteye::noteye_refresh ();
}


void shNotEyeInterface::drawLog ()
{
    //noteye::noteye_refresh ();
}

void shNotEyeInterface::refreshScreen ()
{
    noteye::noteye_halfdelayms (5);
    noteye::noteye_getchev ();
    noteye::noteye_cbreak ();
}


const int Floor = spFlat + spFloor + spIFloor + spCeil;
const int spWall = spWallN + spWallE + spWallS + spWallW;
const int Wall = spFlat + spWall + spIWallL + spIWallR + spICeil;
const int spFeature = spFlat + spMonst + spIItem;
const int WayDown = spFlat + spFloor + spIFloor;

void
drawSpecialEffect (eff::type effect, shNotEyeInterface::shCache *cache)
{
    const int OPT = 23; /* Difference between laser and optic blast. */
    const int eff2tile[eff::last_corner][2] =
    {   /* None: */ {5, kRowDefault},
        /* Laser: f-slash, horiz, vert, b-slash, hit. */
        {1, kRow4DirAtt0}, {2, kRow4DirAtt0}, {3, kRow4DirAtt0},
        {4, kRow4DirAtt0}, {5, kRow4DirAtt0},
        /* Optic blast: f-slash, horiz, vert, b-slash, hit. */
        {1+OPT, kRow4DirAtt0}, {2+OPT, kRow4DirAtt0}, {3+OPT, kRow4DirAtt0},
        {4+OPT, kRow4DirAtt0}, {5+OPT, kRow4DirAtt0},
        /* Bolt: f-slash, horiz, vert, b-slash, hit. */
        {10, kRow4DirAtt0}, {11, kRow4DirAtt0}, {12, kRow4DirAtt0},
        {13, kRow4DirAtt0}, {14, kRow4DirAtt0},
        /* Packet storm: SW-NE, E-W, N-S, SE-NW, hit. */
        {15, kRow4DirAtt0}, {16, kRow4DirAtt0}, {17, kRow4DirAtt0},
        {18, kRow4DirAtt0}, {19, kRow4DirAtt0},
        /* Heat ray: SW-NE, E-W, N-S, SE-NW, hit. */
        {10, kRow4DirAtt1}, {11, kRow4DirAtt1}, {12, kRow4DirAtt1},
        {13, kRow4DirAtt1}, {14, kRow4DirAtt1},
        /* Antimatter ray: SW-NE, E-W, N-S, SE-NW, hit. */
        {1, kRow4DirAtt1}, {2, kRow4DirAtt1}, {3, kRow4DirAtt1},
        {4, kRow4DirAtt1}, {5, kRow4DirAtt1},
        /* Railgun: N, NE, E, SE, S, SW, W, NW directions and hit. */
        {3, kRow8DirAtt0}, {1, kRow8DirAtt0}, {6, kRow8DirAtt0},
        {8, kRow8DirAtt0}, {7, kRow8DirAtt0}, {5, kRow8DirAtt0},
        {2, kRow8DirAtt0}, {4, kRow8DirAtt0}, {5, kRowDefault},
        /* Psi vomit: N, NE, E, SE, S, SW, W, NW directions and hit. */
        {23, kRow8DirAtt0}, {21, kRow8DirAtt0}, {26, kRow8DirAtt0},
        {28, kRow8DirAtt0}, {27, kRow8DirAtt0}, {25, kRow8DirAtt0},
        {22, kRow8DirAtt0}, {24, kRow8DirAtt0}, {25, kRowDefault},
        /* Hydralisk: N, NE, E, SE, S, SW, W, NW directions and hit. */
        {3, kRow8DirAtt1}, {1, kRow8DirAtt1}, {6, kRow8DirAtt1},
        {8, kRow8DirAtt1}, {7, kRow8DirAtt1}, {5, kRow8DirAtt1},
        {2, kRow8DirAtt1}, {4, kRow8DirAtt1}, {9, kRow8DirAtt1},
        /* Combi-stick: N, NE, E, SE, S, SW, W, NW, E-W, N-S, SW-NE, SE-NW. */
        {11, kRow8DirAtt0}, { 9, kRow8DirAtt0}, {14, kRow8DirAtt0},
        {16, kRow8DirAtt0}, {15, kRow8DirAtt0}, {13, kRow8DirAtt0},
        {10, kRow8DirAtt0}, {12, kRow8DirAtt0}, {18, kRow8DirAtt0},
        {19, kRow8DirAtt0}, {17, kRow8DirAtt0}, {20, kRow8DirAtt0},
        /* NNTP Daemon flame breath. */
        { 7, kRow0DirAtt}, { 8, kRow0DirAtt}, { 9, kRow0DirAtt},
        {10, kRow0DirAtt}, {11, kRow0DirAtt}, {12, kRow0DirAtt},
        {13, kRow0DirAtt}, {14, kRow0DirAtt},
        /* Invis (should not be used), pea pellet, plasma glob, plasma hit. */
        {5, kRowDefault}, {1, kRow0DirAtt}, {2, kRow0DirAtt}, {3, kRowBoom},
        /* Explosion, frost, poison, radiation. */
        {1, kRowBoom}, {4, kRow0DirAtt}, {6, kRow0DirAtt}, {2, kRowBoom},
        /* Flashbang, incendiary, psi storm, disintegration. */
        {4, kRowBoom}, {5, kRow0DirAtt}, {6, kRowBoom}, {3, kRow0DirAtt},
        /* Shrapnel/frag, acid splash, defiler vomit, water splash. */
        {5, kRowBoom}, {2, kRowSplash}, {1, kRowBreath}, {5, kRowSplash},
        /* Web, Bugs, viruses. */
        {8, kRowBoom}, {5, kRowDefault}, {6, kRow0DirAtt},
        /* Smart-Disc, radar blip, sensed life, sensed tremor. */
        {5, kRowMissile}, {4, kRowCursor}, {5, kRowCursor}, {6, kRowCursor},
        /* last_effect: */ {5, kRowDefault},
        /* Laser beam corners: NW, NE, SE, SE. */
        {6, kRow4DirAtt0}, {7, kRow4DirAtt0}, {8, kRow4DirAtt0},
        {9, kRow4DirAtt0},
        /* Optic blast corners: NW, NE, SE, SE. */
        {6+OPT, kRow4DirAtt0}, {7+OPT, kRow4DirAtt0}, {8+OPT, kRow4DirAtt0},
        {9+OPT, kRow4DirAtt0},
        /* Psionic vomit corners: NW, NE, SE, SE. */
        {30, kRow8DirAtt0}, {31, kRow8DirAtt0}, {32, kRow8DirAtt0},
        {33, kRow8DirAtt0},
        /* Packet storm corners: NW, NE, SE, SE. */
        {20, kRow4DirAtt0}, {21, kRow4DirAtt0}, {22, kRow4DirAtt0},
        {23, kRow4DirAtt0},
        /* Heat ray corners: NW, NE, SE, SE. */
        {15, kRow4DirAtt1}, {16, kRow4DirAtt1}, {17, kRow4DirAtt1},
        {18, kRow4DirAtt1},
        /* Antimatter ray corners: NW, NE, SE, SE. */
        {6, kRow4DirAtt1}, {7, kRow4DirAtt1}, {8, kRow4DirAtt1},
        {9, kRow4DirAtt1}
    };
    /* Stored here as documentation.  Splashes row: */
    /* Coffee, BBB/Acid, Beer, Cola, Water, Sludge, Other, Blood */

    const int Bolt = spFlat + spFree + spIItem;
    const int BoltHit = spFlat + spMonst + spIItem;
    const int Marker = spFlat + spIItem;

    switch (effect) {
    /* Never drawn effects. */
    case eff::none:  case eff::invis:  case eff::rail:
        return;
    /* Here go effects without tiles at the moment.  Don't draw anything. */
    case eff::bugs:
        return;
    /* Hit tiles in bolt row. */
    case eff::laser:  case eff::optic:  case eff::bolt:  case eff::psi_vomit:
        cache->add (eff2tile[effect][0], eff2tile[effect][1], BoltHit);
        return;
    default:
        break;
    }
    int col = eff2tile[effect][0];
    int row = eff2tile[effect][1];
    switch (row) {
    case kRow8DirAtt0:  case kRow8DirAtt1:
    case kRow4DirAtt0:  case kRow4DirAtt1:
        cache->add (col, row, Bolt);
        return;
    case kRow0DirAtt:  case kRowSplash:  case kRowBoom:
        cache->add (col, row, BoltHit);
        return;
    case kRowCursor:  case kRowMissile:
        cache->add (col, row, Marker);
        return;
    }
}


void
shNotEyeInterface::terr2tile (shTerrainType terr, shCache *cache, int odd, int recolor)
{
    switch (terr) {
    case kStone:
        break;
    case kSewerWall1:
        cache->add (1, kRowSewers, Wall, recolor);
        break;
    case kSewerWall2:
        cache->add (2, kRowSewers, Wall, recolor);
        break;
    case kCavernWall1:
        cache->add (1, kRowGammaCaves, Wall, recolor);
        break;
    case kCavernWall2:
        cache->add (2, kRowGammaCaves, Wall, recolor);
        break;
    case kVWall: case kHWall: case kNTee: case kSTee: case kWTee: case kETee:
        cache->add (1, kRowSpaceBase, Wall, recolor);
        break;
    case kNWCorner: case kNECorner: case kSWCorner: case kSECorner:
        cache->add (2, kRowSpaceBase, spFlat + spWall + spIWallL + spIWallR, recolor);
        cache->add (1, kRowSpaceBase, spICeil, recolor);
        break;
    case kVirtualWall1:
        cache->add (1, kRowMainframe, Wall, recolor);
        break;
    case kVirtualWall2:
        cache->add (2, kRowMainframe, Wall, recolor);
        break;
    case kFloor:
    case kBrokenLightAbove:
        cache->add (5, kRowSpaceBase, Floor, recolor);
        break;
    case kCavernFloor:
        cache->add (5, kRowGammaCaves, Floor, recolor);
        break;
    case kSewerFloor:
        cache->add (5, kRowSewers, Floor, recolor);
        break;
    case kVirtualFloor:
        cache->add (5, kRowMainframe, Floor, recolor);
        break;
    case kSewage:
        cache->add (35 + odd, kRowSewers, Floor, recolor);
        break;
    case kGlassPanel:
        cache->add (5, kRowSpaceBase, Floor, recolor);
        cache->add (4, kRowSpaceBase, Wall, recolor);
        break;
    case kVoid:
    case kMaxTerrainType:
        break;
    }
}


void
shNotEyeInterface::feat2tile (shFeature *feat, shCache *cache, int recolor)
{
    int featrow = kRowASCII;
    switch (Level->mMapType) {
    case shMapLevel::kBunkerRooms:
    case shMapLevel::kTown:
    case shMapLevel::kRabbit:
        featrow = kRowSpaceBase; break;
    case shMapLevel::kSewer:
    case shMapLevel::kSewerPlant:
        featrow = kRowSewers; break;
    case shMapLevel::kRadiationCave:
        featrow = kRowGammaCaves; break;
    case shMapLevel::kMainframe:
        featrow = kRowMainframe; break;
    case shMapLevel::kTest:
        featrow = kRowASCII; break;
    }
    switch (feat->mType) {
    case shFeature::kDoorHiddenVert:
    case shFeature::kDoorHiddenHoriz:
        cache->add (1, featrow, Wall, recolor);
        break;
    case shFeature::kDoorClosed:
    case shFeature::kDoorOpen:
    {   /* Doors are very detailed. */
        int spDoor = spFlat + spCenter;
        spDoor += (feat->isHorizontalDoor ())
            ? spIWallL + spWallN + spWallS
            : spIWallR + spWallE + spWallW;
        /* Order is: closed, open, -, -, berserk closed, berserk open. */
        int x = 11; /* 11 is "plain" closed door. */
        if (feat->isOpenDoor ())  ++x;
        if (feat->isBerserkDoor () and !feat->mTrapUnknown)  x += 4;
        cache->add (x, featrow, spDoor, recolor);
        if (feat->isClosedDoor ()) { /* Show ceiling only if closed. */
            cache->add (1, featrow, spICeil, recolor);
            if (feat->isMagneticallySealed ()) {
                cache->add (14, featrow, spDoor, recolor); /* Indicate force field. */
            }
            if (feat->isInvertedDoor ()) { /* Troll face. */
                cache->add (17, featrow, spDoor, recolor);
            }
        }
        if (feat->isAutomaticDoor ()) { /* Pictures a detector above. */
            cache->add (13, featrow, spDoor, recolor);
        }
        if (feat->isClosedDoor () and feat->isLockDoor ()) {
            x = 19; /* No lock. */
            if (feat->isRetinaDoor ()) {
                x = 30;
            } else { /* Order is: open, closed, broken open, broken closed. */
                x = 26; /* 26 is "plain" open. */
                if (feat->isLockedDoor ())  ++x;
                if (feat->isLockBrokenDoor ())  x += 2;
            }
            cache->add (x, featrow, spDoor, recolor);
        }
        if (feat->isCodeLockDoor ()) { /* Color markings. */
            shObjectIlk *ilk = feat->keyNeededForDoor ();
            if (ilk)  switch (ilk->mAppearance.mGlyph.mColor) {
            case kNavy: x = 20; break;
            case kGreen: x = 21; break;
            case kRed: x = 22; break;
            case kOrange: x = 23; break;
            case kMagenta: x = 24; break;
            default: x = 25; break;
            } else {
                x = 25;
            }
            cache->add (x, featrow, spDoor, recolor);
        }
    }
    break;
    case shFeature::kStairsUp:
        cache->add (10, featrow, spFeature, recolor);
        break;
    case shFeature::kStairsDown:
        /* Most areas have way down in the floor. */
        if (featrow != kRowMainframe)
            cache->add (9, featrow, WayDown, recolor);
        else /* Sight obstructing way down. */
            cache->add (9, featrow, spFeature, recolor);
        break;
    case shFeature::kVat:
        cache->add (31, featrow, spFeature, recolor);
        break;
    case shFeature::kMovingHWall:
        cache->add (34, featrow, spFeature, recolor);
        break;
    case shFeature::kMachinery:
        cache->add (34, featrow, spFeature, recolor);
        break;
    case shFeature::kRadTrap:
        cache->add (2, kRowTrap, spFeature, recolor);
        break;
    case shFeature::kPit:
        cache->add (feat->mTrapMonUnknown == 2 ? 6 : 3, kRowTrap, spFeature, recolor);
        break;
    case shFeature::kAcidPit:
        cache->add (feat->mTrapMonUnknown == 2 ? 6 : 4, kRowTrap, spFeature, recolor);
        break;
    case shFeature::kHole:
        cache->add (5, kRowTrap, spFeature, recolor);
        break;
    case shFeature::kFloorGrating:
        cache->add (38, kRowSpaceBase, spFeature, recolor);
        break;
    case shFeature::kBrokenGrating:
        cache->add (39, kRowSpaceBase, spFeature, recolor);
        break;
    case shFeature::kSewagePit:
        cache->dellast (); /* Remove sludge. */
        cache->add (7, kRowTrap, spFeature, recolor);
        break;
    case shFeature::kACMESign:
    case shFeature::kTrapDoor:
    case shFeature::kWeb:
    case shFeature::kPortal:
    case shFeature::kPortableHole:
        cache->add (0, kRowTrap, spFeature, recolor);
        break;
    case shFeature::kComputerTerminal:
    case shFeature::kMaxFeatureType:
        break;
    }
}

const int spObj = spFlat + spItem + spIItem;
const int spCre = spFlat + spMonst + spIItem;

static void
putOverlay (shObject *obj, shNotEyeInterface::shCache *cache)
{
    using namespace obj;
    shGlyph *g = NULL;
    if (obj->has_subtype (computer)) {
        /* Show virus or antivirus status in lower right corner. */
        if (obj->is (fooproof | known_fooproof)) {
            cache->add (22, kRowTag1, spObj);
        } else if (obj->is (known_infected)) {
            cache->add (obj->is (infected) ? 23 : 37, kRowTag1, spObj);
        }
    } else if (obj->isA (kFloppyDisk)) {
        /* Add label to floppy disks. */
        if (obj->is (known_type)) {
            g = &obj->myIlk ()->mReal.mGlyph;
        } else if (obj->is (known_appearance)) {
            g = &obj->myIlk ()->mAppearance.mGlyph;
        }
        if (g) cache->add (g->mTileX, g->mTileY, spObj);
        /* Show virus status. */
        if (obj->is (known_infected)) {
            cache->add (obj->is (infected) ? 23 : 37, kRowTag1, spObj);
        }
    } else if (obj->myIlk ()->has_mini_icon () and obj->is (known_type)) {
        /* Indicate item type in upper right corner. */
        g = &obj->myIlk ()->mReal.mGlyph;
        cache->add (g->mTileX, g->mTileY, spObj);
    } else if (obj->has_subtype (jumpsuit)) {
        /* Put question mark on unidentified jumpsuit types. */
        g = &obj->myIlk ()->mReal.mGlyph;
        cache->add (obj->is (known_type) ? g->mTileX : 7, g->mTileY, spObj);
    }
    /*else if ((obj->isA (kArmor) or obj->isA (kWeapon)) and obj->is (fooproof))
    {
        cache->add (21, kRowCursor, spObj);
    }*/
}

/* Necklace of the Eye needs to be told both what tiles reside at given */
void         /* (x, y) coordinate pair and what ASCII glyph to display. */
shNotEyeInterface::draw (int x, int y, shCreature *c,
                   shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s)
{
    shGlyph g = getASCII (x, y, eff::none, c, oi, o, f, s);
    noteye::noteye_move (y, x);
    noteye::setTextAttr (g.mColor, g.mBkgd);
    noteye::noteye_addch (g.mSym);

    shMapLevel::shMemory mem = Level->getMemory (x, y);
    mCache[x][y].needs_update = true;
    mCache[x][y].tiledata->reset ();
    int recolor = Hero.cr ()->usesPower (kGammaSight) and
                  Level->isRadioactive (x, y);
    if (recolor)  recolor = 0x00FF00;
    if (s) {
        terr2tile (s->mTerr, &mCache[x][y], (x + y) % 2, recolor);
    }
    if (f) {
        feat2tile (f, &mCache[x][y], recolor);
    }
    if (o or oi) {
        shGlyph g = o ? o->getGlyph () : oi->mVague.mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spObj);
        if (o)  putOverlay (o, &mCache[x][y]);
    }
    if (c) {
        bool frozen = c->is (kFrozen);
        recolor = frozen ? 0x1CFFFF : 0;
        if (c->mTrapped.mWebbed) /* Web tile. */
            mCache[x][y].add (8, kRowBoom, spCre, recolor);
        /* Monster itself. */
        shGlyph g = c->mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spCre, recolor);
        if (frozen) /* Ice block tile. */
            mCache[x][y].add (7, kRowBoom, spCre);
        /* A small symbol tile in upper right corner. */
        if (c->isPet () and !c->isHero ()) /* Heart. */
            mCache[x][y].add (6, kRowTag2, spCre);
        else if (c->is (kAsleep)) /* Zzzzz. */
            mCache[x][y].add (1, kRowTag2, spCre);
    } else if (mem.mMon) {
        shGlyph g = MonIlks[mem.mMon].mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spCre);
    }

    /* Shield bubble. */
    if (s) {
        if (y < MAPMAXROWS-1) {
            shCreature *c = Level->getCreature (x, y+1);
            if (c and c->intr (kShielded) and c->countEnergy ())
                mCache[x][y].add (1, kRowShield, spCre);
        }
        if (x > 0) {
            shCreature *c = Level->getCreature (x-1, y);
            if (c and c->intr (kShielded) and c->countEnergy ())
                mCache[x][y].add (2, kRowShield, spCre);
        }
        if (y > 0) {
            shCreature *c = Level->getCreature (x, y-1);
            if (c and c->intr (kShielded) and c->countEnergy ())
                mCache[x][y].add (3, kRowShield, spCre);
        }
        if (x < MAPMAXCOLUMNS - 1) {
            shCreature *c = Level->getCreature (x+1, y);
            if (c and c->intr (kShielded) and c->countEnergy ())
                mCache[x][y].add (4, kRowShield, spCre);
        }

        shCreature *c = Level->getCreature (x, y);
        if (c and c->intr (kShielded) and c->countEnergy ())
            mCache[x][y].add (5, kRowShield, spCre);
    }
}

void
shNotEyeInterface::drawMem (int x, int y, shCreature *c,
                   shObjectIlk *o, shFeature *f, shSquare *s)
{
    shGlyph g = getMemASCII (x, y, eff::none, c, o, f, s);
    noteye::noteye_move (y, x);
    noteye::setTextAttr (g.mColor, g.mBkgd);
    noteye::noteye_addch (g.mSym);

    shMapLevel::shMemory mem = Level->getMemory (x, y);
    mCache[x][y].needs_update = true;
    mCache[x][y].tiledata->reset ();

    if (s) {
        terr2tile (s->mTerr, &mCache[x][y], (x + y) % 2, 0);
    } else if (mem.mTerr) {
        terr2tile (mem.mTerr, &mCache[x][y], (x + y) % 2, 0);
    }
    if (f) {
        feat2tile (f, &mCache[x][y], 0);
    } else if (mem.mFeat != shFeature::kMaxFeatureType) {
        shFeature *real = Level->getFeature (x, y);
        shFeature fake;
        fake.mType = mem.mFeat;
        fake.mDoor.mFlags = mem.mDoor;
        if (real)
            fake.mTrapUnknown = real->mTrapUnknown;
        feat2tile (&fake, &mCache[x][y], 0);
    }
    if (o or mem.mObj) {
        shGlyph g = o ? o->mVague.mGlyph : AllIlks[mem.mObj].mVague.mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spObj);
    }
    if (c or mem.mMon) {
        shGlyph g = c ? c->mGlyph : MonIlks[mem.mMon].mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spCre);
    }
}

void
shNotEyeInterface::drawEffect (int x, int y, eff::type e)
{
    if (e == eff::none)
        return;

    drawSpecialEffect (e, &mCache[x][y]);
}
