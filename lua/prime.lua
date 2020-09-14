if not noteyedir then
  noteyedir = (os.getenv ("NOTEYEDIR") or ".").."/"
  end

if not noteyeloaded then
  threaded = false
  game_to_launch = "prime"
  dofile (noteyedir.."./lua/noteye.lua")
  return
  end

dofile (commondir.."generic-tile.lua")
global_uu = 4
global_co = 0
global_ba = 0

mapregion = {x0=0, y0=0, x1=64, y1=20}
minipos = {x=1, y=1}

setwindowtitle ("Pesky Reticulans, Improvements, Moar Everything")
defaultmode (modeTiles)

menuCtrlM = false

musicvolume (0)
volmusic = 0

lnoteyesleep = noteyesleep

function findpc ()
  mapon = is_map_on ()
  ncursor = get_pc_coords ()
  -- note to self: check for this variable added in viewmodes.lua
  minimapon = is_minimap_on ()
  end

function ismenukey (ev)
  if ev.type == evKeyDown and ev.symbol == SDLK_F3 then
    return true
  elseif ev.type == evKeyDown and ev.symbol == SDLK_LALT then
    alt_state (1)
  elseif ev.type == evKeyUp and ev.symbol == SDLK_LALT then
    alt_state (0)
    end
  end

function xminimap (C, x, y)
  local color = minimap (x, y)
  return tilefill (vgaget (color))
  end

function xtile (C, x, y)
  if C == 0 then return outofscreen end
  local M = map_contents (x, y)
  if M.usecache then return readcache (x, y) end

  local tile = tilespatial (tilefill (0), ssFloor) -- totally black
  local endloop = false
  local tl = 1
  repeat
    local x = M[string.format('%d', tl)]
    if x == nil then break end
    local y = M[string.format('%d', tl+1)]
    local sp = M[string.format('%d', tl+2)]
    local rc = M[string.format('%d', tl+3)]
    tl = tl + 4
    local basetile = tileat (x, y)
    if rc >= 1 then
      basetile = tilecol (basetile, rc, recDefault)
      end
    tile = tilemerge (tile, tilespatial (basetile, sp))

    until endloop
  writecache (x, y, tile)
  return tile
  end

-- cache

function writecache (x, y, tile)
  if not CachedTiles then
    CachedTiles = { }
    end
  local id = x + y * 64
  CachedTiles[id] = tile
  end

function readcache (x, y)
  if not CachedTiles then
    CachedTiles = { }
    end
  local id = x + y * 64
  if not CachedTiles[id] then 
    return tilefill (0)
    end
  return CachedTiles[id]
  end

-- tiles

function tileat (x, y)
  if not TilesPNG then
    --TilesPNG = loadimage (gfxdir.."primetiles.png")
    TilesPNG = loadimage ("gfx/primetiles.png")
    TilesKey = getpixel (TilesPNG, 0, 0)
    TilesTab = { }
    ffid = freeformparam (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
    end
  local id = x + y * 40
  if not TilesTab[id] then 
    -- low and high harcoded for version 2.1
    local low = 5
    local high = low + 26 + 25
    -- monsters without tiles
    if (y == 2 or (y >= low and y <= high)) and x >= 24 and x <= 38 then
      TilesTab[id] = tilecol (tileat (0, y), 0xFF000000 + vgaget (x-24), recMult)
      -- row with door stuff
      elseif y >= 97 and y <= 101 and x >= 11 and x <= 30 then
      TilesTab[id] = tilefreeform (addtile (TilesPNG, x*32, y*32, 32, 32, TilesKey), ffid)
      -- optic blast
      elseif y == 89 and x >= 24 and x <= 32 then
      TilesTab[id] = tilecol (tileat (x-23, y), 0xFFFF0000, recDefault)
      -- grenades
      elseif y == 64 and x >= 20 then
      local base, color = grenade (x)
      TilesTab[id] = tilecol (tileat (base, y), 0xFF000000 + vgaget (color), recMult)
      else
      TilesTab[id] = addtile (TilesPNG, x*32, y*32, 32, 32, TilesKey)
      end
    end
  return TilesTab[id]
  end

function tilerec (x, y)
  return rec (tileat (x, y))
  end

function rungamex(cmdline)
  P = internal(S, Font, cmdline)
  game_core(P, mainloopcyc)
  end

rungame("whatever")
