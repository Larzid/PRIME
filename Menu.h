#ifndef MENU_H
#define MENU_H

struct shMenuChoice
{
    char mLetter, mAltLetter;
    int mCount;           /* count available. -1 indicates this is a header */
    int mSelected;        /* count currently selected */
    char mText[256];
    union {
        const void *mPtr; /* value of the object returned later if this */
        int mInt;         /* choice is selected. */
    } mValue;

    shMenuChoice (char letter, char alt, const char *text, const void *value,
        int count, int selected = 0);
    shMenuChoice (char letter, char alt, const char *text, int value,
        int count, int selected = 0);
};

#endif
