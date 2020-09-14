#ifndef NEUI_H
#define NEUI_H
#include "Interface.h"
#include "noteye-curses.h"

#define MAXKEYS DBASE+32

class shNotEyeInterface : public shInterface
{
 public:
    shNotEyeInterface (int argc, char **argv);
    ~shNotEyeInterface ();

    int getChar ();
    int getSpecialChar (SpecialKey *sk);
    int getStr (char *buf, int len, const char *prompt,
                const char *dflt = NULL);
    const char *getKeyForCommand (Command cmd);
    shVector<const char *> *getKeysForCommand (Command cmd);

    void cursorOnXY (int x, int y, int curstype);
    void draw (int x, int y, shCreature *c,
               shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s);
    void drawMem (int x, int y, shCreature *c,
                  shObjectIlk *o, shFeature *f, shSquare *s);
    void drawEffect (int x, int y, eff::type e);

    int diag (const char *format, ...);
    void drawScreen ();
    void refreshScreen ();
    void drawLog ();
    void pageLog ();
    void showVersion ();
    void doScreenShot (FILE *file);

    int getMaxLines ();
    int getMaxColumns ();
    void newWin (Window win);
    void delWin (Window win);
    void moveWin (Window win, int x1, int y1, int x2, int y2);
    void clearWin (Window win);
    void refreshWin (Window win);
    void setWinColor (Window win, shColor fg, shColor bg);
    void winGoToYX (Window win, int y, int x);
    void winGetYX (Window win, int *y, int *x);
    void winPutchar (Window win, const char c);
    void winPrint (Window win, const char *fmt, ...);
    void winPrint (Window win, int y, int x, const char *fmt, ...);

    shMenu *newMenu (const char *prompt, int flags);
    void runMainLoop ();

    struct shCache {
        bool needs_update;
        shVector<int> *tiledata;
        void add (int x, int y, int spatial, int recolor = 0);
        void dellast ();
    } mCache[MAPMAXCOLUMNS][MAPMAXROWS];

 private:
    Command mKey2Cmd[MAXKEYS];
    struct {
        int x1, y1, x2, y2; /* Window absolute position on screen. */
        int cx, cy;         /* Cursor coordinates. */
    } mWin[kMaxWin];

    Command keyToCommand (int key);
    void readKeybindings (const char *fname);
    void assign (int key, Command cmd);
    void resetKeybindings ();

    bool has_vi_keys ();
    bool has_vi_fire_keys ();

    void terr2tile (shTerrainType terr, shCache *cache, int odd, int recolor);
    void feat2tile (shFeature *feat, shCache *cache, int recolor);
};

#endif
