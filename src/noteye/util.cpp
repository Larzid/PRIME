// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

using namespace std;

namespace noteye {

static vector<int> eventobjs;

static vector<Object*> objs;
static const char *lastfn;

// critical errors
static FILE *errfile;

// log file
static FILE *logfile;

template<class T> int size(T& x) { return x.size(); }

extern "C" {

extern noteyehandler noteyeErrorHandler;

static void noteyeError(int id, const char *b1, const char *b2, int param = NOPARAM);

Object* noteye_getobj(int id) {
  if(id <= 0 || id > (int) objs.size()) {
    noteyeError(20, "no such object", lastfn, id);
    return NULL;
    }
  return objs[id];
  }
  
Object* noteye_getobjd(int id) {
  if(id <= 0 || id > (int) objs.size()) return NULL;
  return objs[id];
  }

void noteye_wrongclass(int id, lua_State *L) {
#ifdef USELUA
  if(L) {
    static char buf[256];
    sprintf(buf, "object %d of wrong class in %s", id, lastfn);
    lua_pushstring(L, buf);
    lua_error(L);
    }
#endif
  noteyeError(2, "object of wrong class", lastfn, id);
  }

void deleteobj(int idx) {
  delete objs[idx];
  objs[idx] = NULL;
  }

static noteyecolor ZZ;

}

noteyecolor& qpixel(SDL_Surface *surf, int x, int y) {
  if(x<0 || y<0 || x >= surf->w || y >= surf->h) return ZZ;
  char *p = (char*) surf->pixels;
  p += y * surf->pitch;
  noteyecolor *pi = (noteyecolor*) (p);
  return pi[x];
  }

// should probably be changed for big endian architectures...
uint8& part(noteyecolor& col, int i) {
  uint8* c = (uint8*) &col;
  return c[i];
  }

void applygamma(uint8& col, int gamma) {
  if(!gamma) return;
  col = int(255 * pow((col/255.0), exp(gamma/-64.0)));
  }

void recolor(noteyecolor& col, noteyecolor ncol, int mode) {

  if(mode == recMult) {
    for(int p=0; p<4; p++)
      part(col, p) = (part(col, p) * part(ncol, p) + 127) / 255;
    return;
    }

  if(mode == recPurple)
    if(!(part(col,0) == part(col,2) && part(col,0) > part(col,1)))
      return;
    
  if(mode == recHue) {
    int c0 = part(col, 0) & 255;
    int c1 = part(col, 1) & 255;
    int c2 = part(col, 2) & 255;
    
    int mi = min(min(c0, c1), c2);
    int ma = max(max(c0, c1), c2);
    
    if(mi == ma) return;
    
    float hue = 0;
    float d = ma-mi;
    
    // 0 GROW 1
    if(c0 == mi && c2 == ma) hue = (c1-mi)/d;
    // GROW 1 1
    if(c0 == mi && c1 == ma) hue = 1 + (ma-c2)/d;
    // 1 1 FALL
    if(c2 == mi && c1 == ma) hue = 2 + (c0-mi)/d;
    // 1 FALL 0
    if(c0 == ma && c2 == mi) hue = 3 + (ma-c1)/d;
    // FALL 0 0
    if(c1 == mi && c0 == ma) hue = 4 + (c2-mi)/d;
    // 0 0 GROW
    if(c1 == mi && c2 == ma) hue = 5 + (ma-c0)/d;
    
    // bool b = c0 == 61 && c1 == 98 && c2 == 144;
    
    hue += (ncol % 1536) / 256.;
    while(hue >= 6) hue -= 6;
    while(hue < 0) hue += 6;
    
    int fr = int(mi + (ma-mi) * (hue-floor(hue)));
    int f2 = ma+mi-fr;
    
    if(hue < 1)      c0 = mi, c1 = fr, c2 = ma;
    else if(hue < 2) c0 = mi, c1 = ma, c2 = f2;
    else if(hue < 3) c0 = fr, c1 = ma, c2 = mi;
    else if(hue < 4) c0 = ma, c1 = f2, c2 = mi;
    else if(hue < 5) c0 = ma, c1 = mi, c2 = fr;
    else             c0 = f2, c1 = mi, c2 = ma;

    part(col, 0) = c0, part(col, 1) = c1, part(col, 2) = c2;
    return;
    }

  if(mode == recGamma) {
    for(int i=0; i<4; i++)
      applygamma(part(col, i), int(part(ncol, i)) - 128);
    return;
    }

  uint8 cmax = 0, cmin = 255;
  for(int p=0; p<3; p++)
    cmax = max(cmax, part(col,p)), 
    cmin = min(cmin, part(col,p));
  for(int p=0; p<3; p++)
    part(col,p) = cmin + ((1+cmax-cmin) * part(ncol, p))/ 256;
  }

void mixcolor(noteyecolor& col, noteyecolor ncol) {
  col = ((col&0xFEFEFE) + (ncol&0xFEFEFE)) >> 1;
  }

uint8 mixpart(uint8 a, uint8 b, uint8 c) {
  return (a*(255-c)+b*c+127)/255;
  }

bool istrans(noteyecolor a, noteyecolor b) { return (a&0xFFFFFF) == (b&0xFFFFFF); }

bool istransA(noteyecolor a, noteyecolor b) { 
  if(b == transAlpha) return part(a, 3) == 0;
  return (a&0xFFFFFF) == (b&0xFFFFFF); 
  }

void mixcolorAt(noteyecolor& col, noteyecolor ncol, noteyecolor p) {
  for(int i=0; i<4; i++)
    part(col,i) = mixpart(part(col,i), part(ncol,i), part(p, i));
  }

typedef unsigned char uint8;

void alphablend(noteyecolor& col, noteyecolor ncol) {
  int s3 = part(ncol, 3);
  for(int i=0; i<3; i++) {
    part(col, i) = 
      (part(col, i) * (255-s3) + part(ncol, i) * s3 + 128) >> 8;
    }
  part(col, 3) = s3;
  }

void alphablendc(noteyecolor& col, noteyecolor ncol, bool cache) {
  if(cache) col = ncol;
  else alphablend(col, ncol);
  }

SDL_Surface* newImage(int x, int y) {
  SDL_Surface *surf = SDL_CreateRGBSurface(SDL_HWSURFACE, x, y, 32, 0,0,0,0);
  if(!surf)
    noteyeError(3, "failed to create an image", NULL);
  return surf;
  }

int sti(string s) { return atoi(s.c_str()); }
int sth(string s) { int ret; sscanf(s.c_str(), "%x", &ret); return ret; }

int tty_fix();

noteyecolor vgacol[16] = {
  0x0000000, 0x10000aa, 0x200aa00, 0x300aaaa, 0x4aa0000, 0x5aa00aa, 0x6aa5500, 0x7aaaaaa,
  0x8555555, 0x95555ff, 0xA55ff55, 0xB55ffff, 0xCff5555, 0xDff55ff, 0xEffff55, 0xFffffff
  };

#define Get(T, name, id) \
  T* name = dbyId<T> (id)

// #include <typeinfo>

extern "C" {

int registerObject(Object *o) {
  int res = objs.size();
  // printf("O:%d: %p [%s]\n", res, o, typeid(*o).name());
  objs.push_back(o);
  return o->id = res;
  }

#ifdef USELUA
// lua utils

int noteye_retInt(lua_State *L, int i) {
  lua_pushinteger(L, i);
  return 1;
  }

int noteye_retBool(lua_State *L, bool b) {
  lua_pushboolean(L, b);
  return 1;
  }

int noteye_retStr(lua_State *L, const char *s) {
  lua_pushstring(L, s);
  return 1;
  }

int noteye_retObject(lua_State *L, Object *o) {
  return noteye_retInt(L, registerObject(o));
  }

int retObjectEv(lua_State *L, Object *o) {
  int id = registerObject(o);
  eventobjs.push_back(id);
  return noteye_retInt(L, id);
  }

void checkArg(lua_State *L, int q, const char *fname) {
  lastfn = fname;
  if(lua_gettop(L) != q) {
    noteyeError(4, "bad number of arguments", fname, q);
    }
  }

int noteye_argcount(lua_State *L) {
  return lua_gettop(L);
  }

void noteye_table_setInt (lua_State *L, const char *index, int value) {
  lua_pushstring(L, index);
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

void noteye_table_setIntAtInt (lua_State *L, int index, int value) {
  lua_pushinteger(L, index);
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

void noteye_table_setNum (lua_State *L, const char *index, double value) {
  lua_pushstring(L, index);
  lua_pushnumber(L, value);
  lua_settable(L, -3);
}

void noteye_table_setNumAtInt (lua_State *L, int index, double value) {
  lua_pushinteger(L, index);
  lua_pushnumber(L, value);
  lua_settable(L, -3);
}

void noteye_table_setStr (lua_State *L, const char *index, const char *str) {
  lua_pushstring(L, index);
  lua_pushstring(L, str);
  lua_settable(L, -3);
}

void noteye_table_setStrAtInt (lua_State *L, int index, const char *str) {
  lua_pushinteger(L, index);
  lua_pushstring(L, str);
  lua_settable(L, -3);
}

void noteye_table_setBool (lua_State *L, const char *index, bool b) {
  lua_pushstring(L, index);
  lua_pushboolean(L, b);
  lua_settable(L, -3);
}

void noteye_table_setBoolAtInt (lua_State *L, int index, bool b) {
  lua_pushinteger(L, index);
  lua_pushboolean(L, b);
  lua_settable(L, -3);
}

void noteye_table_opensub (lua_State *L, const char *index) {
  lua_pushstring(L, index);
  lua_newtable(L);
  }

void noteye_table_opensubAtInt (lua_State *L, int index) {
  lua_pushinteger(L, index);
  lua_newtable(L);
  }

void noteye_table_closesub (lua_State *L) {
  lua_settable(L, -3);
  }

int noteye_table_new(lua_State *L) { lua_newtable(L); return 1; }

bool havefield(lua_State *L, const char *key) {
  bool result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = lua_isnil(L, -1);
  lua_pop(L, 1);
  return result;
  }

int noteye_argInt(lua_State *L, int x) {
#ifdef RASPBERRY_FIX
  return (int) (long long) luaL_checknumber(L, x);
#endif
  return (int) luaL_checkint(L, x);
  }

long double noteye_argNum(lua_State *L, int x) {
  return luaL_checknumber(L, x);
  }

bool noteye_argBool(lua_State *L, int x) {
  return (bool) lua_toboolean(L, x);
  }

const char *noteye_argStr(lua_State *L, int x) {
  return luaL_checkstring(L, x);
  }

#endif

#define ERRBUFSIZE 2048
static char noteyeerrbuf[ERRBUFSIZE];

void noteyeError(int id, const char *b1, const char *b2, int param) {
  if(b2 && param != NOPARAM)
    snprintf(noteyeerrbuf, ERRBUFSIZE, "%s [%s] %d", b1, b2, param);
  else if(b2)
    snprintf(noteyeerrbuf, ERRBUFSIZE, "%s [%s]", b1, b2);
  else if(param != NOPARAM)
    snprintf(noteyeerrbuf, ERRBUFSIZE, "%s [%d]", b1, param);
  else 
    snprintf(noteyeerrbuf, ERRBUFSIZE, "%s", b1);
  (*noteyeErrorHandler) (id, b1, b2, param);
  }

void noteyeError2(int id, const char *b1, const char *b2, int param) {
  fprintf(errfile, "NotEye error #%d: %s", id, b1);
  if(b2) fprintf(errfile, " [%s]", b2);
  if(param != NOPARAM) fprintf(errfile, " [%d]", param);
  fprintf(errfile, "\n");
  noteye_halt();
  exit(1);
  }

noteyehandler noteyeErrorHandler = &noteyeError2;

}

int noteye_retStr(lua_State *L, string s) {
  lua_pushstring(L, s.c_str());
  return 1;
  }

int getfieldInt (lua_State *L, const char *key) {
  int result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = luaInt(-1);
  lua_pop(L, 1);
  return result;
  }

int getfieldInt (lua_State *L, const char *key, int def) {
  int result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = lua_isnil(L, -1) ? def : luaInt(-1);
  lua_pop(L, 1);
  return result;
  }

long double getfieldNum (lua_State *L, const char *key) {
  long double result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = luaNum(-1);
  lua_pop(L, 1);
  return result;
  }

long double getfieldNum (lua_State *L, const char *key, long double def) {
  long double result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = lua_isnil(L, -1) ? def : luaNum(-1);
  lua_pop(L, 1);
  return result;
  }

}
