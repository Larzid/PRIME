#define SETSQ(_x, _y, _w) mSquares[_x][_y].mTerr = _w
#define GETSQ(_x, _y) mSquares[_x][_y].mTerr
#define SETSQFLAG(_x, _y, _f) mSquares[_x][_y].mFlags |= shSquare:: _f
#define GETSQFLAG(_x, _y, _f) (mSquares[_x][_y].mFlags & shSquare:: _f)
#define SETROOM(_x, _y, _r) (mSquares[_x][_y].mRoomId = _r)
#define SETSQROOM(_x, _y, _w, _r) \
    (mSquares[_x][_y].mTerr = _w); \
    (mSquares[_x][_y].mRoomId = _r)

#define TESTSQ(_x, _y, _t) \
(_x >= 0 and _x < mColumns and _y >= 0 and _y < mRows and (_t == GETSQ (_x, _y)))

#define TESTSQFLAG(_x, _y, _t) \
(_x >= 0 and _x < mColumns and _y >= 0 and _y < mRows \
and (GETSQFLAG (_x, _y, _t)))


