-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- functions related to viewing

-- mode constants

modeASCII = 1
modeTiles = 2
modeFPP   = 3
modeISO   = 4
modeMini  = 5
modeMed   = 6
modeBlock = 7 -- minimap only
modeTPP   = 8 -- third-person perspective

if not tppDistance then
  tppDistance = 10
  tppAngle = 60
  tppFacing = 90
  tppZoom = 2
  tppAutoFacing = true
  end

if not minimode then
  minimode = modeASCII -- by default
  minimode_default = true
  end

D = _G

D.output = Gfx

-- the default config, to be used in games by default
-- (called here, you usually don't need to call it yourself)
function dftconfig()

  -- size of the console
  if haveascii then
    D.conssize = V.scrgetsize(openconsole())
  else
    D.conssize = V(80, 25)
    end

  -- size of tiles to use in the tile mode
  D.tilesize = V(32, 32)
  
  -- which part of the process screen contains the map?
  D.map = rectXY(0, 0, 80, 19)

  -- where is the mini-map located?
  -- 0 is top left, 1 is bottom right, other values are proportional
  -- minipos = {x=1, y=1}

  -- how to place the ASCII screen if the resolution is larger than
  -- enough to fit it? 0.5 means center, 0 means top left, etc
  D.reloffset = V(0.5, 0.5)
  
  -- what area should the tiles cover?
  -- 0 = only the map area, 1 = normal screen area, 2 = extended screen area
  D.mapextent = 2
  
  -- which tile is used out of the map
  D.OutOfMap = 0
  
  -- what color to use for the border of the minimap?
  D.minimapborder = tonumber("808080", 16)
  
  -- which tile layers are used by the given game
  D.tilelayers = {0}
  
  -- parameters for the Isometric
  D.IsoParam = isoparam(32, 32, 32, 32)  
  end

dftconfig()

-- update the screen (copy from the internal buffer to the real screen)
function updscr()
  V.updaterect(D.xscreen)
  end

-- clear the internal buffer
function clrscr()
  if havegfx then
    V.fillimage(D.output, D.xscreen, vgaget(0))
    end
  end

-- override this function if you want the screen size to be
-- greater than console size times font size
-- (this function works by modifying its parameter)
function setscreensize(xscrsize)
  end

-- called to change the video mode, and calculate all the parameters
function setvideomodex()
  if havegfx then
    D.xscrsize = V.copy(D.rscrsize)
    local s = D.reqres.sfull or D.reqres.s
    if s and s.x > D.xscrsize.x then D.xscrsize.x = s.x end
    if s and s.y > D.xscrsize.y then D.xscrsize.y = s.y end
    setscreensize(D.xscrsize)
    if fscr and not fscrmode then
      D.xscrsize = V.be(findvideomode(D.xscrsize.x, D.xscrsize.y))
      end
    local flag = SDL_RESIZABLE
    if fscr then flag = flag + SDL_FULLSCREEN end
    if useGL and opengl then flag = flag + SDL_OPENGL + SDL_HWSURFACE end
    local res = setvideomodei(D.xscrsize.x, D.xscrsize.y, flag)
    if res == false then
      -- probably the font was too big
      if fallback then 
        error("Error: unable to set any graphics mode")
        end
      fallback = true
      selectfontfull(loadfont("cp437-8", 32, 8))
      fallback = false
      end
    D.xscreen = rectS(D.xscrsize)
  
    clrscr()
  elseif hadgfx then
    closevideomode()
    end
  
  if haveascii then
    mainscreen = openconsole()
  elseif hadascii then
    delete(mainscreen)
    mainscreen = NIL
    end
    
  hadgfx = havegfx
  hadascii = haveascii
  end

-- as before, but also prepare the console (set size etc)

function setfontsize()
  D.fontsize = 
    D.reqres.sfull and V.floor(D.reqres.sfull/rconssize)
    or D.reqres.f and V.ceil(D.reqres.f*FontData.size)
    or D.reqres.s and V.floor(D.reqres.s/rconssize)
    or V(8,16)
  D.guifontsize = D.guiscaling and D.guiscaling * FontData.size or 
    V.min(FontData.size, D.fontsize)
  end

function prepareconsole()

  if fscr and fscrmode then
    D.reqres.sfull = V.be(origvideomode())
  else
    D.reqres.sfull = nil
    end
    
  D.rconssize = (D.conssize + V(0, D.msgreserve))
  
  setfontsize()
  
  D.scrsize = D.fontsize * D.conssize

  D.rscrsize = D.fontsize * D.rconssize
  
  setvideomodex()
  
  if havegfx then
  
    D.screen = rectTS((D.xscreen.size-D.rscrsize) * D.reloffset, D.scrsize)
    D.rscreen = rectTS((D.xscreen.size-D.rscrsize) * D.reloffset, D.rscrsize)
    end
  
  S = V.renewscreen(S, D.conssize)
  IMG  = V.renewscreen(IMG, D.conssize)
  IMGX = V.renewscreen(IMGX, D.conssize) -- auxiliary for shading
  
  MAP = V.renewscreen(MAP, D.map.size)
  
  Tiles = V.renewscreen(Tiles, D.map.size)
  scrset(Tiles, 999, 999, OutOfMap)  
  
  D.mmapsize = D.map.size + V(2,2)
  
  Minimap = V.renewscreen(Minimap, D.mmapsize)
  V.scrfill(Minimap, rectTB(V(0,0), D.mmapsize), tilefill(vgaget(0)))

  oplayerpos = nil
  mapcenter = playerpos or V(1,1)
  mapon = false
  lastcmp = nil -- forget the calculated minimap position
  end

-- == extra stuff ==

-- also draw the messages

function drawmessage()
  while table.getn(messages)>0 and (messages[1].at < getticks() or table.getn(messages) > 5) do
    table.remove(messages, 1)
    end
  
  local yy = table.getn(messages)
  
  if yy>0 then
    if havegfx then
      drawtile(1, tilefill(vgaget(1)), 0, D.xscreen.size.y-D.fontsize.y*yy, D.xscreen.size.x, D.fontsize.y*yy)
      Msgline = renewscreen(Msgline, D.conssize.x, 1)
      for y=1,yy do
        scrfill(Msgline, 0, 0, D.conssize.x, 1, 0)
        scrwrite(Msgline, 0, 0, messages[y].text, Font, vgaget(14))
        drawscreen(1, Msgline, 0, D.xscreen.size.y-D.fontsize.y*(yy-y+1), D.fontsize.x, D.fontsize.y)
        end
      end
    if haveascii then
      for y=1,yy do
        scrwrite(mainscreen, 0, 24+y-yy, messages[y].text, Font, vgaget(14))
        end
      end
    end
  end

-- draw the MiniMap

D.minimapsize = V(3,3)

function getpopuplocation(margin, size, last)
  local top = D.rmaparea.top + margin
  local bot = D.rmaparea.bot - margin - size

  if (mode == modeMed or mode == modeASCII or mode == modeMini) then
    return V.cmpcheck(D.mapcenter, D.map.ctr, bot, top)
  elseif mode == modeTiles or mode == modeISO then
    local cmp = V.cmpcheck(D.playerpos, D.mapcenter, bot, top)
    if last and D.playerpos.x == D.mapcenter.x then
      cmp.x = last.x
      end
    if last and D.playerpos.y == D.mapcenter.y then
      cmp.y = last.y
      end
    return cmp
  else 
    return last or V(bot.x, top.y)
    end
  end

function setupMiniMap()

  -- do not move the minimap during ctrl+zooming, look-at-location, or move-to-location
  if D.cmini and ctrlzoomingactive then return end
  if D.cmini and minimovable == 3 then return end
  if D.cmini and minimovable == 4 then return end

  local ms = D.minimapsize
  local size = D.minimapsize * D.mmapsize

  local cmp
  
  if D.minimoved and D.cmini and D.cmini.top and not D.cmini.bot then
    cmp = D.cmini.top
  elseif D.minimoved and D.cmini then
    return
    -- the user has moved the minimap, so simply trust them
  else 
    -- the configuration has a special place for the minimap
    cmp = getminipos(size)
    if not cmp then
      cmp = getpopuplocation(0, size, lastcmp)
      lastcmp = cmp      
      end
    end

  D.cmini = rectTS(cmp, size)
  end

function getminipos(size)
  if (D.minipos and D.minipos.y >= 0.99) or msgreserve * fontsize.y >= size.y-8 then 
    return D.rscreen.bot - size
    end
  if D.minipos then 
    return D.screen.top + (D.screen.size-size) * D.minipos
    end
  end

ZoomCache = nil

function miniZoom()

  if ZoomCache then
    zcs = V.imggetsize(ZoomCache)
    if zcs ~= V.floor(D.cmini.size) then
      delete(ZoomCacheTile)
      delete(ZoomCache)
      ZoomCache = nil
      end
    end
  
  if not ZoomCache then
    ZoomCache = V.newimage(D.cmini.size, 0)
    ZoomCacheTile = V.addtile(ZoomCache, rectS(D.cmini.size), -1)
    end
  
  local copies = {}  
  function savecopy(name, val)
    copies[name] = D[name]
    D[name] = val
    end
  
  local nmc = true
  
  if mousepos and mode ~= modeFPP and mode ~= modeTPP then
    local u = (mousepos - D.screen.top) / D.fontsize
    if inrect(u, D.map) then
      local upro = pixeltoprocess(mousepos)
      savecopy("mapcenter",  upro - V(0.5, 0.5) - D.map.top)
      nmc = false
      end
    end
  if nmc and playerpos then
    savecopy("mapcenter", animated_playerpos())
    end

  if mode == minimode then 
    savecopy("tilesize", D.tilesize * 2)
    end
  
  savecopy("output", ZoomCache)
  savecopy("mode", minimode)
  savecopy("maparea",  maparea)
  savecopy("tilectr",  tilectr)
  
  drawmap[mode]()

  for k,v in pairs(copies) do D[k] = v end
  
  V.drawtile(1, ZoomCacheTile, D.cmini)
  return true
  end

function animated_playerpos()
  xtile(scrget(IMG, playerpos.x, playerpos.y), playerpos.x, playerpos.y)
  local masp = moveanimations["player"]
  if masp then return masp.animxy or playerpos end
  return playerpos
  end

function miniFPP()

  local copies = {}  
  function savecopy(name, val)
    copies[name] = D[name]
    D[name] = val
    end
  savecopy("maparea",  maparea)
  savecopy("tilectr",  tilectr)
  
  V.scrcopy(MAP, 0, Tiles, 0, D.map.size, xtile)
  
  setfppvar()
  fppvar.vx0 = fppvar.vx0 + D.cmini.top.x
  fppvar.vy0 = fppvar.vy0 + D.cmini.top.y
  fppvar.vx1 = fppvar.vx1 + D.cmini.top.x
  fppvar.vy1 = fppvar.vy1 + D.cmini.top.y
  fppvar.vxm = fppvar.vxm + D.cmini.top.x
  fppvar.vym = fppvar.vym + D.cmini.top.y
  
  local fd = fppFacing or facedir * 45
  -- rear mirror!
  if mode == modeFPP then fd = fd + 180 end
  
  if mousepos then
    local u = (mousepos - D.screen.top) / D.fontsize
    if inrect(u, D.map) then
      local upr = pixeltoprocess(mousepos)
      fd = V.atan2(upr-playerpos-D.map.top-V(.5,.5)) * 180 / math.pi - 90
      end
    end
  
  mc = mapcenter
  if playerpos then mapcenter = animated_playerpos() end
  
  local m = mode
  mode = modeFPP
  
  fppbackground(fppvar, fd)
  V.scrcopy(MAP, 0, Tiles, 0, D.map.size, xtile)
  fpp(fppvar, D.mapcenter.x, D.mapcenter.y, fd, Tiles)
  
  D.mapcenter = mc
  
  mode = m
  return true
  end

function miniReal()
  if D.minimapsize <= V(6, 8) then
    setminifont("tinyfont", 32, 8, fontadj.simpleA)
  elseif D.minimapsize.y <= 9 then
    setminifont("cp437-8", 32, 8, fontadj.simple)
  elseif D.minimapsize.y <= 14 then
    setminifont("cp437-14", 32, 8, fontadj.simple)
  else
    setminifont("cp437-16", 32, 8, fontadj.simple)
    end
  V.scrfill(Minimap, rectS(D.mmapsize), tilefill(minimapborder))
  V.scrcopy(MAP, 0, Minimap, V(1,1), D.map.size, xminimap)
  V.drawscreen(1, Minimap, D.cmini.top, D.minimapsize)
  
  if mode == modeTiles then
    local rtop = ((maparea.top - tilectr) / tilesize + mapcenter) + 1.5
    rtop = rtop * minimapsize + cmini.top
    local minirect = rectTS(rtop, maparea.size / tilesize * D.minimapsize)
    minirect.top = V.max(minirect.top, D.cmini.top)
    minirect.bot = V.min(minirect.bot, D.cmini.bot)
    minirect.size = minirect.bot - minirect.top
    drawframe(1, minirect)
    end

  return true
  end

minimap_frame = tonumber("F808080", 16)

function drawframe(out, R)
  local z = tileshade(minimap_frame)
  V.drawtile(1, z, rectXY(R.top.x, R.top.y, R.top.x+1, R.bot.y))
  V.drawtile(1, z, rectXY(R.top.x, R.top.y, R.bot.x, R.top.y+1))
  V.drawtile(1, z, rectXY(R.top.x, R.bot.y-1, R.bot.x, R.bot.y))
  V.drawtile(1, z, rectXY(R.bot.x-1, R.top.y, R.bot.x, R.bot.y))
  end

function minimapoff()
  -- is the minimap off?
  return 
    (not mapon) or
    -- externally disabled by PRIME
    (not minimapon) or
    -- yes, it is off
    minimode == 0 or 
    -- minimode is the same as mode, so ignore (except modeTiles, which means zoom,
    -- and modeFPP, which means rear mirror)
    (minimode == mode and minimode ~= modeTiles and minimode ~= modeFPP) or 
    -- modeBlock is useless in both modeASCII and modeMini
    (minimode == modeBlock and (mode == modeASCII or mode == modeMini))
  end

drawmini = {}
drawmini[modeFPP] = miniFPP
drawmini[modeISO] = miniZoom
drawmini[modeTiles] = miniZoom
drawmini[modeBlock] = miniReal
drawmini[modeASCII] = miniReal

function setminifont(fname, a,b,c)
  loadfont(fname, a,b,c)
  MiniFont = Fonts[fname].font
  end

function drawMiniMap()
  if minimapoff() then 
     return false
     end
  inmini = true
  setupMiniMap()
  local d = drawmini[minimode] and drawmini[minimode]()
  inmini = false
  return d
  end

-- == various graphical modes ==

drawmap = {}

drawmap[modeMini] = function()
  setmaparea()
  if mapon then
    MiniTiles = V.renewscreen(MiniTiles, D.map.size)
    V.scrcopy(MAP, 0, MiniTiles, 0, D.map.size, xtile)
    for k,v in pairs(tilelayers) do
      V.drawscreenx(D.output, MiniTiles, D.screen.top + D.fontsize * D.map.top, D.fontsize, getlayerX(v))
      end
    return false
    end
  end

-- tile display --

function setmaparea()
  if inmini then
    D.maparea = rectS(D.cmini.size)
    D.rmaparea = nil
    return
    end
  D.rmaparea = D.map * D.fontsize + D.screen.top
  if mapextent == 2 then
    D.maparea = D.xscreen
  elseif mapextent == 1 then
    D.maparea = D.screen
  else
    D.maparea = D.rmaparea
    end
  end

-- calculate the place where to put the center in the tile mode
function settilectr()
  tilectr = (D.rmaparea or D.maparea).ctr
  end

function adjusttileext()
  end

function recentermap()
  if (not D.rmaparea) or ctrlzoomingactive then
    return 
    end
  
  local tm = rectTS(D.tilectr - (D.mapcenter + .5) * D.tilesize, D.tilesize * D.map.size)
  
  for k in pairs(V.xy) do 
    if tm.size[k] < D.rmaparea.size[k] then 
      mapcenter[k] = (D.tilectr[k] - D.rmaparea.ctr[k]) / D.tilesize[k] - .5 + D.map.size[k] / 2
    elseif tm.top[k] > D.rmaparea.top[k] then
      mapcenter[k] = (D.tilectr[k] - D.rmaparea.top[k]) / D.tilesize[k] - .5
    elseif tm.bot[k] < D.rmaparea.bot[k] then
      mapcenter[k] = (D.tilectr[k] - D.rmaparea.bot[k]) / D.tilesize[k] - .5 + D.map.size[k]
      end
    end
  end

drawmap[modeTiles] = function()  

  setmaparea()
  settilectr()
  recentermap()
  
  local mcf = V.floor(D.mapcenter)
  tcs = D.tilectr - (D.mapcenter -mcf) * tilesize

  local tetop = V.floor((D.maparea.top-tcs)/D.tilesize)
  local tebot = V.ceil((D.maparea.bot-tcs)/D.tilesize)+1
  D.tileext = rectTB( tetop, tebot)

  adjusttileext()
  TileCut = V.renewscreen(TileCut, D.tileext.size)
  
  scrset(MAP, 999, 999, 0)
  V.scrcopy(MAP, mcf+D.tileext.top, TileCut, 0, D.tileext.size, xtile)
  
  local tcs2 = tcs + (D.tileext.top - 0.5) * D.tilesize
  local tcs3 = tcs2 + D.tileext.size * D.tilesize
  
  for k,v in pairs(tilelayers) do
    V.drawscreenx(D.output, TileCut, tcs2, D.tilesize, getlayerX(v))
    end
  return getAxCommon()
  end

-- med display --

drawmap[modeMed] = function()

  setmaparea()
  if not mapon then
    return false
    end
  
  local medsize = math.floor((D.map.size.x+1)/2)
  local tx = D.mapcenter.x - math.floor(medsize/2)
  if tx < 0 then tx = 0
  elseif tx + medsize > D.map.size.x then tx = D.map.size.x - medsize
  end
  
  MedTiles = renewscreen(MedTiles, medsize, D.map.size.y)
  scrcopy(MAP, tx, 0, MedTiles, 0, 0, medsize, D.map.size.y, xtile)

  for k,v in pairs(tilelayers) do
    V.drawscreenx(1, MedTiles, screen.top + map.top * D.fontsize, 
      D.fontsize * V(2,1), getlayerX(v))
    end

  D.playerat = V(
    (((map.top.x+mapcenter.x+0.5) - (map.top.x+tx)) * 2 + map.top.x) * D.fontsize.x,
    (mapcenter.y+0.5) * D.fontsize.y
    )
  
  return false
  end

function setfppvar()
  setmaparea()
  fppvar = {
    vx0 = maparea.top.x, vy0 = maparea.top.y,
    vx1 = maparea.bot.x, vy1 = maparea.bot.y,
    vxm = maparea.ctr.x, vym = maparea.ctr.y,
    vys = maparea.size.y/2,
    vxs = maparea.size.y/2,
    vimg = D.output,
    dy = 2
    }
  end

drawmap[modeFPP] = function()
  setfppvar()
  fppbackground(fppvar, facedir * 45)
  
  if fpplookat then
    fppFacing = V.atan2(fpplookat-mapcenter) * 180 / math.pi - 90
    facedir = math.floor(fppFacing / 45 + 0.5) % 8
    if facedir < 0 then facedir = facedir + 8 end
  else
    fppFacing = fppFacing or (facedir * 45)
    end
  
  simFacing = be360(fppFacing)
  tppAngleRound = nil
  V.scrcopy(MAP, 0, Tiles, 0, D.map.size, xtile)
  fpp(fppvar, mapcenter.x, mapcenter.y, fppFacing, Tiles)
  return getAxCommon()
  end

drawmap[modeTPP] = function()

  setfppvar()

  tppFacing = tppFacing or (facedir * 45)
  simFacing = be360(tppFacing)
  
  if not tppAutoFacing then
    facedir = math.floor(tppFacing / 45 + 0.5) % 8
    if facedir < 0 then facedir = facedir + 8 end
    end
  
  tppAngleRound = math.floor(tppAngle) % 360
  if tppAngleRound < 0 then tppAngleRound = tppAngleRound + 360 end
  
  local tppAngleRad = tppAngle * math.pi / 180
  local angle = tppFacing * math.pi / 180
  zsi = tppDistance * math.sin(tppAngleRad)

  fppvar.dz = (fppvar.dz or 0) + zsi * 32
  fppvar.dy = 0
  fppvar.dx = 0
  fppvar.cameraangle = tppAngle
  fppvar.camerazoom  = tppZoom

  local lastmc = mapcenterTPP
  
  mapcenterTPP = mapcenter - 
    V(math.cos(angle) * tppDistance * math.cos(tppAngleRad) / (fppvar.xz or 1),
     -math.sin(angle) * tppDistance * math.cos(tppAngleRad))
  
  fppbackground(fppvar, facedir * 45)
  V.scrcopy(MAP, 0, Tiles, 0, D.map.size, xtile)  
  
  fpp(fppvar, mapcenterTPP.x, mapcenterTPP.y, tppFacing, Tiles)
  
  return getAxCommon()
  end

function fppbackground(v)
  fillimage(D.output, v.vx0, v.vy0, v.vx1-v.vx0, v.vy1-v.vy0, vgaget(0))
  end
  
-- iso display --

drawmap[modeISO] = function()
  V.scrcopy(MAP, 0, Tiles, 0, D.map.size, xtile)
  scrset(Tiles, 999, 999, OutOfMap)
  isi = V.be(isosizes(IsoParam))
  
  setmaparea()
  settilectr()
  local mcf = V.floor(D.mapcenter)
  
  local dxy = D.mapcenter - mcf
  
  tcs = tilectr - V(isi.x/2 + isi.floor*(dxy.x-dxy.y), isi.y/2 + isi.floor*(dxy.x+dxy.y)/2)

  local rect = (D.maparea - tcs) / V(2*isi.floor, isi.floor)
  
  local dminx = math.floor(rect.top.x + rect.top.y) - 2
  local dminy = math.floor(rect.top.y - rect.bot.x) - 2
  local dmaxx = math.ceil(rect.bot.x + rect.bot.y) + 2
  local dmaxy = math.ceil(rect.bot.y - rect.top.x) + 2
  
  for dx=dminx,dmaxx do
  for dy=dminy,dmaxy do
    local dr = V(tcs.x+isi.floor*(dx-dy), tcs.y+isi.floor*(dx+dy)/2)
    if dr > D.maparea.top - isi and dr < D.maparea.bot then
      local sg = scrget(Tiles, mcf.x+dx, mcf.y+dy)
      local ip = isoproject(sg, IsoParam)
      V.drawtile(D.output, ip, rectTS(dr, isi))
      end
    end end
  return getAxCommon()
  end

function drawMap()
  return drawmap[mode] and drawmap[mode]()
  end

-- if the game has a special background, then draw it and return the ASCII transformer
function drawBackgroundAndMap()
  return drawMap()
  end

-- drawing the ASCII screen

D.shifts = {V(-1,0), V(0,-1), V(1,0), V(0,1)}

function drawAscii(ax)
  if not ax then
    V.drawscreenx(D.output, IMG, D.screen.top, D.fontsize, eq)
  else
    if ax[1] then V.drawscreenx(D.output, IMG, D.screen.top, D.fontsize, ax[1]) end

    if ax[2] then
      V.scrcopy(IMG, 0, IMGX, 0, D.conssize, ax[2])
      for k,v in pairs(D.shifts) do
        V.drawscreenx(D.output, IMGX, D.screen.top+v, D.fontsize, eq)
        end
      end

    if ax[3] then
      V.drawscreenx(D.output, IMG, D.screen.top, D.fontsize, ax[3])
      end
    end
  end

axTransparent = {nil, cellblack, celltransorshade}
axSemitransparent = {cellbackshade75, cellblack, celltrans}

axInMap = axTransparent

function getAxCommon()
  -- with mapextent == 0, we can simply draw the ASCII screen;
  -- otherwise, use axMapOn or axMapOff
  return mapextent > 0 and (mapon and axInMap or axSemitransparent)
  end

function framecount()
  if fps_start then
    frametime = (getticks() - fps_start) / 1000
    frames = frames + 1
    end
  end

function drawdisplay()
  scrollmap()
  framecount()
  clrscr()  
  local b = drawBackgroundAndMap()
  drawAscii(b)  
  drawMiniMap()
  drawmessageTemp()
  end

function tmpmessage(str)
  message = str
  table.insert(messages, {at = getticks() + 10, text = str, tmp = true})
  end

function drawmessageTemp()

  if conshelp and mousepos then
    local c = V.floor(mousepos / D.fontsize)
    local sg = V.scrget(S, c)
    local av,co,ba = gavcoba(sg)
    thelp = "at "..tostring(c).." ("..tostring(mousepos)..") char = "..gch(sg).."/"..av.." color = "..ashex(co).." back = "..ashex(ba)
    tmpmessage(thelp)
    end

  if modeconfigactive and mode ~= modeTPP then
    modeconfigactive = false
    end
  
  if modeconfigactive then
    tmpmessage("\vf\vearrows/keypad\vp configure, \vep\vp presets, \veESC\vp play")
    tmpmessage(
      "a="..tppAngle.." d="..tppDistance.." z="..tppZoom..
      " f="..(tppAutoFacing and "player" or tppFacing)
      )
    end

  drawmessage()
  
  while table.getn(messages) > 0 and messages[table.getn(messages)].tmp do 
    table.remove(messages, table.getn(messages))
    end
  end
  
function drawandrefresh()
  profstart("drawandrefresh")
  if havegfx then
    drawdisplay()
    V.updaterect(D.xscreen)
    end
  if haveascii then
    V.scrcopy(IMG, 0, mainscreen, 0, D.conssize, eq)
    drawmessage()
    V.refreshconsole(pcursor, pcursor.size or 1)
    end
  profend("drawandrefresh")
  end

-- convert a pixel to process coordinates (modify the event)
function pixeltoproc(ev)
  local ptp = pixeltoprocess(ev)
  if not ptp then return end
  local evb = V.floor(ptp)
  ev.x = evb.x
  ev.y = evb.y
  return ev
  end

function minipixeltoprocess(ev)
  if minimode == modeBlock or minimode == modeASCII then
--  print("top = "..tostring(D.map.top))
--  print("ev = "..tostring(ev))
--  print("cmini = "..tostring())
    return (ev - cmini.top) / minimapsize - 1.5
    end
  end

-- convert a pixel to process coordinates
function pixeltoprocess(ev)

  if mode == modeASCII or mode == modeMini then
    return (ev - D.screen.top) / D.fontsize
  elseif mode == modeMed and D.playerat then
    local x = (D.maparea.top + D.mapcenter + (ev - D.playerat) / D.fontsize/2).x + 0.5
    local y = ((ev - D.screen.top) / D.fontsize).y
    return V(x,y)
  elseif mode == modeTiles and D.mapcenter and D.tilectr then
    return D.map.top + D.mapcenter + (ev - D.tilectr) / D.tilesize + 0.5
  elseif mode == modeISO then
    local isi = isosizes(IsoParam)
    local rx = ev.x - tilectr.x
    local ry = ev.y - tilectr.y
    ry = ry / isi.floor
    rx = rx / 2 / isi.floor
    return map.top + D.mapcenter + V(rx+ry, ry-rx) + 0.5
    end
  -- note: does not return anything in modeFPP!
  end

-- convert a pixel to a direction (return integer)

function pixeltodir(ev)
  if mode == modeFPP or mode == modeTPP then
    return vectodir(V(ev.x - fppvar.vxm, ev.y - fppvar.vym), 32)
  elseif playerpos then
    local e = pixeltoprocess(ev)
    if not e then return end
    local dir = vectodir(e-map.top-playerpos-0.5, 0.8)
    return dir
    end
  end

-- message subsystem

messages = {}

-- add a message, only locally
function localmessage(str)
  message = str
  table.insert(messages, {at = getticks() + 10000, text = str})
  end

-- == graphical modes menu ==

modesmenu = {}

function addgraphmode(key, text, m, menu)
  addtomenu(menu or modesmenu,
    key,
    writechoicef(function() return ((mode == m) and "\ve("..string.char(15)..")\vp " or "\v7( )\vp ")..text end),
    function () 
      pickmode(m) 
      modeconfigactive = true
      end
    )
  end

function pickmode(m)
  cfgscripts.mode = function(file)
    file:write("pickmode("..m..")\n")
    end
  mode = m modepower = 10 
  autoGL()
  end

function autoGL()
  if (mode == modeFPP or mode == modeTPP) and not useGL then resetGL = true pickgl(true) end
  if mode ~= modeFPP and mode ~= modeTPP and resetGL then resetGL = false pickgl(false) end
  end

addgraphmode("p", "plain ASCII only (look at the map)", modeASCII)
addgraphmode("t", "tile mode", modeTiles)
addgraphmode("f", "first person perspective", modeFPP)
addgraphmode("i", "isometric projection", modeISO)
addgraphmode("j", "mini-tile mode", modeMini)
addgraphmode("d", "double mini-tiles", modeMed)
addgraphmode("3", "third person perspective", modeTPP)

addtomenu(mainmenu, "m", writechoice("select NotEye's graphical mode and mini-map"),
  function()
    menuexecute(modesmenu, glheader)
    end
  )

function pickfull(f, f2)
  -- backward compatibility:
  cfgscripts.full = function(file)
    file:write("pickfull("..booltf(f)..","..booltf(f2)..")\n")
    end
  fscr = f
  fscrmode = f2
  prepareconsole()
  end

if opengl then addtomenu(modesmenu, "o", writechoice("switch OpenGL usage"),
  function() 
    pickgl(not useGL)
    if useGL then localmessage("OpenGL activated")
    else localmessage("OpenGL deactivated") end
    end
  ) end

function pickgl(b)
  cfgscripts.gl = function(file)
    file:write("pickgl("..booltf(b)..")\n")
    end
  useGL = b
  setvideomodex()
  end

function glheader()
  menuy = 8
  scrwrite(IMG, 1, 1, "Select your graphical mode:", Font, vgaget(10))

  scrwrite(IMG, 1, 3, "OpenGL is currently \ve"..boolonoff(useGL), Font, vgaget(7))

  scrwrite(IMG, 1, 5, "Hint: OpenGL makes NotEye work much faster in the FPP mode", Font, vgaget(7))
  scrwrite(IMG, 1, 6, "(and possibly in other modes), but is less robust", Font, vgaget(7))
  end

function pickminimap(v)
  cfgscripts.minimap = function(file)
    file:write("pickminimap("..v..")\n")
    end
  minimode = v
  end

mmodes = {}
mmodes[0] = "mini-map off"
mmodes[modeASCII] = "mini-map mode: ASCII"
mmodes[modeBlock] = "mini-map mode: blocks"
mmodes[modeTiles] = "mini-map mode: tiles"
mmodes[modeFPP]   = "mini-map mode: FPP"
mmodes[modeISO]   = "mini-map mode: Isometric"

addtomenu(modesmenu, "m", writechoicef(
  function() 
    return mmodes[minimode]
    end ),
  function()
    local b = true
    local nv = 0
    for k,v in pairs(mmodes) do
      if b then nv = k b = false end
      if k == minimode then b = true end
      end
    pickminimap(nv)
    return true
    end
  )

function pickminimapsize(x, y)
  cfgscripts.minimapsize = function(file)
    file:write("pickminimapsize("..x..","..y..")\n")
    end
  minimapsize.x = x
  minimapsize.y = y
  D.cmini = nil
  if minimovable > 1 then minimovable = 1 end
  end

asciirecom = {"for the ASCII minimap, the recommended size is 3x5",
  "",
  "minimap commands:",
  "\veleft-click\vp: center the big map at the given location",
  "\veright-click\vp: move the minimap, or bring it back to the default location",
  "(edit the minimap size to reset it to default if it went off screen)",
  "\vemiddle-click\vp: zoom (i.e., show it full screen)",
  "\vemousewheel down/up\vp: move to/from this location",
  "\veShift+mousewheel down/up\vp: change the minimap height",
  "\veCtrl+mousewheel down/up\vp: change the minimap width",
  "\veShift+Ctrl+mousewheel down/up\vp: change both the minimap width and height"
  }

addtomenu(modesmenu, "s", writechoicef(
  function() 
    return "mini-map size: \ve"..xxy(minimapsize).."\vp (also help about the minimap)"
    end ),
  function()
    minimapsize.x = asknum(minimapsize.x, "minimap width", minimapsize.x, asciirecom)
    minimapsize.y = asknum(minimapsize.y, "minimap height", minimapsize.y, asciirecom)
    pickminimapsize(minimapsize.x, minimapsize.y)
    end
  )

zoomnote = { "Note that you can usually zoom in the game with \veCtrl+MouseWheel\vp" }

addtomenu(modesmenu, "z", writechoicef(
  function() return "current tile size: \ve"..xxy(tilesize).."\vp (affects Tile and Iso modes)" end),
  function()  
    local tsx = asknum(tilesize.x, "new tile width", tilesize.x, zoomnote)
    if tsx ~= tilesize.x and tsx > 0 and tsx < 300 then
      zoomtiles(tsx / tilesize.x, playerpos)
      clearmoveanimations()
      end
    end
  )

function playermoved()
  scrolltoplayer = true
  scrollmanual = false
  end

function mr0(v) if v>0 then return math.floor(v) else return -math.floor(-v) end end

function scrollmaphorizontal(v)
  scrolltoplayer = false
  if mode == modeTiles then
    mapcenter.x = mapcenter.x - v
  elseif mode == modeISO then
    mapcenter.x = mapcenter.x - v
    mapcenter.y = mapcenter.y + v
    end
  end

function scrollmapvertical(v)
  scrolltoplayer = false
  if mode == modeTiles then
    mapcenter.y = mapcenter.y - v
  elseif mode == modeISO then
    mapcenter.x = mapcenter.x - v
    mapcenter.y = mapcenter.y - v
    end
  end

function scrollf(cx, tx, sval)
  if cx < tx then 
    cx = cx + sval * (1 + tx - cx)
    if cx > tx then return tx end
    end
  if cx > tx then 
    cx = cx - sval * (1 + cx - tx)
    if cx < tx then return tx end
    end
  return cx
  end

function scrollfr(cur, tgt, sval)
  if not cur then return tgt end
  cur = cur % 360
  if tgt > cur+540 then tgt = tgt - 1080 end
  if tgt < cur-540 then tgt = tgt + 1080 end
  if tgt > cur+180 then tgt = tgt - 360 end
  if tgt < cur-180 then tgt = tgt + 360 end
  return scrollf(cur, tgt, sval)
  end

function scrollfv(cur, tgt, sval)
  return V(scrollf(cur.x, tgt.x, sval), scrollf(cur.y, tgt.y, sval))
  end

spdAutocenter = 100
spdMoveanim = 100
spdScrolling = 100
spdRotateanim = 100
scrollwidth = 20

scrollTick = getticks()

function getscrollarea()
  return D.rmaparea
  end

function scrollmap()
  if ctrlzoomingactive or not mapon then 
    scrollLastTick = nil
    return
    end
  scrollTick = getticks()
  local scrolldelta = scrollLastTick and (scrollTick - scrollLastTick) or 0

  local rotateval = scrolldelta * spdRotateanim / 40000
  
  fppFacing = scrollfr(fppFacing, facedir * 45, rotateval)
  if tppAutoFacing then
    tppFacing = scrollfr(tppFacing, facedir * 45, rotateval)
    end
  
  if modeconfigactive then applymodeconfig(scrolldelta) end
  
  local ascrollval = scrolldelta * spdAutocenter / 40000
  local mscrollval = scrolldelta * spdScrolling / 10000
  if scrolltoplayer then
    mapcenter = scrollfv(mapcenter, playerpos, ascrollval)
    end
  scrollLastTick = scrollTick
  setmaparea()
  local scrollarea = getscrollarea()
  if mousepos and mouse_inside and scrollarea and mode ~= modeFPP and mode ~= modeTPP then
    if inrect(mousepos, scrollarea) and 
      scrollmanual and 
      mscrollval > 0 and 
      minimovable < 2 and
      not (cmini and cmini.bot and inrect(mousepos, D.cmini)) then
      local mx = mousepos.x
      local my = mousepos.y
      local sw = scrollarea.size * scrollwidth / 100
      local top = scrollarea.top + sw
      local bot = scrollarea.bot - sw
      if mx < top.x then
        scrollmaphorizontal(mscrollval * (top.x - mx) / sw.x)
        end
      if mx > bot.x then
        scrollmaphorizontal(mscrollval * (bot.x - mx) / sw.x)
        end
      if my < top.y then
        scrollmapvertical(mscrollval * (top.y - my) / sw.y)
        end
      if my > bot.y then
        scrollmapvertical(mscrollval * (bot.y - my) / sw.y)
        end
      end
    end
  end

function picktilesize(tx,ty, i1,i2,i3,i4)
  cfgscripts.zoom = function(file)
    file:write("picktilesize("..tx..","..ty..","..i1..","..i2..","..i3..","..i4..")\n")
    end
  tilesize = V(tx,ty)
  IsoParam = isoparam(i1,i2,i3,i4)
  end

function zoomtiles(factor, ev)
  if mode == modeTPP then
    picknumber("tppDistance", tppDistance * math.pow(factor, 0.25))
    return
    end
  setmaparea()
  settilectr()
  evc = pixeltoprocess(ev)
  isi = isosizes(IsoParam)
  picktilesize(
    tilesize.x * factor, tilesize.y * factor,
    isi.floor*factor,isi.wall*factor,isi.icon*factor,isi.iconh*factor
    )
  settilectr()
  evc2 = pixeltoprocess(ev)
  mapcenter = mapcenter + evc - evc2
  end

function handlemodeconfig(ev)
  if mode ~= modeTPP then
    modeconfigactive = false
    end
  if ev.type == evKeyDown and ev.symbol == SDLK_ESCAPE then
    modeconfigactive = false
    end
  if ev.type == evKeyDown and ev.symbol == SDLK_KP5 then
    tppAutoFacing = true
    end
  if ev.type == evKeyDown and ev.chr == av("a") then
    local a = asknum(tppAngle, "new TPP camera angle", tppAngle)
    if a then picknumber("tppAngle", a) end
    end
  if ev.type == evKeyDown and ev.chr == av("d") then
    local a = asknum(tppDistance, "new TPP distance", tppDistance)
    if a then picknumber("tppDistance", a) end
    end
  if ev.type == evKeyDown and ev.chr == av("z") then
    local a = asknum(tppZoom, "new TPP zoom", tppZoom)
    if a then picknumber("tppZoom", a) end
    end
  if ev.type == evKeyDown and ev.chr == av("f") then
    local a = asknum(tppFacing, "new TPP camera facing direction", nil, 
      {"press ESC to make the camera always face the same direction as the player"})
    if a then picknumber("tppFacing", a) pickbool("tppAutoFacing", false) 
    else pickbool("tppAutoFacing", true)
    end
    end
  if ev.type == evKeyDown and ev.chr == av("p") then
    menuexecute(tppPresets, dfthdr("Third Person Perspective: preset parameters"))
    end
  end

function applymodeconfig(delta)
  delta = delta / 1000
  if getkeystate(SDLK_LSHIFT) > 0 then delta = delta * 5 end
  if getkeystate(SDLK_RSHIFT) > 0 then delta = delta * 5 end
  if getkeystate(SDLK_LCTRL) > 0 then delta = delta / 5 end
  if getkeystate(SDLK_RCTRL) > 0 then delta = delta / 5 end
  tppDistance = 
    tppDistance + delta * (getkeystate(SDLK_KP2) + getkeystate(SDLK_DOWN) - getkeystate(SDLK_KP8) - getkeystate(SDLK_UP))
  local d = (getkeystate(SDLK_KP6) + getkeystate(SDLK_RIGHT) - getkeystate(SDLK_KP4) - getkeystate(SDLK_LEFT))
  if d ~= 0 then
    tppAutoFacing = false
    tppFacing = tppFacing + 10 * delta * d
    end
  tppAngle = 
    tppAngle + 10 * delta * (getkeystate(SDLK_KP9) + getkeystate(SDLK_PAGEUP) - getkeystate(SDLK_KP3) - getkeystate(SDLK_PAGEDOWN))
  tppZoom =
    tppZoom * math.exp(delta * (getkeystate(SDLK_KP_PLUS) + getkeystate(SDLK_PLUS) - getkeystate(SDLK_KP_MINUS) - getkeystate(SDLK_MINUS)))
  end

tppPresets = {}

function pickTPP(angle, distance, zoom, facing)
  picknumber("tppAngle", angle)
  picknumber("tppDistance", distance)
  picknumber("tppZoom", zoom)
  pickbool("tppAutoFacing", not facing)
  if facing then picknumber("tppFacing", facing) 
  else 
    if gamefourdir and (facedir % 2 == 1) then facedir = facedir+1 end
    if gamehexdir and (facedir % 4 == 2) then facedir = facedir+2 end
    tppFacing = facedir * 45
    end
  end

addtomenu(tppPresets, "d", writechoice("default"),
  function() pickTPP(60, 10, 2) end
  )

addtomenu(tppPresets, "t", writechoice("top-down"),
  function() pickTPP(90, 10, 2) end
  )

addtomenu(tppPresets, "i", writechoice("isometric-like"),
  function() pickTPP(30, 20, 4, 135) end
  )

addtomenu(tppPresets, "b", writechoice("view from behind"),
  function() pickTPP(0, 1.5, 1) end
  )

