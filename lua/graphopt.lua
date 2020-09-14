-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

graphmenu = {}

addtomenu(mainmenu, "o", writechoice("animations, scrolling, palette, etc."),
  function()
    menuexecute(graphmenu, dfthdr("Other graphical options"))
    end
  )

animsmenu = graphmenu

--addtomenu(graphmenu, "a", writechoice("configure animations and scrolling"),
--  function()
--    menuexecute(animsmenu, dfthdr("Animations & Scrolling"))
--    end
--  )

function spdval(z)
  if z == 0 then return "0 (no movement)"
  elseif z <= 10 then return z.." (extremely slow)"
  elseif z <= 50 then return z.." (slow)"
  elseif z <= 99 then return z.." (a bit slow)"
  elseif z == 100 then return z.." (normal)"
  elseif z <  200 then return z.." (a bit fast)"
  elseif z <  1000 then return z.." (fast)"
  elseif z < 10000 then return z.." (extremely fast)"
  else return z.." (instant)"
  end
  end

addtomenu(animsmenu, "m", writechoicef(function() return "movement animation speed: \ve"..spdval(spdMoveanim) end), 
  function()
    pickinteger("spdMoveanim", asknum(spdMoveanim, "Movement speed", 10000, {
      "This value controls the speed of movement animations, for games which support ",
      "this feature.", "",
      "100 is normal speed. 10000 is instant.", "",
      "Note that when the Movement Animation speed equals Autocentering speed, ",
      "the player character remains centered all the time."
      }))
    return true
    end
  )

addtomenu(animsmenu, "c", writechoicef(function() return "auto-centering speed: \ve"..spdval(spdAutocenter) end), 
  function()
    pickinteger("spdAutocenter", asknum(spdAutocenter, "Movement speed", 10000, {
      "This value controls the speed of smooth auto-centering.", "",
      "100 is normal speed. 10000 is instant auto-centering.",
      "0 is no auto-centering (enter a menu to center, or scroll manually)."
      })
      )
    return true
    end
  )

addtomenu(animsmenu, "s", writechoicef(function() return "manual scrolling speed: \ve"..spdval(spdScrolling) end), 
  function()
    pickinteger("spdScrolling", asknum(spdScrolling, "Movement speed", 10000, {
      "You can scroll the map by placing the mouse pointer on the edge of the map.",
      "This value controls the speed of this.", "",
      "100 is normal speed, 0 is disable manual scrolling."
      })
      )
    return true
    end
  )

addtomenu(animsmenu, "r", writechoicef(function() return "manual scrolling region width: \ve"..scrollwidth.."%" end), 
  function()
    pickinteger("scrollwidth", asknum(scrollwidth, "Scrolling region width: ", 20, {
      "You can scroll the map by placing the mouse pointer on the edge of the map.",
      "This value controls the size of the scrolling area, as percentage of the ",
      "map area. This works on both ends, so 50% scrolls everywhere."
      })
      )
    return true
    end
  )

addtomenu(animsmenu, "=", writechoice("set movement speed to autoscroll speed"), 
  function() pickinteger("spdMoveanim", spdAutocenter) end
  )

spdMissile = 100

addtomenu(animsmenu, "p", writechoicef(function() return "projectile animation speed: \ve"..spdval(spdMissile) end), 
  function()
    pickinteger("spdMissile", asknum(spdMissile, "Projectile animation speed", 10000, {
      "This value controls the speed of missile animations, for games which support ",
      "this feature.", "",
      "100 is normal speed. 10000 is instant."
      }))
    return true
    end
  )

-- tools for movement animation

function checkmonstershift(ID)
  if ID and global_x and global_y then
    local gxy = V(global_x, global_y)
    Data = moveanimations[ID]
    if not Data then 
      Data = {}
      moveanimations[ID] = Data
      end
    if not Data.animxy then
      Data.animxy = V.copy(gxy)
      Data.lasttick = scrollTick
    elseif Data.animxy ~= gxy and scrollTick then
      if V.norm(Data.animxy-gxy) > 25 then
        Data.animxy = gxy
        return nil
        end
      local newtick = scrollTick
      local sval = (newtick - Data.lasttick) * spdMoveanim / 40000
      Data.lasttick = newtick
      local shift = Data.animxy - gxy
      Data.animxy = scrollfv(Data.animxy, gxy, sval)
      if Data.animxy == gxy then return nil end
      return Data.animxy - gxy
    else
      Data.lasttick = scrollTick
      end
    end
  end

amsmul = V(1,1)

bobval = -0.04

function applymoveshift(T, ID, bob)

  if mode == modeFPP or mode == modeTPP then
    T = simulateSpAuto(T)
    end

  local shift = checkmonstershift(ID)
  if not shift then return T end
  local dy = bob and (bobval * math.max(math.abs(math.sin(shift.x * 2 * math.pi)), math.abs(math.sin(shift.y * 2 * math.pi)))) or 0
  return applyshift(T, shift, dy)
  end

-- ffidUpright : transform a character on floor to one standing upright (facing south)

ffidUpright = freeformparam(
  1,0,0.5,1,
  0,1,0,0,
  0,0,0,-1,
  0,0,1,0
  )

freeformparamflags(ffidUpright, 0, true)

-- ffidPlaneRotate[i] rotates a character on floor by i degrees
-- ffidPlaneFacing[i] rotates a character so that its UP is UP when rotated in facing i
-- ffidMonsterAngle[i] adjusts a monster on floor to camera angle i

ffidPlaneRotate = {}
ffidPlaneRotateHex = {}
ffidMonsterAngle = {}

ffidPlaneFacing = {}
ffidPlaneFacingHex = {}

for alfa=0,359 do
  local alfarad = alfa * math.pi / 180
  local C = math.cos(alfarad)
  local S = math.sin(alfarad)
  
  local r3 = math.sqrt(3)

  ffidPlaneRotate[alfa] = freeformparam(
    1, 0.5*(1-C+S), 0.5*(1-C-S), 0,
    0, C, S, 0,
    0,-S, C, 0,
    0, 0, 0, 1
    )
  ffidPlaneRotateHex[alfa] = freeformparam(
    1, 0.5*(1-C+S), 0.5*(1-C-S), 0,
    0, C*r3, S, 0,
    0,-S*r3, C, 0,
    0, 0, 0, 1
    )
  freeformparamflags(ffidPlaneRotate[alfa], 0, true)
  ffidPlaneFacing[(450-alfa)%360] = ffidPlaneRotate[alfa]
  ffidPlaneFacingHex[(450-alfa)%360] = ffidPlaneRotateHex[alfa]

  -- (1, 0, 1, 0) => (1, 0, 1, 0)
  
  ffidMonsterAngle[alfa] = freeformparam(
    1, 0, 1-S, C,
    0, 1, 0, 0,
    0, 0, S, -C,
    0, 0, C, S
    )
  freeformparamflags(ffidPlaneRotate[alfa], 0, true)
  end

function be360(alfa)
  alfa = alfa % 360
  if alfa < 0 then alfa = alfa + 360 end
  return math.floor(alfa)
  end

function raddeg(alfa)
  return be360(alfa  * 180 / math.pi)
  end

function simulateSpMonster(t)
  if t == 0 then return 0 end

  --local alfa = 
  --  raddeg(math.pi -V.atan2(V(global_x, global_y) - (mode==modeTPP and mapcenterTPP or mapcenter)))
  
  if tppAngleRound then
    t = tilefreeform(t, ffidMonsterAngle[tppAngleRound])
  else
    t = tilefreeform(t, ffidUpright)
    end
  if simFacing then
    t = tilefreeform(t, (fppvar.hex and ffidPlaneFacingHex or ffidPlaneFacing)[simFacing])
    end
  return tilespatial(t, spFree)
  end

function simulateSpItem(t)
  return simulateSpMonster(tiletransform(t, 0.25, 0.5, 0.5, 0.5))
  end

function simulateSpAuto(t)
  if mode ~= modeFPP and mode ~= modeTPP then return 0 end
  
  return tilemerge3(
    tilespatial(getdistill(t, spFree), spFree),
    simulateSpItem(getdistill(t, spItem)),
    simulateSpMonster(getdistill(t, spMonst))
    )
  end

function adjustTPP(t)
  if mode == modeTPP then return simulateSpAuto(t) end
  return t
  end

function adjTppItem(t)
  if mode == modeTPP then return simulateSpItem(t) end
  return tilespatial(t, spFlat + spItem + spIItem)
  end

function adjTppMonster(t)
  if mode == modeTPP then return simulateSpMonster(t) end
  return tilespatial(t, spFlat + spMonst + spIItem)
  end

function applyshift(T, shift, dy)
  if mode == modeISO and isi then
    local sx = isi.floor*(shift.x-shift.y) / isi.icon
    local sy = isi.floor*(shift.x+shift.y)/2 / isi.iconh
    return tiletransform(T, sx, sy, 1, 1)
  elseif mode == modeFPP or mode == modeTPP then
    shift = shift * amsmul
    -- todo: bobbing
    return tiletransform(T, shift.x, shift.y, 1, 1)
  else
    shift = shift * amsmul
    return tiletransform(T, shift.x, shift.y + dy, 1, 1)
    end
  end

if imagebackgrounds == nil then imagebackgrounds = true end

addtomenu(graphmenu, "b", writechoicef(
  function() 
    return "background images: \ve"..boolonoff(imagebackgrounds)
    end),
  function()
    pickbool("imagebackgrounds", not imagebackgrounds)
    end
  )

detaillevel = 100
violencelevel = 100

function detval(z)
  if z == 0 then return "0 (no details)"
  elseif z <= 10 then return z.." (almost no details)"
  elseif z < 40 then return z.." (low detail)"
  elseif z < 80 then return z.." (medium detail)"
  elseif z < 120 then return z.." (high detail)"
  else return z.." (extremely high detail)"
  end
  end

function violval(z)
  if z == 0 then return "0 (no violence)"
  elseif z < 25 then return z.." (almost no violence)"
  elseif z < 75 then return z.." (corpses, no blood)"
  elseif z < 125 then return z.." (blood and corpses)"
  else return z.." (lots of violence)"
  end
  end

detailinfo = {}

violenceinfo = {}

function addinfo(info, val, text)
  if info[val] then
    info[val] = (info[val] + ", ") + text
  else
    info[val] = text
    end
  end

function listinfo(info, dft, list)
  local empty = true

  local infotable = {}
  for k in pairs(info) do table.insert(infotable, k) end
  table.sort(infotable)
    
  for kk,k in ipairs(infotable) do
    table.insert(list, k.."+ - " ..info[k])
    empty = false
    end
  if empty then
    table.insert(list, dft)
    end
  end

addtomenu(graphmenu, "d", writechoicef(
  function() 
    return "detail level: \ve"..detval(detaillevel)
    end),
  function()
    local dil = {
      "Some special graphical effects might consume time and memory, ",
      "making NotEye run less efficiently. Low values of detail level ",
      "disable such effects.", 
      "(Use left/right arrows to switch between different values)",
      ""
      }
    listinfo(detailinfo, "The current game has no controllable detail level.", dil)
    pickinteger("detaillevel", 
      asknum(detaillevel, "Detail level", 100, dil, levelarrows(detailinfo)))
    end
  )

addtomenu(graphmenu, "v", writechoicef(
  function() 
    return "violence level: \ve"..violval(violencelevel)
    end),
  function()
    local dil = {
      "Some imagery, such as blood and corpses, might be not appropriate for ",
      "some players. Low values of violence level disable such imagery. ",
      "(Use left/right arrows to switch between different values)",
      ""
      }
    listinfo(violenceinfo, "The current game has no controllable violence level.", dil)
    pickinteger("violencelevel", 
      asknum(violencelevel, "Violence level", 100, dil, levelarrows(violenceinfo)))
    end
  )

function levelarrows(t)
  return function(ev, val)
    if ev.type == evKeyDown then 
      if ev.symbol == SDLK_RIGHT then
        local newval = 100
        for k,v in pairs(t) do 
          if k > 0+val and k < newval then newval = k end
          end
        return newval
        end
      if ev.symbol == SDLK_LEFT then
        local newval = 0
        for k,v in pairs(t) do 
          if k < 0+val and k > newval then newval = k end
          end
        return newval
        end
      end
    end
  end
