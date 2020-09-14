-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- This implements the DawnLike tileset

dawn = {}

-- getdawntile: get the specific tile from the tileset,
-- for example getdawntile("Items/Amulet", 2, 1)
-- will return the third (2+1) amulet from the second (1+1) row
-- from the file gfx/dawnlike/Items/Amulet.png

function getdawntile(name, x, y)
  if not dawn[name] then dawn[name] = {} end
  
  if not dawn[name].f then
    dawn[name] = {f = loadimage(gfxdir.."dawnlike/"..name..".png")}
    end
  local id = 100*y+x
  if not dawn[name][id] then
    dawn[name][id] = addtile(dawn[name].f, 16*x, 16*y, 16, 16, transAlpha)
    end
  return dawn[name][id]
  end

dawn["Objects/Wall"] = { trans = transNone }
dawn["Objects/Floor"] = { trans = transNone }

-- animdawntile: will animate the tile by adding the "0" or "1" suffix
-- to the filename, based on the time
-- change the "getdawnframe" function to change the animation speed

function getdawnframe()
  return math.floor(getticks() / 500)
  end
  
function animdawntile(name, x, y)
  name = name..(getdawnframe() % 2)
  return
    getdawntile(name, x, y)
  end

-- specific dawn tiles

-- e.g. dawnmonster("Player", 1, 1)

function dawnmonster(mon, x,y)
  if mode == modeTPP then
    return simulateSpMonster(animdawntile("Characters/"..mon, x, y))
  else
    return tilespatial(animdawntile("Characters/"..mon, x, y), spFlat + spIItem + spMonst)
    end
  end

function dawnitem(it, x,y)
  if mode == modeTPP then
    return simulateSpItem(animdawntile("Items/"..mon, x, y))
  else
    return tilespatial(animdawntile("Items/"..it, x, y), spFlat + spIItem + spIItem)
    end
  end

-- dawnfloor(): get the DawnLike's floor tile
-- requires a function "getfloorat" which gets the floor type at the given coordinates

-- floor types are 1..12 (the first column), add 13 for the second column,
-- and 26 for the third column

local dawnfloordx = {5, 4, 3, 0, 6, 5, 2, 1, 3, 0, 3, 0, 2, 1, 2, 1}
local dawnfloordy = {0, 1, 2, 2, 1, 1, 2, 2, 0, 0, 1, 1, 0, 0, 1, 1}

function dawnfloor()
  local t = getfloorat(global_x, global_y)
  local s = 1
  if getfloorat(global_x+1, global_y) == t then s = s + 1 end
  if getfloorat(global_x, global_y-1) == t then s = s + 2 end
  if getfloorat(global_x-1, global_y) == t then s = s + 4 end
  if getfloorat(global_x, global_y+1) == t then s = s + 8 end
  local dt = getdawntile("Objects/Floor", 7*math.floor(t / 13)+dawnfloordx[s], 3*(t % 13)+dawnfloordy[s])
  return tilespatial(dt, spFlat + spIFloor + spFloor)
  end

-- dawnwallat(x,y): get the DawnLike's wall tile
-- x goes from 0 to 2, y goes from 1 to 17
-- requires a function iswallat(x,y) which says whether there is also a wall at these
-- coordinates

local dawnwalldx = {1, 1, 0, 0, 1, 1, 2, 4, 0, 0, 0, 3, 2, 4, 5, 4}
local dawnwalldy = {1, 0, 1, 2, 0, 0, 2, 2, 1, 0, 1, 1, 0, 0, 1, 1}

function dawnwall(x, y)
  local s = 1
  if iswallat(global_x+1, global_y) then s = s + 1 end
  if iswallat(global_x, global_y-1) then s = s + 2 end
  if iswallat(global_x-1, global_y) then s = s + 4 end
  if iswallat(global_x, global_y+1) then s = s + 8 end
  local dt = getdawntile("Objects/Wall", 6*x+dawnwalldx[s], 3*y+dawnwalldy[s])
  local dt3 = getdawntile("Objects/Wall", 6*x+1, 3*y)
  return tilemerge3(tilespatial(dt, spFlat), 
    tilespatial(dt3, spWall + spIWall),
    tilespatial(tilefill(0x101010), spWallTop + spICeil)
    )
  end

-- sample monster meanings

dawnmons = {}

dawnmons["A"] = {"Pest", 1, 1}
dawnmons["B"] = {"Pest", 1, 1}
dawnmons["C"] = {"Cat", 1, 0}
dawnmons["D"] = {"Reptile", 3, 11}
dawnmons["E"] = {"Elemental", 1, 3}
dawnmons["F"] = {"Elemental", 2, 4}
dawnmons["G"] = {"Humanoid", 7, 0}
dawnmons["H"] = {"Humanoid", 0, 7}
dawnmons["I"] = {"Elemental", 4, 1}
dawnmons["J"] = {"Dog", 0, 1}
dawnmons["K"] = {"Humanoid", 8, 0}
dawnmons["L"] = {"Reptile", 0, 7}
dawnmons["M"] = {"Rodent", 0, 1}
dawnmons["N"] = {"Humanoid", 0, 24}
dawnmons["O"] = {"Avian", 0, 3}
dawnmons["P"] = {"Plant", 0, 1}
dawnmons["Q"] = {"Quadraped", 0, 0}
dawnmons["R"] = {"Rodent", 0, 0}
dawnmons["S"] = {"Reptile", 2, 3}
dawnmons["T"] = {"Humanoid", 0, 6}
dawnmons["U"] = {"Quadraped", 2, 4}
dawnmons["V"] = {"Elemental", 2, 6}
dawnmons["W"] = {"Undead", 0, 5}
dawnmons["X"] = {"Undead", 0, 2}
dawnmons["Y"] = {"Humanoid", 1, 20}
dawnmons["Z"] = {"Undead", 2, 0}

dawnmons["a"] = {"Pest", 1, 1}
dawnmons["b"] = {"Pest", 1, 1}
dawnmons["c"] = {"Cat", 1, 0}
dawnmons["d"] = {"Reptile", 3, 11}
dawnmons["e"] = {"Elemental", 1, 3}
dawnmons["f"] = {"Elemental", 2, 4}
dawnmons["g"] = {"Humanoid", 7, 0}
dawnmons["h"] = {"Humanoid", 0, 7}
dawnmons["i"] = {"Elemental", 4, 1}
dawnmons["j"] = {"Dog", 0, 1}
dawnmons["k"] = {"Humanoid", 8, 0}
dawnmons["l"] = {"Reptile", 0, 7}
dawnmons["m"] = {"Rodent", 0, 1}
dawnmons["n"] = {"Humanoid", 0, 24}
dawnmons["o"] = {"Avian", 0, 3}
dawnmons["p"] = {"Plant", 0, 1}
dawnmons["q"] = {"Quadraped", 0, 0}
dawnmons["r"] = {"Rodent", 0, 0}
dawnmons["s"] = {"Reptile", 2, 3}
dawnmons["t"] = {"Humanoid", 0, 6}
dawnmons["u"] = {"Quadraped", 2, 4}
dawnmons["v"] = {"Elemental", 2, 6}
dawnmons["w"] = {"Undead", 0, 5}
dawnmons["x"] = {"Undead", 0, 2}
dawnmons["y"] = {"Humanoid", 1, 20}
dawnmons["z"] = {"Undead", 2, 0}

dawnmons["@"] = {"Player", 0, 0}

