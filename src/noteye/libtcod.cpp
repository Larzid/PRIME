// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2012 Zeno Rogue, see 'noteye.h' for details

#define SERVERONLY
#define NOAUDIO
#define LIBTCOD
// This file is supposed to be included from libtcod.
// You can also base a NotEye for your own game on it

static struct lua_State *LS_image;

#include "noteye.h"
#include "util.cpp"
#include "gfx.cpp"
#include "image.cpp"
#include "tiles.cpp"
#include "font.cpp"
#include "screen.cpp"

int imagenotfound(string s) {
  fprintf(stderr, "Image not found: %s\n", s.c_str());
  exit(2);
  }

#include "stream.cpp"

static TCPServer *server;

#define MAXCLIENT 256

static NTCPStream *client[MAXCLIENT];

extern "C" {

static Image  *img;
//static Font   *fnt;
static Screen *scr;

TCOD_noteye tnoteye;

static int  lastframe = 0; // time of last frame rendered
static bool delayed;       // was the sending of the last screen delayed?

int *ourfont;
int fontcount;

void TCOD_noteye_init() {

  if(scr) return;
  
  if(tnoteye.port == 0) tnoteye.port = 6678;
  if(tnoteye.quality == 0) tnoteye.quality = 256;
  if(tnoteye.minTicks == 0) tnoteye.minTicks = 50;
  if(tnoteye.name == NULL) tnoteye.name = "libtcod";
  if(tnoteye.maxClients <= 0) tnoteye.maxClients = 1;
  if(tnoteye.maxClients > MAXCLIENT) tnoteye.maxClients = MAXCLIENT;
  
  objs.push_back(NULL);
  objs.push_back(&gfx);  
  exsurface=SDL_CreateRGBSurface(SDL_SWSURFACE,16,16,32,0,0,0,0);

  img = new Image(8*32, 16*8);
  img->title = TCOD_ctx.font_file;
  registerObject(img);

  fontcount = TCOD_ctx.fontNbCharHoriz * TCOD_ctx.fontNbCharVertic;
  ourfont = new int[fontcount];

  for(int ch=0; ch<fontcount; ch++) {
    int ascii= TCOD_ctx.ascii_to_tcod[ch];
    
    int ti = 
      addTile(img, 
        (ascii%TCOD_ctx.fontNbCharHoriz)*TCOD_ctx.font_width, 
        (ascii/TCOD_ctx.fontNbCharHoriz)*TCOD_ctx.font_height, 
        TCOD_ctx.font_width, TCOD_ctx.font_height, 0);

    (byId<TileImage> (ti, LS_image))->chid = ch;

    ourfont[ch] = ti;
    }

  scr = new Screen;
  registerObject(scr);
  
  IPaddress ip;
  noteye_initnet();
  
  if(SDLNet_ResolveHost(&ip, NULL, tnoteye.port) != 0) {
    printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
    exit(1);
    }

  TCPsocket skt = SDLNet_TCP_Open(&ip);
  if(!skt) {
    printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
    exit(1);
    }
  
  server = new TCPServer(skt);
  }

int requal(int v) {
  int nq = tnoteye.quality;
  v *= nq;
  v /= 256;
  v *= 255;
  v /= (nq-1);
  return v;
  }

static int noteyecolor(const TCOD_color_t& color) {
  int i = 0;
  i += requal(color.r) << 16;
  i += requal(color.g) << 8;
  i += requal(color.b) << 0;
  return i;
  }

char_t *tbuffer;

static void TCOD_noteye_render() {
  TCOD_noteye_init();
  int sx = TCOD_console_get_width(NULL);
  int sy = TCOD_console_get_height(NULL);
  scr->setSize(sx, sy);
  char_t *buffer = tbuffer;
  for(int y=0; y<sy; y++) for(int x=0; x<sx; x++) {
    scr->get(x,y) = 
      addMerge(
//      addFill(0), addRecolor(fnt->ti[buffer->c], 0xFFFFFF)
        addFill(noteyecolor(buffer->back), 0xFFFFFF), addRecolor(ourfont[buffer->c], noteyecolor(buffer->fore), recDefault),
        false
        );
    buffer++;
    }
  lastframe = SDL_GetTicks();
  }

void TCOD_noteye_sendscreen(char_t *buffer) {

  if(!tnoteye.mode) return;
  TCOD_noteye_init();

  tbuffer = buffer;
  delayed = int(SDL_GetTicks()) < lastframe + tnoteye.minTicks;
  if(delayed) return;
  TCOD_noteye_render();
  for(int i=0; i<tnoteye.maxClients; i++) if(client[i]) {
    // printf("Sending a screen... objs = %d hash = %d:%d\n", size(objs), hashok, hashcol);
    client[i]->writeInt(nepScreen);
    client[i]->writeScr(scr);
    client[i]->flush();
    }
  }

void TCOD_noteye_check() {
  if(!tnoteye.mode) return;
  TCOD_noteye_init();

  if(delayed) TCOD_noteye_sendscreen(tbuffer);
  
  int i;
  for(i=0; i < MAXCLIENT; i++) if(client[i] == NULL) break;
  if(i < MAXCLIENT && i < tnoteye.maxClients) {
    TCPsocket skt = SDLNet_TCP_Accept(server->socket);
    if(skt) {
      client[i] = new NTCPStream(skt);
      client[i]->writeStr("NotEye stream");
      client[i]->writeInt(NOTEYEVER);
      client[i]->writeStr(tnoteye.name);
      if(scr) {
        TCOD_noteye_render();
        client[i]->writeInt(nepScreen);
        client[i]->writeScr(scr);
        client[i]->flush();
        }
      }
    }
  
  for(int i=0; i < tnoteye.maxClients; i++) while(client[i] && client[i]->ready()) {
    if(client[i]->eof()) {
      delete client[i];
      client[i] = NULL;
      }
    else {
      int nep = client[i]->readInt();
      if(nep == nepKey) {
        SDL_Event ev;
        SDL_KeyboardEvent& kev(ev.key);
        kev.keysym.sym = SDLKey(client[i]->readInt());
        kev.keysym.mod = SDLMod(client[i]->readInt());
        ev.type = client[i]->readInt() == evKeyDown ? SDL_KEYDOWN : SDL_KEYUP;
        kev.keysym.unicode = client[i]->readInt();
        SDL_PushEvent(&ev);
        }
      else if(nep == nepMouse) {
        tnoteye.x = client[i]->readInt();
        tnoteye.y = client[i]->readInt();
        tnoteye.state = client[i]->readInt();
        tnoteye.active = true;
        }
      else if(nep == nepMessage) {
        string s = client[i]->readStr();
        TCOD_noteye_writestr(s.c_str());
        }
      else fprintf(stderr, "Unknown NEP: %d\n", nep);
      }
    }
  }

void TCOD_noteye_writeint(int v) {
  for(int i=0; i<tnoteye.maxClients; i++) if(client[i]) 
    client[i]->writeInt(v);
  }

void TCOD_noteye_writestr(const char *v) {
  for(int i=0; i<tnoteye.maxClients; i++) if(client[i]) 
    client[i]->writeStr(v);
  }

void TCOD_noteye_flush() {
  for(int i=0; i<tnoteye.maxClients; i++) if(client[i]) 
    client[i]->flush();
  }

}

void noteye_halt() {}

TileImage::~TileImage() {}
