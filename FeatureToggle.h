/* Save on crash feature.  May need to be turned off if PRIME crashes
   right after running the executable. */
#define CATCH_SIGSEGV
/* When recompiling delete: main.o */

/* Playing through PuTTY or from some terminals causes number pad to behave
   in very bad manner.  Defining this prevents using NCurses keypad translation
   and allows using number pad in most cases. */
//#define NO_CURSES_KEYPAD
/* When recompiling delete: NCUI.o */

/* The NO_CURSES_KEYPAD define is planned to become an option from 2.3. */

/* Default to I/O handled by Free Pascal libs. */
//#define USE_FPC
/* When recompiling delete: main.o */
