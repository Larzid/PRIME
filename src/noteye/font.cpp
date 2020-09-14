// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

namespace noteye {

Font *newFont(Image *base, int inx, int iny, int trans) {
  Font *F = new Font;
  int sx = base->s ? base->s->w : 0;
  int sy = base->s ? base->s->h : 0;
  int dx = sx / inx;
  int dy = sy / iny;
  F->cnt = inx * iny;
  F->ti = new int[F->cnt];
  for(int i=0; i<inx*iny; i++) {
    F->ti[i] = 
      addTile(base, dx * (i % inx), dy * (i / inx), dx, dy, trans);
    (byId<TileImage> (F->ti[i], NULL))->chid = i;
    }
  return F;
  }

#ifdef USELUA
int lh_newfont(lua_State *L) {
  checkArg(L, 4, "newfont");
  return noteye_retObject(L, newFont(luaO(1, Image), luaInt(2), luaInt(3), luaInt(4)));
  }

int lh_getchar(lua_State *L) {
  checkArg(L, 2, "getchar");
  return noteye_retInt(L, luaO(1, Font)->ti[(unsigned char) (luaStr(2)[0])]);
  }

int lh_getcharav(lua_State *L) {
  checkArg(L, 2, "getchar");
  int i = luaInt(2);
  Font *f = luaO(1, Font);
  if(i < 0 || i >= f->cnt) return 0;
  return noteye_retInt(L, f->ti[i]);
  }
#endif

}
