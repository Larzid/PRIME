#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void utilInitialize ();

#define PRIME_PATH_LENGTH 1024 /* apparently, don't believe PATH_MAX */
extern char UserDir[PRIME_PATH_LENGTH];

int RNG (int max);            /* 0 to max-1, inclusive */
int RNG (int min, int max);   /* min to max, inclusive */
int NDX (int n, int x);       /* rolls n dice with x faces on them */
int D20 (void);               /* rolls 20 sided dice */
const char *fmt (const char *format, ...);

void shuffle (void *array, int nmemb, int size);

extern char IsVowelLookupTable [256];

int mini (int x, int y);
int maxi (int x, int y);

#define SHBUFLEN 256

char *GetBuf ();

#define YOUR(_X) ((_X)->your())
#define THE(_X) ((_X)->the())
#define AN(_X) ((_X)->an())
#define THESE(_X) ((_X)->these())

#define isvowel(_c) IsVowelLookupTable[((int)_c)]
/* Expander for things like "mHelmet and mHelmet->isSealedArmor ()". */
#define NOT0(A,B) (A and A B)

/* Yeah, I shoulda used the STL vector...  Eit!  -- CADV */

template <class T>
class shVector
{
 public:

    shVector (int capacity = 4);
    ~shVector ();

    int add (T item);
    int count ();
    T get (int index);
    T *getPtr (int index);
    void set (int index, T item);
    int find (T item); /* returns idx of item, -1 if not found*/
    int remove (T item);
    T removeByIndex (int index);
    void reset ();
    void sort (int (* compareFunc) (T *, T *));
    void ensure (int capacity);
    void setCount (int size);

 protected:
    void grow (int newCapacity);

    T *mItems;
    int mCount;
    int mCapacity;
};


template <class T>
class shHeap : public shVector <T>  /* T must be a pointer type */
{
public:
    shHeap (int (*compareFunc) (T, T),
            int capacity = 4);
    int insert (T item);
    T extract ();

    int (* mCompareFunc) (T, T);
};


//constructor
template <class T>
shVector<T>::shVector (int capacity)
{
    mCapacity = capacity;
    mItems = (T*) malloc (sizeof (T) * mCapacity);
    mCount = 0;
}

template <class T>
shVector<T>::~shVector ()
{
    free (mItems);
}


template <class T>
void
shVector<T>::ensure (int capacity)
{
    if (mCapacity < capacity)
        grow (capacity);
}


/* used by SaveLoad.cpp */
template <class T>
void
shVector<T>::setCount (int size)
{
    ensure (size);
    mCount = size;
}


template <class T>
void
shVector<T>::grow (int newCapacity)
{
    mItems = (T*) realloc (mItems, sizeof(T) * newCapacity);
    mCapacity = newCapacity;
}


template <class T>
void
shVector<T>::reset ()
{
    mCount = 0;
}


template <class T>
int
shVector<T>::add (T item)
{
    if (++mCount == mCapacity) {
        grow (mCapacity * 2);
    }
    mItems[mCount-1] = item;
    return mCount - 1;
}


template <class T>
void
shVector<T>::set (int index, T item)
{
    ensure (index * 2);
    mItems[index] = item;
    if (mCount <= index) {
        mCount = index + 1;
    }
}

template <class T>
int
shVector<T>::count ()
{
    return mCount;
}

template <class T>
T
shVector<T>::get (int index)
{
    return mItems[index];
}

template <class T>
T *
shVector<T>::getPtr (int index)
{
    return &mItems[index];
}


template <class T>
int
shVector<T>::find (T item)
{
    int i;
    for (i = 0; i < mCount; i++) {
        if (item == mItems[i]) {
            return i;
        }
    }
    return -1;
}


template <class T>
int
shVector<T>::remove (T item)
{
    int i;
    for (i = 0; i < mCount; i++) {
        if (item == mItems[i]) {
            if (i < mCount-1) {
                memmove (&mItems[i], &mItems[i+1],
                         (mCount - i -1) * sizeof (T));
            }
            --mCount;
            return i;
        }
    }
    return -1;
}


template <class T>
T
shVector<T>::removeByIndex (int index)
{
    T item = mItems[index];
    if (index < mCount-1) {
        memmove (&mItems[index], &mItems[index+1],
                 (mCount - index - 1) * sizeof (T));
    }
    --mCount;
    return item;
}


void insertionsort (void *array, size_t nmemb, size_t size,
                    int (*compar) (const void *, const void *));

template <class T>
void
shVector<T>::sort (int (* compareFunc) (T *, T *))
{
    /* must be a stable sort (mergesort is preferred) */

    insertionsort (mItems, mCount, sizeof (T),
                   (int (*)(const void *, const void *)) compareFunc);
}


//constructor
template <class T>
shHeap<T>::shHeap (int (*compareFunc) (T, T),
                   int capacity)
    : shVector<T> (capacity)
{
    mCompareFunc = compareFunc;
}


template <class T>
int
shHeap<T>::insert (T item)
{
    if (++this->mCount == this->mCapacity) {
        this->grow (this->mCapacity * 2);
    }
    int i = this->mCount - 1;
    while (i > 0 and
           mCompareFunc (item, this->mItems[(i-1)/2]) < 0)
    {
        this->mItems[i] = this->mItems[(i-1)/2];
        i = (i-1) / 2;
    }
    this->mItems[i] = item;

    return i;
}


template <class T>
T
shHeap<T>::extract ()
{
    T res;
    int i, l, r, smallest;

    if (0 == this->mCount) {
        return NULL;
    }
    res = this->mItems[0];
    this->mItems[0] = this->mItems[--this->mCount];

    i = 0;
heapify:
    l = 2 * i + 1;
    r = 2 * i + 2;
    if (l < this->mCount and
        mCompareFunc (this->mItems[l], this->mItems[i]) < 0)
    {
        smallest = l;
    } else {
        smallest = i;
    }
    if (r < this->mCount and
        mCompareFunc (this->mItems[r], this->mItems[smallest]) < 0)
    {
        smallest = r;
    }
    if (smallest != i) {
        T tmp = this->mItems[i];
        this->mItems[i] = this->mItems[smallest];
        this->mItems[smallest] = tmp;
        i = smallest;
        goto heapify;
    }

    return res;
}


struct shLogFile
{
    FILE *mDbgFile;
    bool open ();
    void close ();
    void log (const char *format, ...);
};

extern struct shLogFile debug;
#endif
