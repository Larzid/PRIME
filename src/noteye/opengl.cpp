// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

#ifdef OPENGL
#ifdef MAC
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif

namespace noteye {

// on some machines you need to use texture sizes which are powers of 2
#define POWEROFTWO

extern GFX gfx;

bool useGL(Image *i) {
  return (i == &gfx) && (gfx.flags & SDL_OPENGL);
  }

void glError(const char* GLcall, const char* file, const int line) {
  GLenum errCode = glGetError();
  if(errCode!=GL_NO_ERROR) {
    if(logfile) fprintf(logfile, "OPENGL ERROR #%i: in file %s on line %i :: %s\n",errCode,file, line, GLcall);
    fprintf(stderr, "OPENGL ERROR #%i: in file %s on line %i :: %s\n",errCode,file, line, GLcall);
    }
  }
#define GLERR(call) glError(call, __FILE__, __LINE__)

void initOrthoGL() {
  glClearColor(0, 0, 0, 0);
  glClearDepth(1.0f); 
  glViewport(0, 0, gfx.s->w, gfx.s->h); 
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity(); 
  glOrtho(0, gfx.s->w, gfx.s->h, 0, 1, -1); 
  glMatrixMode(GL_MODELVIEW); 
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D); 
  }

struct GLtexture {
  GLuint name;
  int cachechg;
  int maxxn, maxxd, maxyn, maxyd;
  };

void deleteTextureGL(TileImage *T) {
  if(T->gltexture) {
    glDeleteTextures(1, &T->gltexture->name);
    delete T->gltexture;
    T->gltexture = NULL;
    }
  }

void genTextureGL(TileImage *T) {
  GLERR("bitmap");
  if(!T->gltexture) {
    T->gltexture = new GLtexture;
    glGenTextures(1, &T->gltexture->name);
    T->gltexture->cachechg = -2;
    }
  if(T->gltexture->cachechg == T->i->changes) {
    glBindTexture(GL_TEXTURE_2D, T->gltexture->name);
    return;
    }
  int tsx = T->sx, tsy = T->sy;
#ifdef POWEROFTWO
  int i = 0;
  while(tsx > 1) i++, tsx >>= 1;
  tsx = 2; while(i) i--, tsx <<= 1;
  while(tsy > 1) i++, tsy >>= 1;
  tsy = 2; while(i) i--, tsy <<= 1;
  T->gltexture->maxxn = T->sx;
  T->gltexture->maxxd = tsx;
  T->gltexture->maxyn = T->sy;
  T->gltexture->maxyd = tsy;
#else
  T->gltexture->maxxn = 1;
  T->gltexture->maxyn = 1;
  T->gltexture->maxxd = 1;
  T->gltexture->maxyd = 1;
#endif
  T->gltexture->cachechg = T->i->changes;
  int *bitmap = new int[tsx * tsy];
  int *p = bitmap;
#ifdef POWEROFTWO
  for(int y=0; y<tsy; y++) for(int x=0; x<tsx; x++) *(p++) = 0;
  p = bitmap;
#endif
  SDL_Surface *src = T->i->s;
  for(int y=0; y<T->sy; y++) {
    for(int x=0; x<T->sx; x++) {    
      int px = qpixel(src, T->ox+x, T->oy+y);
      if(T->trans == transAlpha) *(p++) = px;
      else if(istrans(px, T->trans)) *(p++) = 0;
      else *(p++) = 0xFF000000 | px;
      }
    p += (tsx-T->sx);
    }
  glBindTexture(GL_TEXTURE_2D, T->gltexture->name);
  GLERR("bitmap");

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tsx, tsy, 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap);
  GLERR("bitmap");
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  GLERR("bitmap");
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  delete[] bitmap;
  GLERR("bitmap");
  }

void disableGL() {
  for(int i=0; i<(int) objs.size(); i++) {
    TileImage *TI = dbyId<TileImage> (i);
    if(TI) deleteTextureGL(TI);
    }
  }

void drawTileImageGL(Image *dest, drawmatrix &M, TileImage *TI) {
  genTextureGL(TI);
  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  glColor4f(1,1,1,1);

#ifdef POWEROFTWO_NOFIX
  float dx = (1.0*TI->gltexture->maxxn) / TI->gltexture->maxxd;
  float dy = (1.0*TI->gltexture->maxyn) / TI->gltexture->maxyd;
  glTexCoord2f(0, 0); glVertex3f(M.x, M.y, 0);
  glTexCoord2f(dx, 0); glVertex3f(M.x+M.tx, M.y+M.txy, 0);
  glTexCoord2f(dx, dy); glVertex3f(M.x+M.tx+M.tyx, M.y+M.ty+M.txy, 0);
  glTexCoord2f(0, dy); glVertex3f(M.x+M.tyx, M.y+M.ty, 0);

#else
  int txx = M.tx*TI->gltexture->maxxd/TI->gltexture->maxxn;
  int txy = M.txy*TI->gltexture->maxxd/TI->gltexture->maxxn;
  int tyy = M.ty*TI->gltexture->maxyd/TI->gltexture->maxyn;
  int tyx = M.tyx*TI->gltexture->maxyd/TI->gltexture->maxyn;
  glTexCoord2f(0, 0); glVertex3f(M.x, M.y, 0);
  glTexCoord2f(1, 0); glVertex3f(M.x+txx, M.y+txy, 0);
  glTexCoord2f(1, 1); glVertex3f(M.x+txx+tyx, M.y+tyy+txy, 0);
  glTexCoord2f(0, 1); glVertex3f(M.x+tyx, M.y+tyy, 0);
  
  #endif
  glEnd(); glGetError();
  }

void drawFillGL(Image *dest, drawmatrix &M, TileFill *TF) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  noteyecolor col = TF->color;
  glColor4f(
    part(col,2)/255.0,
    part(col,1)/255.0,
    part(col,0)/255.0,
    (part(TF->alpha,0)+part(TF->alpha,1)+part(TF->alpha,2))/765.0
    );
  glVertex3f(M.x, M.y, 0);
  glVertex3f(M.x+M.tx, M.y+M.txy, 0);
  glVertex3f(M.x+M.tx+M.tyx, M.y+M.ty+M.txy, 0);
  glVertex3f(M.x+M.tyx, M.y+M.ty, 0);
  glEnd(); glGetError();
  }

void fillRectGL(int x, int y, int w, int h, noteyecolor col) {
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glBegin(GL_QUADS);
  glColor3f(part(col,2)/255.0, part(col,1)/255.0, part(col,0)/255.0);
  glVertex3f(x, y, 0);
  glVertex3f(x+w, y, 0);
  glVertex3f(x+w, y+h, 0);
  glVertex3f(x, y+h, 0);
  glEnd();
  }

extern viewpar V;

static fpoint4 addShift(fpoint4 o, fpoint4 y, TileImage *w);

void renderAffineImageGL(TileImage *w, fpoint4 orig, fpoint4 ox, fpoint4 oy) {
  genTextureGL(w);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  if(w->sx == 1 && w->sy == 1) {
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    // for some reason OpenGL does not work as expected with 1x1 textures
    noteyecolor col = qpixel(w->i->s, w->ox, w->oy);
    if(w->trans == transNone) col |= 0xFF000000;
    glColor4f(part(col,2)/255.0, part(col,1)/255.0, part(col,0)/255.0, part(col,3)/255.0);
    glVertex3f(orig.x, orig.y, orig.z);
    glVertex3f(orig.x+ox.x, orig.y+ox.y, orig.z+ox.z);
    glVertex3f(orig.x+ox.x+oy.x, orig.y+ox.y+oy.y, orig.z+ox.z+oy.z);
    glVertex3f(orig.x+oy.x, orig.y+oy.y, orig.z+oy.z);
    }
  else {
    if(V.shiftdown) orig = addShift(orig, oy, w);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(1,1,1,1);
    float dx = (1.0*w->gltexture->maxxn) / w->gltexture->maxxd;
    float dy = (1.0*w->gltexture->maxyn) / w->gltexture->maxyd;
    glTexCoord2f(0,   0); glVertex3f(orig.x, orig.y, orig.z);
    glTexCoord2f(dx,  0); glVertex3f(orig.x+ox.x, orig.y+ox.y, orig.z+ox.z);
    glTexCoord2f(dx, dy); glVertex3f(orig.x+ox.x+oy.x, orig.y+ox.y+oy.y, orig.z+ox.z+oy.z);
    glTexCoord2f(0,  dy); glVertex3f(orig.x+oy.x, orig.y+oy.y, orig.z+oy.z);
    }
  glEnd(); glGetError();
  return;
  }

void initFPPGL() {
  glViewport(V.x0, gfx.s->h-V.y1, V.x1-V.x0, V.y1-V.y0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double mz = 1;
  glFrustum(
    mz*(V.x0-V.xm)/V.xs, 
    mz*(V.x1-V.xm)/V.xs,
    mz*(V.ym-V.y1)/V.ys,
    mz*(V.ym-V.y0)/V.ys,
    mz, 1000);
  glScalef(V.camerazoom, V.camerazoom, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  // float f = mz*V.ys/(V.y1-V.y0);
  glRotatef(V.cameraangle, 1, 0, 0);
  glRotatef(V.cameratilt, 0, 0, 1);
  glClearDepth( 1 );
  glClearColor(0,0,0,0);
  }

/*
void closeFPPGL() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, gfx.s->w, gfx.s->h, 0, 1, -1); 
  glMatrixMode(GL_MODELVIEW); 
  glLoadIdentity();
  } */

void refreshGL() {
  SDL_GL_SwapBuffers();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  }

void screenshotGL(const char *fname) {
  SDL_Surface *s = 
    SDL_CreateRGBSurface(SDL_SWSURFACE, gfx.s->w, gfx.s->h, 32, 0xff0000, 0xff00, 0xff, 0xff000000);

  if(!s) return;

  glReadPixels(0, 0, gfx.s->w, gfx.s->h, GL_BGRA, GL_UNSIGNED_BYTE, s->pixels);
  
  for(int y=0; y<gfx.s->h; y++) if(y*2 < gfx.s->h)
  for(int x=0; x<gfx.s->w; x++)
    swap(qpixel(s, x, y), qpixel(s, x, gfx.s->h-1-y));

  SDL_SaveBMP(s, fname);
  SDL_FreeSurface(s);
  }

int getpixelGL(int x, int y) {
  int p = 0;
  glReadPixels(x, gfx.s->h-1-y, 1, 1, GL_BGRA, GL_UNSIGNED_BYTE, &p);
  return p;
  }

#endif
}
