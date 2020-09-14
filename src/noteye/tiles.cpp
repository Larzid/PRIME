// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

namespace noteye {

#define HASHMAX 65535

#define COLMOD 65519

void TileImage::debug() {
  printf("%d: image from %d [%s]\n", id, i->id, i->title.c_str());
  }

int TileMerge::hash() const {
  return (t1 ^ (t2 * 13157)) & HASHMAX;
  }

void Deb(int x) { 
  Get(Tile, T, x); 
  if(T) T->debug();
  else printf("%d: not a tile\n", x);
  }


void TileMerge::debug() {
  printf("%d: merge %d + %d\n", id, t1, t2);
  Deb(t1); Deb(t2);
  }

int TileFill::hash() const {
  return (alpha + (color % COLMOD)) & HASHMAX;
  }

void TileFill::debug() {
  printf("%d: color %8x alpha %8x\n", id, color, alpha);
  }

int TileRecolor::hash() const {
  return ((color % COLMOD) ^ (mode + t1 * 13157)) & HASHMAX;
  }

void TileRecolor::debug() {
  printf("%d: recolor %d, color %8x mode %d\n", id, t1, color, mode);
  Deb(t1);
  }

int TileSpatial::hash() const {
  return (t1+sf) & HASHMAX;
  }

void TileSpatial::debug() {
  printf("%d: %d, spatial %8x\n", id, t1, sf);
  Deb(t1);
  }

int TileLayer::hash() const {
  return (t1+layerid) & HASHMAX;
  }

void TileLayer::debug() {
  printf("%d: %d, layer %d\n", id, t1, layerid);
  Deb(t1);
  }

int TileTransform::hash() const {
  return (t1+1973) & HASHMAX;
  }

void TileTransform::debug() { 
  printf("%d: transform %d\n", id, t1);
  Deb(t1);
  }

int TileFreeform::hash() const {
  return (t1+par->id) & HASHMAX;
  }

void TileFreeform::debug() { 
  printf("%d: freeform %d\n",id, t1);
  Deb(t1);
  }

Tile *hashtab[HASHMAX+1];

int addTile(Image *i, int ox, int oy, int sx, int sy, int trans) {
  if(sx == 0 || sy == 0) {
    if(logfile)
      fprintf(logfile, "WARNING: attept to create tile of size %dx%d", sx, sy);
    return 0;
    }
  TileImage *T = new TileImage(sx, sy);
  T->i = i;
  T->ox = ox;
  T->oy = oy;
  T->trans = trans;
  return registerObject(T);
  }

//--

bool eq(const TileMerge& a, const TileMerge& b) {
  return a.t1 == b.t1 && a.t2 == b.t2 && a.over == b.over;
  }

bool eq(const TileFill& a, const TileFill& b) {
  return a.color == b.color && a.alpha == b.alpha;
  }

bool eq(const TileRecolor& a, const TileRecolor& b) {
  return a.color == b.color && a.t1 == b.t1 && a.mode == b.mode;
  }

bool eq(const TileSpatial& a, const TileSpatial& b) {
  return a.t1 == b.t1 && a.sf == b.sf;
  }

bool eq(const TileLayer& a, const TileLayer& b) {
  return a.t1 == b.t1 && a.layerid == b.layerid;
  }

bool eq(const TileTransform& a, const TileTransform& b) {
  return a.t1 == b.t1 && a.dx == b.dx && a.dy == b.dy && a.sx == b.sx && a.sy == b.sy &&
    a.dz == b.dz && a.rot == b.rot;
  }

bool eq(const TileFreeform& a, const TileFreeform& b) {
  return a.t1 == b.t1 && a.par == b.par;
  }

int hashok = 0, hashcol = 0;

Tile::~Tile() { 
  if(nextinhash) nextinhash->previnhash = previnhash;
  if(previnhash) *previnhash = nextinhash; 
  }

template<class T> int registerTile(const T& x) {

  int hsh = x.hash();
  
  Tile** hso (&(hashtab[hsh]));
  Tile** hs = hso;
  while(*hs) {
    if((*hs)->previnhash != hs) printf("hashtable error!\n");
    T* y = dynamic_cast<T*> (*hs);
    if(y && eq(x,*y)) { 
      hashok++; 
      if(hs != hso) {
        // move to the front
        Tile *nih = (*hs)->nextinhash;
        if(nih) nih->previnhash = hs; // (*hs)->previnhash;
        *hs = nih;

        (*hso)->previnhash = &(y->nextinhash);
        y->nextinhash = *hso;
        y->previnhash = hso;
        *hso = y;
        }
      return y->id; 
      }
    else {
      hashcol++;
      hs = &(*hs)->nextinhash;
      }
    }
    
  T *xc = new T;
  *xc = x;
  xc->nextinhash = *hso;
  if(*hso) (*hso)->previnhash = &(xc->nextinhash);
  xc->previnhash = hso;
  *hso = xc;
  int id = registerObject(xc);
  xc->preprocess();
  return id;
  }

int addMerge(int t1, int t2, bool over) {
  // an optimization for merging zeros
  if(t1 == 0) return t2;
  if(t2 == 0) return t1;

  TileMerge T;
  T.t1 = t1;
  T.t2 = t2;
  T.over = over;
  
  return registerTile(T);
  }

int addLayer(int t1, int layerid) {
  if(t1 == 0) return 0;
  TileLayer TL;
  TL.t1 = t1;
  TL.layerid = layerid;
  return registerTile(TL);
  }

int addSpatial(int t1, int sf) {
  if(t1 == 0) return 0;
  TileSpatial TSp;
  TSp.t1 = t1;
  TSp.sf = sf;
  return registerTile(TSp);
  }

int addTransform(int t1, double dx, double dy, double sx, double sy, double dz, double rot) {
  TileTransform TT;
  TT.t1 = t1;
  TT.dx = dx; TT.dy = dy;
  TT.sx = sx; TT.sy = sy;
  TT.dz = dz; TT.rot = rot;
  return registerTile(TT);
  }

int addFreeform(int t1, FreeFormParam *p) {
  if(t1 == 0) return 0;
  TileFreeform TFF;
  TFF.t1 = t1;
  TFF.par = p;
  return registerTile(TFF);
  }

int cloneTransform(int t1, TileTransform *example) {
  if(t1 == 0) return 0;
  TileTransform TT = *example;
  TT.t1 = t1;
  TT.nextinhash = NULL;
  TT.previnhash = NULL;
  return registerTile(TT);
  }

//--

#define rectrans 0xDEBEEF

void TileRecolor::preprocess() {
  Get(TileImage, TIC, t1);
  if(TIC) {
    int sx = TIC->sx, sy = TIC->sy;
    Image *i = new Image(sx, sy, TIC->trans == transAlpha ? 0 : rectrans);
    totalimagecache += sx * sy;
    for(int y=0; y<sy; y++) for(int x=0; x<sx; x++) {
      noteyecolor pix = qpixel(TIC->i->s, TIC->ox+x, TIC->oy+y);
      if(istrans(pix, TIC->trans)) continue;
      recolor(pix, color, mode);
      qpixel(i->s, x, y) = pix;
      }
    i->id = -1;
    char buf[256];
    sprintf(buf, "[%08x %d] ", color, mode);
    i->title = buf + TIC->i->title;
    cache = addTile(i, 0, 0, sx, sy, TIC->trans == transAlpha ? transAlpha : rectrans);
    cachechg = TIC->i->changes;
    }
  else
    cache = 0;
  }

#include <typeinfo>

TileRecolor::~TileRecolor() {
  // if(cache) printf(":%d: cache #%d [%s]\n", id, cache, typeid(*objs[cache]).name());
  TileImage *Cache = dbyId<TileImage> (cache);
  if(Cache) {
    totalimagecache -= Cache->sx * Cache->sy;
    delete Cache->i;
    deleteobj(Cache->id);
    }  
  }

int addFill(int color, int alpha) {
  TileFill TF;
  TF.color = color;
  TF.alpha = alpha;
  TF.cache = NULL;
  return registerTile(TF);
  }

int addRecolor(int t1, int color, int mode) {
  
  // optimizations first
  if(color == -1) return t1;
  if(t1 == 0) return 0;
  
  Get(TileRecolor, TR, t1);
  if(TR && TR->mode == mode) return addRecolor(TR->t1, color, mode);

  Get(TileFill, TF, t1);
  if(TF) return addFill(color, TF->alpha);

  TileRecolor T;
  T.t1 = t1;
  T.color = color;
  T.mode = mode;
  T.cache = 0;
  
  return registerTile(T);
  }

int getCol(int x) {
  if(!x) return -1;
  
  Get(TileImage, TI, x);
  if(TI) return -1;

  Get(TileRecolor, TR, x);
  if(TR) return TR->color;
  
  Get(TileMerge, TM, x);
  if(TM) return getCol(TM->over ? TM->t1 : TM->t2);
  
  return 0;
  }

int getImage(int x) {
  if(!x) return 0;
  
  Get(TileImage, TI, x);
  if(TI) return TI->i->id;

  Get(TileRecolor, TR, x);
  if(TR) return getImage(TR->t1);
  
  Get(TileMerge, TM, x);
  if(TM) {
    int u = getImage(TM->t2);
    if(u) return u;
    return getImage(TM->t1);
    }
  
  return 0;
  }

int getChar(int x) {
  if(!x) return -1;
  
  Get(TileImage, TI, x);
  if(TI) return TI->chid;

  Get(TileRecolor, TR, x);
  if(TR) return getChar(TR->t1);
  
  Get(TileMerge, TM, x);
  if(TM) return getChar(TM->over ? TM->t1 : TM->t2);
  
  return 0;
  }

int getBak(int x) {
  //Get(TileRecolor, TR, x);
  //if(TR) return TR->color;
  
  Get(TileFill, TF, x);
  if(TF) return TF->color;
  
  Get(TileMerge, TM, x);
  if(TM) return getBak(TM->t1);
  
  return -1;
  }

int tileSetFont(int x, Font *f) {
  Get(TileImage, TI, x);
  if(TI && TI->chid >= 0 && TI->chid < 256) return f->ti[TI->chid];

  Get(TileRecolor, TR, x);
  if(TR) return addRecolor(tileSetFont(TR->t1, f), TR->color, TR->mode);
  
  Get(TileMerge, TM, x);
  if(TM) return addMerge(tileSetFont(TM->t1,f), tileSetFont(TM->t2,f), TM->over);
  
  return x;
  }

int distillLayer(int x, int layerid) {
  Get(TileMerge, TM, x);
  if(TM) return 
    addMerge( distillLayer(TM->t1, layerid), distillLayer(TM->t2, layerid), TM->over);

  Get(TileLayer, TL, x);
  if(TL) { if(TL->layerid == layerid) return TL->t1; else return 0; }
  
  Get(TileRecolor, TR, x);
  if(TR) return addRecolor( distillLayer(TR->t1, layerid), TR->color, TR->mode);
  
  Get(TileTransform, TT, x);
  if(TT) 
    return cloneTransform(distillLayer(TT->t1, layerid), TT);
  
  Get(TileFreeform, TFF, x);
  if(TFF)
    return addFreeform(distillLayer(TFF->t1, layerid), TFF->par);
  
  Get(TileSpatial, TS, x);
  if(TS) return addSpatial(distillLayer(TS->t1, layerid), TS->sf);
  
  if(layerid == 0) return x;
  return 0;
  }

int distill(int x, int sp) {

  Get(TileImage, TI, x);
  if(TI) return x;
  
  Get(TileRecolor, TR, x);
  if(TR && TR->cache) {
    Get(TileImage, TIR, TR->t1);
    if(TIR->i->changes != TR->cachechg) {
      // invalidate cache!
      Get(TileImage, TIRC, TR->cache);
      delete TIRC->i;
      deleteobj(TR->cache);
      TR->cache = 0;
      TR->preprocess();
      }
    }
  if(TR && TR->cache) return TR->cache;
  if(TR) {
    int ds = distill(TR->t1, sp);
    Get(TileMerge, TM, ds);
    if(TM) {
      return addMerge( distill(addRecolor(TM->t1, TR->color, TR->mode), sp), distill(addRecolor(TM->t2, TR->color, TR->mode), sp), TM->over);
      }
    
    Get(TileTransform, TT, ds);
    if(TT) {
      int t1 = distill(addRecolor(TT->t1, TR->color, TR->mode), sp);
      return cloneTransform(t1, TT);
      }

    Get(TileFreeform, TFF, ds);
    if(TFF) {
      int t1 = distill(addRecolor(TFF->t1, TR->color, TR->mode), sp);
      return addFreeform(t1, TFF->par);
      }

    Get(TileRecolor, TRe, ds);
    if(TRe) {
      return addRecolor(TRe->cache, TR->color, TR->mode);
      }

    int i = addRecolor(ds, TR->color, TR->mode);
    return distill(i, sp);
    }
  
  Get(TileSpatial, TS, x);
  if(TS && (TS->sf & sp)) return distill(TS->t1, sp);
  
  Get(TileLayer, TL, x);
  if(TL) return distill(TL->t1, sp);
  
  Get(TileMerge, TM, x);
  if(TM) return addMerge( distill(TM->t1, sp), distill(TM->t2, sp), TM->over);
  
  Get(TileFill, TF, x);
  if(TF) return x;
  
  Get(TileTransform, TT, x);
  if(TT) {
    return cloneTransform(distill(TT->t1, sp), TT);
    }
  
  Get(TileFreeform, TFF, x);
  if(TFF) {
    int di = distill(TFF->t1, sp);
    return addFreeform(di, TFF->par);
    }
  
  return 0;
  }

#ifdef USELUA

int lh_tileMerge(lua_State *L) {
  checkArg(L, 2, "tilemerge");
  return noteye_retInt(L, addMerge(luaInt(1), luaInt(2), false));
  }

int lh_tileMergeOver(lua_State *L) {
  checkArg(L, 2, "tilemergeover");
  return noteye_retInt(L, addMerge(luaInt(1), luaInt(2), true));
  }

int lh_addTile(lua_State *L) {
  checkArg(L, 6, "addtile");
  return noteye_retInt(L, addTile(luaO(1, Image), luaInt(2), luaInt(3), 
    luaInt(4), luaInt(5),
    luaInt(6)
    ));
  }

int lh_gch(lua_State *L) {
  int i = getChar(luaInt(1));
  char c = i == -1 ? 0 : i;
  lua_pushlstring(L, &c, 1);
  return 1;
  }

int lh_gchv(lua_State *L) {
  return noteye_retInt(L, getChar(luaInt(1)));
  }

int lh_gco(lua_State *L) {
  return noteye_retInt(L, getCol(luaInt(1)));
  }

int lh_gimg(lua_State *L) {
  return noteye_retInt(L, getImage(luaInt(1)));
  }

int lh_gba(lua_State *L) {
  return noteye_retInt(L, getBak(luaInt(1)));
  }

int lh_gp2(lua_State *L) {
  Get(TileMerge, T, luaInt(1));
  if(!T) return noteye_retInt(L, -1);
  return noteye_retInt(L, T->t2);
  }

int lh_gavcoba(lua_State *L) {
  noteye_retInt(L, getChar(luaInt(1)));
  noteye_retInt(L, getCol(luaInt(1)));
  noteye_retInt(L, getBak(luaInt(1)));
  return 3;
  }

int lh_tileavcobaf(lua_State *L) {
  int kv = luaInt(1);
  Font *F = luaO(4, Font);
  if(kv < 0 || kv >= F->cnt) kv = 32;
  return noteye_retInt(L, 
    addMerge(addFill(luaInt(3), 0xFFFFFF), addRecolor(F->ti[kv], luaInt(2), recDefault), false)
    );
  }

int lh_tileAlpha(lua_State *L) {
  checkArg(L, 2, "tileshadeof");
  int tc = luaInt(1);

  return noteye_retInt(L, addFill(tc, luaInt(2)));
  }

int lh_tileRecolor(lua_State *L) {
  checkArg(L, 3, "tilecol");
  return noteye_retInt(L, addRecolor(luaInt(1), luaInt(2), luaInt(3)));
  }

//--

int lh_tileSpatial(lua_State *L) {
  checkArg(L, 2, "tilespatial");
  return noteye_retInt(L, addSpatial(luaInt(1), luaInt(2)));
  }

int lh_tileLayer(lua_State *L) {
  checkArg(L, 2, "tilelayer");
  return noteye_retInt(L, addLayer(luaInt(1), luaInt(2)));
  }

int lh_getlayer(lua_State *L) {
  checkArg(L, 2, "getlayer");
  return noteye_retInt(L, distillLayer(luaInt(1), luaInt(2)));
  }

int lh_getdistill(lua_State *L) {
  checkArg(L, 2, "getdistill");
  return noteye_retInt(L, distill(luaInt(1), luaInt(2)));
  }

//--

int lh_tileTransform(lua_State *L) {
  checkArg(L, 7, "tilexf");
  return noteye_retInt(L, addTransform(
    luaInt(1), 
    (luaNum(2)),
    (luaNum(3)),
    (luaNum(4)),
    (luaNum(5)),
    (luaNum(6)),
    (luaNum(7))
    ));
  }

int lh_freeformparam(lua_State *L) {
  checkArg(L, 16, "freeformparam");
  FreeFormParam *P = new FreeFormParam;
  int i=1;
  for(int y=0; y<4; y++) for(int x=0; x<4; x++)
    P->d[y][x] = luaNum(i++);
  P->side = 4;
  P->shiftdown = false;
  return noteye_retInt(L, registerObject(P));
  }
  
int lh_freeformparamflags(lua_State *L) {
  checkArg(L, 3, "freeformparamflags");
  FreeFormParam *P = luaO(1, FreeFormParam);
  P->side = luaInt(2);
  P->shiftdown = luaBool(3);
  return 0;
  }
  
int lh_tileFreeform(lua_State *L) {
  checkArg(L, 2, "tilefreeform");
  return noteye_retInt(L, addFreeform(
    luaInt(1), luaO(2, FreeFormParam)));
  }
  
int lh_tiledebug(lua_State *L) {
  checkArg(L, 1, "tilemerge");
  luaO(1, Tile)->debug();
  return 0;
  }

#endif

TileImage::TileImage(int _sx, int _sy) {
  sx = _sx; sy = _sy; chid = '?';
  ox = 0; oy = 0; 
  gltexture = NULL;
  bcurrent = -1;
  }

void provideBoundingBox(TileImage *T) {
  if(T->bcurrent == T->i->changes) return;
  T->bcurrent = T->i->changes;

  T->bx = 0, T->by = 0;
  T->tx = T->sx, T->ty = T->sy;

  for(int ay=0; ay<T->sy; ay++) for(int ax=0; ax<T->sx; ax++)
    if(!istransA(qpixel(T->i->s, T->ox+ax, T->oy+ay), T->trans)) {
      if(ax < T->tx) T->tx = ax;
      if(ay < T->ty) T->ty = ay;
      if(ax >= T->bx) T->bx = ax+1;
      if(ay >= T->by) T->by = ay+1;
      }
  
  /* printf("bouding-box: %s(%d,%d) = (%d,%d) to (%d,%d)\n",
    T->i->title.c_str(), T->ox, T->oy, T->bx, T->by, T->tx, T->ty
    ); */
  }

int getFppDown(TileImage *T) {
  provideBoundingBox(T);
  return T->sy - T->by;
  }

int lh_getobjectinfo(lua_State *L) {
  checkArg(L, 1, "getobjectinfo");

  lua_newtable(L);
  
  int id = luaInt(1);

  Get(TileImage, TI, id);
  if(TI) {
    noteye_table_setInt(L, "type", 0x11);
    noteye_table_setInt(L, "ox", TI->ox);
    noteye_table_setInt(L, "oy", TI->oy);
    noteye_table_setInt(L, "sx", TI->sx);
    noteye_table_setInt(L, "sy", TI->sy);
    noteye_table_setInt(L, "ch", TI->chid);
    noteye_table_setInt(L, "trans", TI->trans);
    noteye_table_setInt(L, "i", TI->i->id);
    noteye_table_setInt(L, "bottom", getFppDown(TI));
    return 1;
    }

  Get(TileRecolor, TR, id);
  if(TR) {
    noteye_table_setInt(L, "type", 0x21);
    noteye_table_setInt(L, "t1", TR->t1);
    noteye_table_setInt(L, "mode", TR->mode);
    noteye_table_setInt(L, "color", TR->color);
    return 1;
    }
  
  Get(TileMerge, TM, id);
  if(TM) {
    noteye_table_setInt(L, "type", TM->over ? 0x18 : 0x12);
    noteye_table_setInt(L, "t1", TM->t1);
    noteye_table_setInt(L, "t2", TM->t2);
    return 1;
    }
  
  Get(TileSpatial, TSp, id);
  if(TSp) {
    noteye_table_setInt(L, "type", 0x14);
    noteye_table_setInt(L, "t1", TSp->t1);
    noteye_table_setInt(L, "sf", TSp->sf);
    return 1;
    }
  
  Get(TileLayer, TL, id);
  if(TL) {
    noteye_table_setInt(L, "type", 0x19);
    noteye_table_setInt(L, "t1", TL->t1);
    noteye_table_setInt(L, "sf", TL->layerid);
    return 1;
    }
  
  Get(TileFill, TF, id);
  if(TF) {
    noteye_table_setInt(L, "type", 0x20);
    noteye_table_setInt(L, "color", TF->color);
    noteye_table_setInt(L, "alpha", TF->alpha);
    return 1;
    }
  
  Get(Tile, TX, id);
  if(TX) {
    noteye_table_setInt(L, "type", 0x10);
    return 1;
    }

  noteye_table_setInt(L, "type", 0);
  return 1;
  }

}
