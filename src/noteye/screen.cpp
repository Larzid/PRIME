// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

namespace noteye {

bool debugon;

int outscr;

void Screen::setSize(int x, int y) {
  sx = x; sy = y;
  v.resize(sx * sy);
  for(int i=0; i<sx*sy; i++) v[i] = 0;
  }

Screen* newScreen(int x, int y) {
  Screen *s = new Screen;
  s->setSize(x, y);
  return s;
  }

void Screen::write(int x, int y, const char *buf, Font *f, int color) {
  int colorstack[128], qcolorstack = 1;
  while(*buf) {
    unsigned char ch = *(buf++);
    if(ch == '\v') {
      char ch2 = *(buf++);
      if(qcolorstack < 1) qcolorstack = 1;
      if(qcolorstack > 127) qcolorstack = 127;
      if(ch2 == 'v') ch = '\v';
      else if(ch2 == ':') ch = *(buf++);
      else if(ch2 >= '0' && ch2 <= '9') {
        colorstack[qcolorstack++] = color;
        color = vgacol[ch2 - '0'];
        continue;
        }
      else if(ch2 >= 'a' && ch2 <= 'f') {
        colorstack[qcolorstack++] = color;
        color = vgacol[ch2 - 'a' + 10];
        continue;
        }
      else if(ch2 >= 'A' && ch2 <= 'F') {
        colorstack[qcolorstack++] = color;
        color = vgacol[ch2 - 'A' + 10];
        continue;
        }
      else if(ch2 == 'p') {
        color = colorstack[--qcolorstack];
        continue;
        }
      else ch = '?';
      }
      
    int& c(get(x,y));
    int ti = f->ti[ch];
    c = addRecolor(ti, color, recDefault);
    x++;
    }
  }

int& Screen::get(int x, int y) {
  if(x < 0 || x >=sx || y < 0 || y >= sy) return outscr;
  return v[y*sx+x];
  }

// --- lua

#ifdef USELUA
int lh_newScreen(lua_State *L) {
  checkArg(L, 2, "newscreen");
  Screen *s = newScreen(luaInt(1), luaInt(2));
  return noteye_retObject(L, s);
  }

int lh_scrwrite(lua_State *L) {
  checkArg(L, 6, "scrwrite");
  luaO(1, Screen)->write(luaInt(2), luaInt(3), luaStr(4), luaO(5, Font), luaInt(6));
  return 0;
  }

int lh_scrget(lua_State *L) {
  checkArg(L, 3, "scrget");
  return noteye_retInt(L, luaO(1, Screen)->get(luaInt(2), luaInt(3)));
  }

int lh_scrset(lua_State *L) {
  checkArg(L, 4, "scrset");
  int& C ( luaO(1, Screen)->get(luaInt(2), luaInt(3)) );
  C = luaInt(4);
  return 0;
  }

int lh_scrsetsize(lua_State *L) {
  checkArg(L, 3, "scrsetsize");
  luaO(1, Screen)->setSize(luaInt(2), luaInt(3));
  return 0;
  }

int lh_scrgetsize(lua_State *L) {
  checkArg(L, 1, "scrgetsize");
  Screen *scr = luaO(1, Screen);
  lua_newtable(L);
  noteye_table_setInt(L, "x", scr->sx);
  noteye_table_setInt(L, "y", scr->sy);
  return 1;
  }

int lh_scrcopy(lua_State *L) {
  checkArg(L, 9, "scrcopy");
  Screen *srcS = luaO(1, Screen);
  int srcX = luaInt(2);
  int srcY = luaInt(3);
  
  Screen *tgtS = luaO(4, Screen);
  int tgtX = luaInt(5);
  int tgtY = luaInt(6);
  
  int SX = luaInt(7);
  int SY = luaInt(8);
  
  for(int x=0; x<SX; x++) for(int y=0; y<SY; y++) {
    int& C1(srcS->get(srcX+x, srcY+y));
    int& C2(tgtS->get(tgtX+x, tgtY+y));
    lua_pushvalue(L, -1);
    lua_pushinteger(L, C1);
    lua_pushinteger(L, srcX+x);
    lua_pushinteger(L, srcY+y);
    
    if (lua_pcall(L, 3, 1, 0) != 0) {
      noteyeError(15, "error running scrcopy", lua_tostring(L, -1));
      return 0;
      }

    C2 = luaInt(-1);
    lua_pop(L, 1);
    }
  
  return 0;
  }

int lh_scrfill(lua_State *L) {
  checkArg(L, 6, "scrfill");
  
  Screen *tgtS = luaO(1, Screen);
  int tgtX = luaInt(2);
  int tgtY = luaInt(3);
  
  int SX = luaInt(4);  
  int SY = luaInt(5);
  
  int C = luaInt(6);
  
  for(int x=0; x<SX; x++) for(int y=0; y<SY; y++) {
    tgtS->get(tgtX+x, tgtY+y) = C;
    }
  
  return 0;
  }

int lh_scrsave(lua_State *L) {
  Screen *srcS = luaO(1, Screen);
  const char *s = luaStr(2);
  int mode = luaInt(3);
  FILE *f = fopen(s, "wt");
  if(!f) {
    fprintf(errfile, "could not save file '%s'\n", s);
    return 0;
    }

  int lcolor = getCol(srcS->get(0,0)) & 0xFFFFFF;

  fprintf(f, 
    mode ? "[tt][color=#%06x]" :
    "<html>\n"
    "<head>\n"
    "<meta name=\"generator\" content=\"Necklace of the Eye\">\n"
    "<title>HTML Screenshot</title>\n"
    "</head>\n"
    "<body bgcolor=#0><pre><font color=#%06x>\n", lcolor
    );
  
  for(int y=0; y<srcS->sy; y++) {
    for(int x=0; x<srcS->sx; x++) {
      int ncolor = getCol(srcS->get(x,y)) & 0xFFFFFF;
      char c = getChar(srcS->get(x,y));
      if(c == 0) c = 32;
      if(ncolor != lcolor && c != 32) {
        fprintf(f, mode ? "[/color][color=#%06x]" : "</font><font color=#%06x>", ncolor);
        lcolor = ncolor;
        }
      fprintf(f, "%c", c);
      }
    fprintf(f, "\n");
    }
  fprintf(f, mode ? "[/color][/tt]" : "</font></body></html>\n");

  fclose(f);
  return 0;
  }

int lh_drawScreen(lua_State *L) {
  Image *dest = luaO(1, Image);
  Screen *scr = luaO(2, Screen);
  int ox = luaInt(3); int oy = luaInt(4);
  int tx = luaInt(5); int ty = luaInt(6);
  
  drawmatrix M;
  M.tx = tx; M.ty = ty;
  M.txy = M.tyx = M.tzx = M.tzy = 0;
  
  for(int y=0; y<scr->sy; y++)
    for(int x=0; x<scr->sx; x++) {
      M.x = ox+x*tx; M.y = oy+y*ty;
      drawTile(dest, M, distill(scr->get(x,y), spFlat));
      }
  
  dest->changes++;
  return 0;
  }

int lh_drawScreenX(lua_State *L) {
  Image *dest = luaO(1, Image);
  Screen *scr = luaO(2, Screen);
  int ox = luaInt(3); int oy = luaInt(4);
  int tx = luaInt(5); int ty = luaInt(6);
  
  drawmatrix M;
  M.tx = tx; M.ty = ty;
  M.txy = M.tyx = M.tzx = M.tzy = 0;

  dest->changes++;
  for(int y=0; y<scr->sy; y++)
    for(int x=0; x<scr->sx; x++) {

      int& C1(scr->get(x, y));
      lua_pushvalue(L, -1);
      lua_pushinteger(L, C1);
      lua_pushinteger(L, x);
      lua_pushinteger(L, y);
      
      if (lua_pcall(L, 3, 1, 0) != 0) {
        noteyeError(16, "error running drawScreenX", lua_tostring(L, -1));
        return 0;
        }
  
      int C2 = luaInt(-1);
      lua_pop(L, 1);
      
      M.x = ox+x*tx; M.y = oy+y*ty;
      drawTile(dest, M, distill(C2, spFlat));
      }
  
  return 0;
  }

int lh_drawTile(lua_State *L) {
  Image *dest = luaO(1, Image);
  drawmatrix M;
  M.x = luaInt(3); M.y = luaInt(4);
  M.tx = luaInt(5); M.ty = luaInt(6);
  M.tyx = M.txy = M.tzx = M.tzy = 0;

  drawTile(dest, M, distill(luaInt(2), spFlat));
  dest->changes++;
  return 0;
  }
#endif

}
