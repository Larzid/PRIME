#include <ctype.h>
#include "NCUI.h"
#include "Hero.h"

int ColorMap[22];

shInterface *
startNCurses (void)
{
    return new shNCursesInterface;
}

shNCursesInterface::shNCursesInterface ()
{
    WINDOW *win = initscr ();

    if (!win) {
        fprintf (stderr, "Sorry, curses support is required.\n");
        exitPRIME (-1);
    }

    if (!has_colors ()) {
        endwin ();
        fprintf (stderr, "Sorry, color support is required.\n");
        exitPRIME (-1);
    }
    start_color ();

    for (int i = 1; i < COLOR_PAIRS; ++i) {
        init_pair (i, i %8, i/8);
    }


#ifdef _WIN32
    curs_set (1);
#endif
    ColorMap[kBlack] = COLOR_BLACK;
    ColorMap[kBlue] = COLOR_PAIR (COLOR_BLUE);
    ColorMap[kGreen] = COLOR_PAIR (COLOR_GREEN);
    ColorMap[kCyan] = COLOR_PAIR (COLOR_CYAN);
    ColorMap[kRed] = COLOR_PAIR (COLOR_RED);
    ColorMap[kMagenta] = COLOR_PAIR (COLOR_MAGENTA);
    ColorMap[kBrown] = COLOR_PAIR (COLOR_YELLOW);
    ColorMap[kGray] = COLOR_PAIR (COLOR_WHITE);
    ColorMap[kDarkGray] = COLOR_PAIR (COLOR_BLACK) | A_BOLD;
    ColorMap[kNavy] = COLOR_PAIR (COLOR_BLUE)| A_BOLD;
    ColorMap[kLime] = COLOR_PAIR (COLOR_GREEN) | A_BOLD;
    ColorMap[kAqua] = COLOR_PAIR (COLOR_CYAN) | A_BOLD;
    ColorMap[kOrange] = COLOR_PAIR (COLOR_RED) | A_BOLD;
    ColorMap[kPink] = COLOR_PAIR (COLOR_MAGENTA) | A_BOLD;
    ColorMap[kYellow] = COLOR_PAIR (COLOR_YELLOW) | A_BOLD;
    ColorMap[kWhite] = COLOR_PAIR (COLOR_WHITE) | A_BOLD;


    mColor = kGray;

//    cbreak ();
    raw ();
    noecho ();
#ifdef DJGPP
    nl ();
#else
    nonl ();
#endif

    mXMax = 64;
    mYMax = 20;

    mWin[kMain] = newwin (20, mXMax, 0, 0);
    if (!mWin[kMain]) goto toosmall;
    mPan[kMain] = new_panel (mWin[kMain]);
    notimeout (mWin[kMain], TRUE);
    mWin[kSide] = newwin (20, 80 - mXMax, 0, mXMax);
    if (!mWin[kSide]) goto toosmall;
    mPan[kSide] = new_panel (mWin[kSide]);
    mWin[kLog] = newwin (5, 80, 20, 0);
    if (!mWin[kLog]) goto toosmall;
    mPan[kLog] = new_panel (mWin[kLog]);
    mWin[kDiag] = NULL;
    if (BOFH and LINES >= 30) {
        mWin[kDiag] = newwin (0, 80, 25, 0);
        scrollok (mWin[kDiag], TRUE);
    }
    mWin[kTemp] = newwin (0, 0, 0, 0);
    notimeout (mWin[kLog], TRUE);
    mLogSize = 5;
    mLogRow = 0;
    mHistoryIdx = 0;
    mHistoryWrapped = 0;
    mNoNewline = 0;
    mLogSCount = 0;
    mPause = 0;

    mX0 = 0;
    mY0 = 0;

    intrflush (mWin[kMain], FALSE);
#ifndef NO_CURSES_KEYPAD
    keypad (mWin[kMain], TRUE);
#endif
    intrflush (mWin[kLog], FALSE);
    scrollok (mWin[kLog], TRUE);
#ifndef WIN32
    set_escdelay (50);
#endif

    ascii_cache = (shGlyph *) calloc (MAPMAXCOLUMNS * MAPMAXROWS, sizeof (shGlyph));

    debug.log ("COLORS: %d", COLORS);
    debug.log ("COLOR_PAIRS: %d", COLOR_PAIRS);
    debug.log ("can change: %d", can_change_color());

    return;

toosmall:
    endwin ();
    fprintf (stderr, "Sorry, but a terminal with dimensions of at least 80x25"
             " is required.\n");
    exitPRIME (-1);
}


shNCursesInterface::~shNCursesInterface ()
{
    free (ascii_cache);
    del_panel (mPan[kSide]);
    del_panel (mPan[kMain]);
    delwin (mWin[kSide]);
    delwin (mWin[kMain]);
    endwin ();
}


void
shNCursesInterface::runMainLoop ()
{
    mainLoop ();
}

void
shNCursesInterface::cursorOnXY (int x, int y, int)
{
    wmove (mWin[kMain], y, x);
    wrefresh (mWin[kMain]); /* Necessary for cursor to be actually moved. */
}


void
shNCursesInterface::winGoToYX (Window win, int y, int x)
{
    wmove (mWin[win], y, x);
}


void
shNCursesInterface::winGetYX (Window win, int *y, int *x)
{
    getyx (mWin[win], *y, *x);
}


void
shNCursesInterface::newWin (Window win)
{
    mWin[win] = newwin (0, 0, 0, 0);
    if (win == kMenu)
        keypad (mWin[win], TRUE);
    mPan[win] = new_panel (mWin[win]);
    update_panels ();
}


void
shNCursesInterface::delWin (Window win)
{
    hide_panel (mPan[win]);
    del_panel (mPan[win]);
    delwin (mWin[win]);
    mWin[win] = NULL;
    update_panels ();
}


void
shNCursesInterface::moveWin (Window win, int x1, int y1, int x2, int y2)
{
    delWin (win);
    mWin[win] = newwin (y2 - y1 + 1, x2 - x1 + 1, y1, x1);
    mPan[win] = new_panel (mWin[win]);
    update_panels ();
}


void
shNCursesInterface::clearWin (Window win)
{
    werase (mWin[win]);
}


void
shNCursesInterface::refreshWin (Window win)
{
    if (win == kTemp or win == kMenu or win == kMenuHelp) {
        update_panels ();
        doupdate ();
    } else {
        wrefresh (mWin[win]);
    }
}


int
shNCursesInterface::getMaxLines ()
{
    return LINES;
}


int
shNCursesInterface::getMaxColumns ()
{
    return COLS;
}


void
shNCursesInterface::assign (int key, Command cmd)
{
    mKey2Cmd[key] = cmd;
}


void
shNCursesInterface::resetKeybindings ()
{
    for (int i = 0; i < KEY_MAX; ++i) {
        mKey2Cmd[i] = kNoCommand;
    }
}


static const struct shNameToKey KeyNames[] =
{
    {"break", KEY_BREAK},
    {"space", ' '},
    {"backspace", KEY_BACKSPACE},
    {"tab", '\t'},
    {"enter", KEY_ENTER},
    {"escape", 27},
    {"down arrow", KEY_DOWN},
    {"up arrow", KEY_UP},
    {"left arrow", KEY_LEFT},
    {"right arrow", KEY_RIGHT},
    {"keypad 1", KEY_C1},
    {"keypad 2", KEY_DOWN},
    {"keypad 3", KEY_C3},
    {"keypad 4", KEY_LEFT},
    {"keypad 5", KEY_B2},
    {"keypad 6", KEY_RIGHT},
    {"keypad 7", KEY_A1},
    {"keypad 8", KEY_UP},
    {"keypad 9", KEY_A3},
    {"home", KEY_HOME},
    {"end", KEY_END},
    {"insert", KEY_IC},
    {"delete", KEY_DC},
    {"page up", KEY_PPAGE},
    {"page down", KEY_NPAGE},
    {"shift home", KEY_SHOME},
    {"shift end", KEY_SEND},
    {"shift insert", KEY_SIC},
    {"shift delete", KEY_SDC},
    {"fn 1", KEY_F(1)},
    {"fn 2", KEY_F(2)},
    {"fn 3", KEY_F(3)},
    {"fn 4", KEY_F(4)},
    {"fn 5", KEY_F(5)},
    {"fn 6", KEY_F(6)},
    {"fn 7", KEY_F(7)},
    {"fn 8", KEY_F(8)},
    {"fn 9", KEY_F(9)},
    {"fn 10", KEY_F(10)},
    {"fn 11", KEY_F(11)},
    {"fn 12", KEY_F(12)}
};
static const int n_keys = sizeof (KeyNames) / sizeof (shNameToKey);

void
shNCursesInterface::readKeybindings (const char *fname)
{
    I->readKeybindings (fname, n_keys, KeyNames);
}


static const char *
keyCodeToString (int code)
{
    if (code >= KEY_F(1) and code <= KEY_F(12)) {
        char *buf = GetBuf ();
        sprintf (buf, "F%d", code - KEY_F(1) + 1);
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
shNCursesInterface::getKeyForCommand (Command cmd)
{   /* Find key corresponding to command. */
    int i;
    for (i = 0; i < KEY_MAX and cmd != mKey2Cmd[i]; ++i)
        ;
    if (i == KEY_MAX) /* Unassigned?  This is bad. */
        return NULL;  /* Safe, will not lead to crash. */
    return keyCodeToString (i);
}


shVector<const char *> *
shNCursesInterface::getKeysForCommand (Command cmd)
{
    shVector<const char *> *keys = new shVector<const char *> ();
    for (int i = 0; i < KEY_MAX; ++i) {
        if (cmd == mKey2Cmd[i])  keys->add (keyCodeToString (i));
    }
    return keys;
}


shInterface::Command
shNCursesInterface::keyToCommand (int key)
{
    return mKey2Cmd[key];
}

bool
shNCursesInterface::has_vi_keys ()
{
    return mKey2Cmd[int('l')] == kMoveE;
}

bool
shNCursesInterface::has_vi_fire_keys ()
{
    return mKey2Cmd[0x1f & 'l'] == kFireE;
}


int
shNCursesInterface::getChar ()
{
    return wgetch (mWin[kMain]);
}


int
shNCursesInterface::getSpecialChar (SpecialKey *sk)
{
    int key = getChar ();
    switch (key) {
    case 27: *sk = kEscape; break;
    case ' ': *sk = kSpace; break;
    case KEY_ENTER: case 10: case 13: *sk = kEnter; break;
    case KEY_BACKSPACE: *sk = kBackSpace; break;
    case KEY_IC: *sk = kInsert; break;
    case KEY_DC: *sk = kDelete; break;
    case KEY_HOME:  case KEY_A1: *sk = kHome; break;
    case KEY_END:   case KEY_C1: *sk = kEnd; break;
    case KEY_PPAGE: case KEY_A3: *sk = kPgUp; break;
    case KEY_NPAGE: case KEY_C3: *sk = kPgDn; break;
    case KEY_UP: *sk = kUpArrow; break;
    case KEY_DOWN: *sk = kDownArrow; break;
    case KEY_LEFT: *sk = kLeftArrow; break;
    case KEY_RIGHT: *sk = kRightArrow; break;
    case KEY_B2: *sk = kCenter; break;
    default: *sk = kNoSpecialKey; break;
    }
    return key;
}

/* MODIFIES: reads a string from the keyboard
   RETURNS: number of characters read, 0 on error */

int
shNCursesInterface::getStr (char *buf, int len, const char *prompt,
                    const char *dflt /* = NULL */ ) /* Default suggestion. */
{   /* can't use wgetnstr b/c it won't pass us ^C */
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
        waddstr (mWin[kLog], buf);
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
            waddch (mWin[kLog], c);
        } else if ('\n' == c or '\r' == c) {
            break;
        } else if ((c == KEY_BACKSPACE or c == 8) and pos) {
            pos--;
            /* Warning: Do not compress those three addch calls into one
               addstr ("\x8 \x8") call.  NCurses on Unix handles this well
               but PDCurses on Windows just chokes. */
            waddch (mWin[kLog], '\x8');
            waddch (mWin[kLog], ' ');
            waddch (mWin[kLog], '\x8');
        } else if (c == 27) { /* Escape. */
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
shNCursesInterface::diag (const char *format, ...)
{
    int res;
    va_list ap;

    mColor = kMagenta;
    va_start (ap, format);
    char dbgbuf[100];
    res = vsnprintf (dbgbuf, 100, format, ap);
    if (mWin[kDiag]) {
        wattrset (mWin[kDiag], ColorMap[kCyan]);
        waddstr (mWin[kDiag], dbgbuf);
        waddch (mWin[kDiag], '\n');
        touchwin (mWin[kDiag]);
        wnoutrefresh (mWin[kDiag]);
        doupdate ();
    }
    debug.log ("%s", dbgbuf);
    va_end (ap);
    mColor = kGray;
    return res;
}


void
shNCursesInterface::pageLog ()
{
    if (Flags.mFadeLog) {
        int x, y;
        getyx (mWin[kLog], y, x);

        wattrset (mWin[kLog], ColorMap[kBlue]);

        for (int i = 0; i < mLogRow; i++) {
            int offs = (HISTORY_ROWS + mHistoryIdx - (mLogRow-i)) % HISTORY_ROWS;
            mvwaddstr (mWin[kLog], i, 0, &mLogHistory[offs*80]);
        }

        wattrset (mWin[kLog], A_NORMAL);
        wmove (mWin[kLog], y, x);
        mLogSCount = 0;
    } else {
        werase (mWin[kLog]);
        wmove (mWin[kLog], 0, 0);
        mLogRow = 0;
        mLogSCount = 0;
    }
    drawLog ();
}


void
shNCursesInterface::doScreenShot (FILE *file)
{
    struct shWindata
    {
        int y1, x1, y2, x2;
        WINDOW *winptr;
    };
    shWindata winyx[2] =
    {
        { 0,     0,     mYMax, mXMax, mWin[kMain] },
        { 0,     mXMax, mYMax, 80,    mWin[kSide] }
    };
    char screen[20][81];
    /* Fill screenshot buffer. */
    for (int win = 0; win < 2; ++win) {
        for (int y = winyx[win].y1; y < winyx[win].y2; ++y) {
            for (int x = winyx[win].x1; x < winyx[win].x2; ++x) {
                chtype square = mvwinch (winyx[win].winptr,
                    y - winyx[win].y1, x - winyx[win].x1);
                char c = square & A_CHARTEXT;    /* Strip color. */
                screen[y][x] = c;
            }
        }
    } /* Dump everything to file. */
    for (int y = 0; y < 20; ++y) {
        screen[y][80] = 0;
        fprintf (file, "%s\n", screen[y]);
    }
    /* Copy last entries of log file. */
    for (int y = mLogSCount - 1; y >= 0; --y) {
        int idx = (mHistoryIdx - y - 1 + HISTORY_ROWS) % HISTORY_ROWS;
        if (idx < 0) {
            idx += HISTORY_ROWS;
        }
        fprintf (file, "%s\n", &mLogHistory[idx * 80]);
    }
}


void
shNCursesInterface::showVersion ()
{
    const char **splash_logo = getLogoPtr ();
    const int len = strlen(splash_logo[0]);
    WINDOW *win;
    PANEL *panel;
    win = newwin (18, 60, 3, 10);
    if (!win) {
        return;
    }
    panel = new_panel (win);

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < len; ++x) {
            if (splash_logo[y][x] == '#') {
                wattrset (win, ColorMap[kLime]);
            } else {
                wattrset (win, ColorMap[kGreen]);
            }
            mvwaddnstr (win, y, x + 15, &splash_logo[y][x], 1);
        }
    }

    wattrset (win, ColorMap[kBrown]);
    mvwaddstr (win, 7, 20, PRIME_VERSION"/Astral Navigator");
    mvwaddstr (win, 9, 7, "Pesky Reticulans, Improvements, More Everything");
    /* The everything part includes bugs.  Tell no one! -- MB */
    wattrset (win, ColorMap[kYellow]);
    mvwaddstr (win, 9, 7,  "P");
    mvwaddstr (win, 9, 13, "R");
    mvwaddstr (win, 9, 25, "I");
    mvwaddstr (win, 9, 39, "M");
    mvwaddstr (win, 9, 44, "E");
    wattrset (win, ColorMap[kOrange]);
    mvwaddstr (win, 11, 14, "Unofficial variant of ZAPM v" ZAPM_VERSION " ");
    mvwaddstr (win, 12, 14, "by Psiweapon and Michal Bielinski");
    wattrset (win, ColorMap[kRed]);
    mvwaddstr (win, 13, 17, "http://pesky-reticulans.org");
    wattrset (win, A_NORMAL);
    mvwaddstr (win, 15, 15, "ZAPM (C) 2002-2010 Cyrus Dolph.");
    mvwaddstr (win, 16, 20, "All rights reserved.");
    mvwaddstr (win, 17, 20, "http://www.zapm.org");
    wattrset (win, ColorMap[kLime]);
    wattrset (win, A_NORMAL);

    update_panels ();
    doupdate ();
    getChar ();
    hide_panel (panel);
    del_panel (panel);
    delwin (win);
    update_panels ();
}


void
shNCursesInterface::winPutchar (Window win, const char c)
{
    waddch (mWin[win], c);
}


void
shNCursesInterface::winPrint (Window win, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    waddstr (mWin[win], linebuf);
}


void
shNCursesInterface::winPrint (Window win, int y, int x, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    mvwaddstr (mWin[win], y, x, linebuf);
}


void
shNCursesInterface::setWinColor (Window win, shColor fg, shColor bg)
{
    if (bg != kBlack)
        wattrset (mWin[win], ColorMap[bg] | A_REVERSE);
    else
        wattrset (mWin[win], ColorMap[fg]);
}


void
shNCursesInterface::drawLog ()
{
    if (mWin[kDiag]) {
        touchwin (mWin[kDiag]);
        wnoutrefresh (mWin[kDiag]);
    }
    touchwin (mWin[kLog]);
    wnoutrefresh (mWin[kLog]);
    doupdate ();
}


void
shNCursesInterface::drawScreen ()
{
    Level->draw ();
    drawSideWin (Hero.cr ());
    wnoutrefresh (mWin[kMain]);
    drawLog (); /* calls doupdate() for us */
}


void
shNCursesInterface::refreshScreen ()
{
    wnoutrefresh (mWin[kMain]);
    drawLog ();
}

/* In ASCII mode only the most important thing on a tile is displayed. */
void
shNCursesInterface::draw (int x, int y, shCreature *c,
                   shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s)
{
    eff::type e = Level->mEffects[x][y];
    shGlyph g = getASCII (x, y, e, c, oi, o, f, s);
    ascii_cache[x + y * MAPMAXCOLUMNS] = g;
    wmove (mWin[kMain], y - mY0, x - mX0);
    if (!g.mBkgd)
        waddch (mWin[kMain], g.mSym | ColorMap[g.mColor]);
    else
        waddch (mWin[kMain], g.mSym | ColorMap[g.mBkgd] | A_REVERSE);
}

void
shNCursesInterface::drawMem (int x, int y, shCreature *c,
                      shObjectIlk *oi, shFeature *f, shSquare *s)
{
    eff::type e = Level->mEffects[x][y];
    shGlyph g = getMemASCII (x, y, e, c, oi, f, s);
    ascii_cache[x + y * MAPMAXCOLUMNS] = g;
    wmove (mWin[kMain], y - mY0, x - mX0);
    if (!g.mBkgd)
        waddch (mWin[kMain], g.mSym | ColorMap[g.mColor]);
    else
        waddch (mWin[kMain], g.mSym | ColorMap[g.mBkgd] | A_REVERSE);
}

void
shNCursesInterface::drawEffect (int x, int y, eff::type e)
{
    if (e == eff::none)
        return;

    shGlyph g = getASCII (x, y, e, NULL, NULL, NULL, NULL, NULL);
    if (g.mSym == 0)
        return;
    /* Effect changes only existing colors. */
    if (g.mSym == ' ') {
        shGlyph *c = &ascii_cache[x + y * MAPMAXCOLUMNS];
        g.mSym = c->mSym;
        if (!g.mColor)
            g.mColor = c->mColor;
    }

    wmove (mWin[kMain], y - mY0, x - mX0);
    if (!g.mBkgd)
        waddch (mWin[kMain], g.mSym | ColorMap[g.mColor]);
    else
        waddch (mWin[kMain], g.mSym | ColorMap[g.mBkgd] | A_REVERSE);
}
