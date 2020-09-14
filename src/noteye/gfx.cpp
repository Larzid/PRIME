// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

// on Mac and other architectures, use a temporary buffer
#ifdef MAC
#define USEGFXBUFFER
#endif

namespace noteye {

GFX gfx;

static SDL_Surface *exsurface;

static bool sdlerror = false;

int origsx, origsy;

void initMode() {

  if(sdlerror) return;
  
  if(exsurface) return;

  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(errfile, "Failed to initialize SDL: '%s'\n", SDL_GetError());
    sdlerror = true;
    return;
    }

  const SDL_VideoInfo *inf = SDL_GetVideoInfo();
  origsx = inf->current_w;
  origsy = inf->current_h;

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  SDL_EnableUNICODE(1);

  exsurface=SDL_CreateRGBSurface(SDL_SWSURFACE,16,16,32,0,0,0,0);
  if(exsurface == NULL) {
    fprintf(errfile, "CreateRGBSurface failed: %s\n", SDL_GetError());
    sdlerror = true;
    return;
    }
  }

#ifdef USELUA
int lh_findvideomode(lua_State *L) {
  int sx = luaInt(1);
  int sy = luaInt(2);
  
  initMode();
  
  SDL_Rect **modes;
  modes=SDL_ListModes(exsurface->format, SDL_FULLSCREEN);
  
  int nsx = 99999, nsy = 99999;
  
  if(modes == (SDL_Rect **)0 || modes == (SDL_Rect **)-1) {
    nsx = sx; nsy = sy;
    }
  else for(int i=0;modes[i];i++) {
    // if(i==0) printf("wanted: %d x %d\n", sx, sy);
    // printf("mode %d x %d\n", modes[i]->w, modes[i]->h);
    if (modes[i]->w >= sx && modes[i]->h >= sy) {
      if(modes[i]->w > nsx || modes[i]->h > nsy) continue;
      // printf("  chosen!\n");
      nsx = modes[i]->w;
      nsy = modes[i]->h;
      }
    }
  lua_newtable(L);
  noteye_table_setInt(L, "x", nsx);
  noteye_table_setInt(L, "y", nsy);
  return 1;
  }

int lh_origvideomode(lua_State *L) {
  lua_newtable(L);
  noteye_table_setInt(L, "x", origsx);
  noteye_table_setInt(L, "y", origsy);
  return 1;
  }

#ifdef USEGFXBUFFER
SDL_Surface *gfxbuffer;
SDL_Surface *gfxreal;
#endif

bool hadGL = false;

bool setvideomode(int sx, int sy, int flags) {
#ifdef OPENGL
  if(gfx.flags & SDL_OPENGL) disableGL();
#endif
  gfx.flags = flags;
  int iflags = flags;

  SDL_Surface *s = SDL_SetVideoMode(sx, sy, 32, iflags);
  if(!s) return false;
  gfx.locked = false;

#ifndef USEGFXBUFFER
  gfx.s = s;

#else
  if(gfxbuffer) SDL_FreeSurface(gfxbuffer);
  gfxbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, sx, sy, 32, 0xFF<<16,0xFF<<8,0xFF,0xFF<<24);
  gfx.s = gfxbuffer;
  gfxreal = s;
  #endif

#ifdef OPENGL
  if(gfx.flags & SDL_OPENGL) initOrthoGL();
#endif

  return true;
  }

int lh_setvideomode(lua_State *L) {
  checkArg(L, 3, "setvideomode");

  int sx = luaInt(1);
  int sy = luaInt(2);
  bool full = luaBool(3);

#ifdef OPENGL
  gfx.flags |= SDL_OPENGL | SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER;
#endif

  return noteye_retBool(L, setvideomode(sx, sy, (full ? SDL_FULLSCREEN : 0)));
  }

int lh_setvideomodex(lua_State *L) {
  checkArg(L, 3, "setvideomodex");

  return noteye_retBool(L, setvideomode(luaInt(1), luaInt(2), luaInt(3)));
  }

int lh_closevideomode(lua_State *L) {
  checkArg(L, 1, "closevideomode");
  gfx.s = NULL;
  return 0;
  }

int lh_setwindowtitle(lua_State *L) {
  checkArg(L, 1, "setwindowtitle");
  const char *s = luaStr(1);
  SDL_WM_SetCaption(s, s);
  return 0;
  }

bool GFX::checkEvent(lua_State *L) {
  initMode();
  if(sdlerror) return false;
  fflush(logfile);
  SDL_Event ev;
  if(SDL_PollEvent(&ev)) {
    if(ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
      SDL_KeyboardEvent& kev(ev.key);
      lua_newtable(L);
      noteye_table_setInt(L, "type", ev.type == SDL_KEYDOWN ? evKeyDown : evKeyUp);
      noteye_table_setInt(L, "symbol", kev.keysym.sym);
      noteye_table_setInt(L, "mod", kev.keysym.mod);
      noteye_table_setInt(L, "chr", kev.keysym.unicode);
      return true;
      }
    
    if(ev.type == SDL_VIDEORESIZE) {
      lua_newtable(L);
      noteye_table_setInt(L, "type", evResize);
      noteye_table_setInt(L, "sx", ev.resize.w);
      noteye_table_setInt(L, "sy", ev.resize.h);
      return true;
      }

    if(ev.type == SDL_ACTIVEEVENT) {
      lua_newtable(L);
      noteye_table_setInt(L, "type", evActive);
      noteye_table_setInt(L, "gain", ev.active.gain);
      noteye_table_setInt(L, "state", ev.active.state);
      return true;
      }

    if(ev.type == SDL_MOUSEMOTION) {
      SDL_MouseMotionEvent& mev(ev.motion);
      lua_newtable(L);
      noteye_table_setInt(L, "type", evMouseMotion);
      noteye_table_setInt(L, "x", mev.x);
      noteye_table_setInt(L, "y", mev.y);
      noteye_table_setInt(L, "state", mev.state);
      return true;
      }

    if(ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
      SDL_MouseButtonEvent& mev(ev.button);
      lua_newtable(L);
      noteye_table_setInt(L, "type", ev.type == SDL_MOUSEBUTTONDOWN ? evMouseDown : evMouseUp);
      noteye_table_setInt(L, "x", mev.x);
      noteye_table_setInt(L, "y", mev.y);
      noteye_table_setInt(L, "state", SDL_GetMouseState(NULL, NULL));
      noteye_table_setInt(L, "button", mev.button);
      return true;
      }

    if(ev.type == SDL_QUIT) {
      lua_newtable(L);
      noteye_table_setInt(L, "type", evQuit);
      return true;
      }
    }
  return false;
  }

int lh_enablekeyrepeat(lua_State *L) {
  checkArg(L, 2, "enablekeyrepeat");
  
  int delay = luaInt(1);
  int interval = luaInt(2);

  if(delay == -1) delay = SDL_DEFAULT_REPEAT_DELAY;
  if(interval == -1) interval = SDL_DEFAULT_REPEAT_INTERVAL;
  
  SDL_EnableKeyRepeat(delay, interval);

  return 0;
  }

int lh_updaterect(lua_State *L) {
  gfx.setLock(false);
  if(gfx.s == NULL) return 0;
  checkArg(L, 4, "updaterect");
  SDL_Rect rect;
  rect.x = luaInt(1);
  rect.y = luaInt(2);
  rect.w = luaInt(3);
  rect.h = luaInt(4);
  
#ifdef OPENGL
  if(gfx.flags & SDL_OPENGL) {
    refreshGL();
    return 0;
    }
#endif

#ifndef USEGFXBUFFER
  SDL_UpdateRects(gfx.s, 1, &rect);
#else
  SDL_SetColorKey(gfxbuffer, 0, 0);
  SDL_SetAlpha(gfxbuffer, 0, 0);
  SDL_BlitSurface(gfxbuffer, &rect, gfxreal, &rect);
  SDL_UpdateRects(gfxreal, 1, &rect);
#endif

  return 0;
  }
#endif

}
