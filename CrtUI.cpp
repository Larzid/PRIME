#include <ctype.h>
#include "CrtUI.h"
#include "Hero.h"

extern "C"
{
    extern void pio_init (void);
    extern void pio_exit (void);
    extern void pio_clear (void);
    extern void pio_refresh (void);
    extern void pio_gotoxy (int, int);
    extern void pio_echo (int, int, unsigned char);
    extern void pio_draw (int, int, unsigned char, int, int);
    extern int pio_screen (int, int);
    extern void pio_string (int, int, const char *);
    extern void pio_textattr (unsigned int);
    extern int pio_get_height (void);
    extern int pio_get_width (void);
    extern unsigned char pio_read_key (void);
}

shInterface *
startCrt (void)
{
    return new shCrtInterface;
}

shCrtInterface::shCrtInterface ()
{
    pio_init ();
    mXMax = 64;
    mYMax = 20;

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

    mX0 = 0;
    mY0 = 0;
}


shCrtInterface::~shCrtInterface ()
{
    pio_exit ();
}


void
shCrtInterface::runMainLoop ()
{
    mainLoop ();
}

void
shCrtInterface::cursorOnXY (int x, int y, int curstype)
{   /* Curstype is ignored for now. */
    pio_gotoxy (x, y);
}


void
shCrtInterface::winGoToYX (Window win, int y, int x)
{
    pio_gotoxy (mWin[win].cx = mWin[win].x1 + x, mWin[win].cy = mWin[win].y1 + y);
}


void
shCrtInterface::winGetYX (Window win, int *y, int *x)
{
    *y = mWin[win].cy - mWin[win].y1;  *x = mWin[win].cx - mWin[win].x1;
}


void
shCrtInterface::newWin (Window win)
{
    mWin[win].x1 = 0;
    mWin[win].y1 = 0;
    mWin[win].x2 = 80;
    mWin[win].y2 = 25;
    if (win == kTemp)
        clearWin (win);
}


void
shCrtInterface::delWin (Window win)
{
    clearWin (win);
    drawScreen ();
}


void
shCrtInterface::moveWin (Window win, int x1, int y1, int x2, int y2)
{
    mWin[win].x1 = x1;
    mWin[win].x2 = x2;
    mWin[win].y1 = y1;
    mWin[win].y2 = y2;
}


void
shCrtInterface::clearWin (Window win)
{
    for (int y = mWin[win].y1; y < mWin[win].y2; ++y)
        for (int x = mWin[win].x1; x < mWin[win].x2; ++x)
            pio_echo (x, y, ' ');
}


void
shCrtInterface::refreshWin (Window win)
{
    pio_refresh ();
}


int
shCrtInterface::getMaxLines ()
{
    return pio_get_height ();
}


int
shCrtInterface::getMaxColumns ()
{
    return pio_get_width ();
}


void
shCrtInterface::assign (int key, Command cmd)
{
    mKey2Cmd[key] = cmd;
}


void
shCrtInterface::resetKeybindings ()
{
    for (int i = 0; i < 512; ++i)
        mKey2Cmd[i] = kNoCommand;
}

const int KEY_F0 = 256 + 58;
static const struct shNameToKey KeyNames[] =
{
    {"space", ' '},
    {"tab", '\t'},
    {"backspace", 8},
    {"enter", 13},
    {"escape", 27},
    {"down arrow", 256 + 'P'},
    {"up arrow", 256 + 'H'},
    {"left arrow", 256 + 'K'},
    {"right arrow", 256 + 'M'},
    {"keypad 1", 256 + 'O'},
    {"keypad 2", 256 + 'P'},
    {"keypad 3", 256 + 'Q'},
    {"keypad 4", 256 + 'K'},
    {"keypad 5", '5'},
    {"keypad 6", 256 + 'M'},
    {"keypad 7", 256 + 'G'},
    {"keypad 8", 256 + 'H'},
    {"keypad 9", 256 + 'I'},
    {"home", 256 + 'G'},
    {"end", 256 + 'O'},
    {"page up", 256 + 'I'},
    {"page down", 256 + 'Q'},
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
    {"fn 11", 256 + 133},
    {"fn 12", 256 + 134}
};
static const int n_keys = sizeof (KeyNames) / sizeof (shNameToKey);

void
shCrtInterface::readKeybindings (const char *fname)
{
    I->readKeybindings (fname, n_keys, KeyNames);
}


static const char *
keyCodeToString (int code)
{
    if (code >= KEY_F0 + 1 and code <= KEY_F0 + 10) {
        char *buf = GetBuf ();
        sprintf (buf, "F%d", code - KEY_F0);
        return buf;
    }
    if (code == 256 + 133 or code == 256 + 134) {
        char *buf = GetBuf ();
        sprintf (buf, "F%d", code - 256 - 133 + 11);
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
shCrtInterface::getKeyForCommand (Command cmd)
{   /* Find key corresponding to command. */
    int i;
    for (i = 0; i < 512 and cmd != mKey2Cmd[i]; ++i)
        ;
    if (i == 512) /* Unassigned?  This is bad. */
        return NULL;  /* Safe, will not lead to crash. */
    return keyCodeToString (i);
}


shVector<const char *> *
shCrtInterface::getKeysForCommand (Command cmd)
{
    shVector<const char *> *keys = new shVector<const char *> ();
    for (int i = 0; i < 512; ++i) {
        if (cmd == mKey2Cmd[i])  keys->add (keyCodeToString (i));
    }
    return keys;
}


shInterface::Command
shCrtInterface::keyToCommand (int key)
{
    return mKey2Cmd[key];
}

bool
shCrtInterface::has_vi_keys ()
{
    return mKey2Cmd[int('l')] == kMoveE;
}

bool
shCrtInterface::has_vi_fire_keys ()
{
    return mKey2Cmd[0x1f & 'l'] == kFireE;
}


int
shCrtInterface::getChar ()
{
    unsigned char key = pio_read_key ();
    if (!key)  return 256 + pio_read_key ();
    return key;
}


int
shCrtInterface::getSpecialChar (SpecialKey *sk)
{
    int key = pio_read_key ();
    if (!key)  switch (key = pio_read_key () + 256, key - 256) {
        case 'G': *sk = kHome; break;
        case 'O':  *sk = kEnd; break;
        case 'I': *sk = kPgUp; break;
        case 'Q': *sk = kPgDn; break;
        case 'H': *sk = kUpArrow; break;
        case 'P': *sk = kDownArrow; break;
        case 'K': *sk = kLeftArrow; break;
        case 'M': *sk = kRightArrow; break;
        default: *sk = kNoSpecialKey; break;
    } else switch (key) {
        case 8: *sk = kBackSpace; break;
        case 27: *sk = kEscape; break;
        case 13: *sk = kEnter; break;
        case ' ': *sk = kSpace; break;
        case '5': *sk = kCenter; break;
        default: *sk = kNoSpecialKey; break;
    }
    return key;
}

/* MODIFIES: reads a string from the keyboard
   RETURNS: number of characters read, 0 on error */

int
shCrtInterface::getStr (char *buf, int len, const char *prompt,
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
        refreshScreen ();
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
shCrtInterface::diag (const char *format, ...)
{
    int res;
    va_list ap;

    mColor = kMagenta;
    va_start (ap, format);
    char dbgbuf[100];
    res = vsnprintf (dbgbuf, 100, format, ap);
    /* No auto-scrolling ... must implement own.
    if (BOFH) {
        setWinColor (kDiag, kCyan);
        winPrint (dbgbuf);
        winPutchar ('\n');
    }
    */
    debug.log ("%s", dbgbuf);
    va_end (ap);
    mColor = kGray;
    return res;
}


void
shCrtInterface::pageLog ()
{
    mLogSCount = 0;
    mLogRow = 0;
    clearWin (kLog);
    winGoToYX (kLog, 0, 0);
    drawLog ();
}


void
shCrtInterface::doScreenShot (FILE *file)
{
    char screen[25][81];
    /* Fill screenshot buffer. */
    for (int y = 0; y < mWin[kLog].y2; ++y)
        for (int x = 0; x < mWin[kSide].x2; ++x)
            screen[y][x] = pio_screen (x, y) & 0xFF;
    /* Dump everything to file. */
    for (int y = 0; y < 25; ++y) {
        screen[y][80] = 0;
        fprintf (file, "%s\n", screen[y]);
    }
}


void
shCrtInterface::showVersion ()
{
    const char **splash_logo = getLogoPtr ();
    const int len = strlen(splash_logo[0]);

    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < len; ++x) {
            shColor c = (splash_logo[y][x] == '#') ? kLime : kGreen;
            pio_draw (x + 25, y + 3, splash_logo[y][x], c, 0);
        }

    pio_textattr (kBrown);
    pio_string (10 + 20, 7 + 3, PRIME_VERSION"/  Metapsychics  ");
    pio_string (10 + 7, 9 + 3, "Pesky Reticulans, Improvements, More Everything");
    pio_draw (10 + 7, 9 + 3, 'P', kYellow, kBlack);
    pio_draw (10 + 13, 9 + 3, 'R', kYellow, kBlack);
    pio_draw (10 + 25, 9 + 3, 'I', kYellow, kBlack);
    pio_draw (10 + 39, 9 + 3, 'M', kYellow, kBlack);
    pio_draw (10 + 44, 9 + 3, 'E', kYellow, kBlack);
    pio_textattr (kOrange);
    pio_string (10 + 14, 11 + 3, "Unofficial variant of ZAPM v" ZAPM_VERSION " ");
    pio_string (10 + 14, 12 + 3, "by Psiweapon and Michal Bielinski");
    pio_textattr (kRed);
    pio_string (10 + 17, 13 + 3, "http://pesky-reticulans.org");
    pio_textattr (kGray);
    pio_string (10 + 15, 15 + 3, "ZAPM (C) 2002-2010 Cyrus Dolph.");
    pio_string (10 + 20, 16 + 3, "All rights reserved.");
    pio_string (10 + 20, 17 + 3, "http://www.zapm.org");

    pio_refresh ();
    getChar ();
    pio_clear ();
}


void
shCrtInterface::winPutchar (Window win, const char c)
{
    if (c == '\n')
        pio_gotoxy (mWin[win].cx = 0, ++mWin[win].cy);
    else
        pio_echo (mWin[win].cx++, mWin[win].cy, c);
}


void
shCrtInterface::winPrint (Window win, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    pio_string (mWin[win].cx, mWin[win].cy, linebuf);
    mWin[win].cx += strlen (linebuf);
    mWin[win].cx = mini (mWin[win].cx, mWin[win].x2);
}


void
shCrtInterface::winPrint (Window win, int y, int x, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    pio_string (mWin[win].x1 + x, mWin[win].y1 + y, linebuf);
    mWin[win].cy = mWin[win].y1 + y;
    mWin[win].cx = mWin[win].x1 + x + strlen (linebuf);
    mWin[win].cx = mini (mWin[win].cx, mWin[win].x2);
}


void
shCrtInterface::setWinColor (Window win, shColor fg, shColor bg)
{
    pio_textattr (fg + (bg << 4));
}


void
shCrtInterface::drawLog ()
{
    pio_refresh ();
}


void
shCrtInterface::drawScreen ()
{
    Level->draw ();
    drawSideWin ();
    pio_refresh ();
}


void
shCrtInterface::refreshScreen ()
{
    pio_refresh ();
}

/* In ASCII mode only the most important thing on a tile is displayed. */
void
shCrtInterface::draw (int x, int y, shSpecialEffect e, shCreature *c,
                   shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s)
{
    shGlyph g = getASCII (x, y, e, c, oi, o, f, s);
    pio_draw (x, y, g.mSym, g.mColor, g.mBkgd);
}

void
shCrtInterface::drawMem (int x, int y, shSpecialEffect e, shCreature *c,
                      shObjectIlk *oi, shFeature *f, shSquare *s)
{
    shGlyph g = getMemASCII (x, y, e, c, oi, f, s);
    pio_draw (x, y, g.mSym, g.mColor, g.mBkgd);
}
