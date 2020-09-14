#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include "Util.h"


#ifdef _WIN32
#define random rand
#define srandom srand
#endif

int
RNG (int max)
{
    return random () % max;
}

int
RNG (int min, int max)
{
    return min + random () % (max + 1 - min);
}

int
NDX (int n, int x)
{
    int result = 0;
    while (n--) {
        result += RNG (1, x);
    }
    return result;
}

int
D20 (void)
{
    return random () % 20 + 1;
}


int
mini (int x, int y)
{
    return x < y ? x : y;
}


int
maxi (int x, int y)
{
    return x > y ? x : y;
}

const char *
fmt (const char *format, ...)
{
    char *buf = GetBuf ();
    va_list ap;
    va_start (ap, format);
    vsnprintf (buf, SHBUFLEN, format, ap);
    va_end (ap);
    return buf;
}

char IsVowelLookupTable[256];


void
utilInitialize ()
{
    srandom ((unsigned) time (NULL));

    /* The casts are silly looking, but without them some
       compilers report array subscript type char warning. */
    IsVowelLookupTable[int('a')] = 1;
    IsVowelLookupTable[int('e')] = 1;
    IsVowelLookupTable[int('i')] = 1;
    IsVowelLookupTable[int('o')] = 1;
    IsVowelLookupTable[int('u')] = 1;
}


void
shuffle (void *array, int nmemb, int size) {
    const int SIZE = 256;
    char tmp[SIZE];

    assert (size <= SIZE);

    for (int i = 0; i < nmemb; ++i) {
        int x = i + RNG (nmemb - i);
        memcpy (tmp, ((char *) array) + x * size, size);
        memcpy (((char *) array) + x * size,
                ((char *) array) + i * size, size);
        memcpy (((char *) array) + i * size, tmp, size);
    }
}


void
insertionsort (void *base, size_t nmemb, size_t size,
                int (*compar) (const void *, const void *))
{
    int i;
    unsigned int j;
    char tmp[20];

    assert (size < 20);

    for (j = 1; j < nmemb; j++) {
        memcpy (tmp, ((char *) base) + j * size, size);
        //memcpy (tmp, (void *) ((unsigned long) base + j * size), size);
        i = j - 1;
        while (i >= 0 and
               compar (((char *) base) + i * size, tmp) > 0)
//               compar ((void *) ((unsigned long) base + i * size), tmp) > 0)
        {
            memmove (((char *) base) + (i + 1) * size,
                     ((char *) base) + i * size, size);
/*            memmove ((void *) ((unsigned long) base + (i + 1) * size),
                     (void *) ((unsigned long) base + i * size), size);*/
            --i;
        }
        memcpy (((char *) base) + (i + 1) * size, tmp, size);
		//memcpy ((void *) ((unsigned long) base + (i + 1) * size), tmp, size);
    }
}



char *
GetBuf ()
{
    const int NUMBUFS = 64;
    static char buffers[NUMBUFS][SHBUFLEN];
    static int n = 0;

    if (n >= NUMBUFS)
        n = 0;

    return buffers[n++];
}


bool
shLogFile::open ()
{
    const char *name;
    name = NULL;
    name = getenv ("USER");
    if (!name) name = getenv ("USERNAME");
    char dbgfilename[256];
    snprintf (dbgfilename, 256, "%s/dbg.%s.txt", UserDir, name);
    mDbgFile = fopen (dbgfilename, "w");
    if (!mDbgFile) {
        fprintf (stderr, "Could not open %s.", dbgfilename);
        return false;
    }
    setvbuf (mDbgFile, (char *) NULL, _IOLBF, 0);
    return true;
}

void
shLogFile::log (const char *format, ...)
{
    va_list ap;
    va_start (ap, format);
    vfprintf (mDbgFile, format, ap);
    fputs ("\n", mDbgFile);
    va_end (ap);
}

void
shLogFile::close ()
{
    fclose (mDbgFile);
}
