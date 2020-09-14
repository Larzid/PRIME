-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- contains various generic functions for reading the process' map

loadfont("tinyfont", 32, 8, fontadj.simpleA)
MiniFont = Fonts["tinyfont"].font

minimap_pc = tonumber("FFFFFFFF", 16)
minimap_dot = tonumber("8404040", 16)
minimap_wall = tonumber("7C0C0C0", 16)
minimap_mon = tonumber("2F08000", 16)
minimap_stairs = tonumber("FF00FF", 16)
minimap_other = tonumber("0000FF", 16)

function xminimap(C)

  if x == playerpos.x and y == playerpos.y then
    return tilefill(tonumber("FFC0C0C0", 16))
    end
  
  if minimode == modeASCII then return setfont(C, MiniFont) end
  
  b = 0  
  local ch = gch(C)
  local av = av(ch)
  
  if ch == " " then b = 0
  elseif ispc(C) then b = minimap_pc
  elseif ch == "." then b = minimap_dot
  elseif av == 250     then b = minimap_dot
  elseif av == 249     then b = minimap_dot
  elseif ch == "#" then b = minimap_wall
  elseif av == 176     then b = minimap_wall
  elseif av == 177     then b = minimap_wall
  elseif av == 178     then b = minimap_wall
  elseif (ch >= "A" and ch <= "Z") or (ch >= "a" and ch <= "z") or (ch == "@") then 
    b = minimap_mon
  elseif ch == (">") or ch == ("<") then b = minimap_stairs
  else b = minimap_other
  end
  
  return tilefill(b)
  end

function ispc(C)
  return gch(C) == "@"
  end

function inmapregion(point)
  return inrect(point, D.map)
  end

function pcat(where)
  return where and inmapregion(where) and ispc(scrget(S, where.x, where.y))
  end

function findpc()
  -- maybe player is under the cursor
  if pcat(pcursor) then 
    ncursor = pcursor

  -- if not, look for him...
  else  
    for x=map.top.x,map.bot.x-1 do
    for y=map.top.y,map.bot.y-1 do
      if ispc(scrget(S, x,y)) then
        ncursor = V(x,y)
        end
      end end
    end

  mapon = pcat(ncursor)
  end

blinkcolorval = tonumber("10101", 16)
blinkcolorbase = tonumber("F000000", 16)

function blinkcolor()
  return blinkcolorbase + blinkcolorval * (128 + math.floor(math.sin(getticks()/60) * 100))
  end
  
function addcursor(C, size)
  local ch = "_"
  if size and size>25 then 
    return tilemergeover(C, tileshade(blinkcolor()))
  else
    local Cur=tilerecolor(fget(Font, "_"), blinkcolor())
    return tilemergeover(C, Cur)
    end
  -- return tilemerge(tilerecolor(fget(Font, "_"), 0xF000000 + 0x10101 * (128 + math.floor(math.sin(cphase/3) * 100))), C)
  end

function addbigcursor(C)
  local Cur=tilerecolor(fget(Font, "#"), blinkcolor())
  return tilemergeover(C, Cur)
  -- return tilemerge(tilerecolor(fget(Font, "_"), 0xF000000 + 0x10101 * (128 + math.floor(math.sin(cphase/3) * 100))), C)
  end

function clearmoveanimations()
  oplayerpos = nil
  moveanimations = {}
  end

clearmoveanimations()

function copymap()
  local ncs = V.scrgetsize(S)
  if ncs ~= conssize then
    conssize = ncs
    prepareconsole()
    end

  -- copy from the process map
  scrcopy(S, 0, 0, IMG, 0,0, conssize.x, conssize.y, eq)

  cphase = cphase + 1
  cursor = pcursor

  if cursor.size and cursor.size > 0 then
    scrset(IMG, cursor.x, cursor.y, addcursor(scrget(IMG, cursor.x, cursor.y), cursor.size))
    end

  findpc()
  moveonwheel = true
  
  if mapon then
    -- yeah, there he is!
    playerpos = ncursor - D.map.top
    if not oplayerpos then
      recenterOnPlayer()
    elseif playerpos.x ~= oplayerpos.x or playerpos.y ~= oplayerpos.y then
      oplayerpos = V.copy(playerpos)
      playermoved()
      end
    scrset(IMG, 999, 999, 0) -- 0 outside the map
    V.scrcopy(S, D.map.top, MAP, 0, D.map.size, eq)

    if mode ~= modeASCII then
      V.scrfill(IMG, rectTS(D.map.top, D.map.size), 0)
      end
    
    readgmessages()
  else
    oplayerpos = nil
    mapon = false
    end
  end

function recenterOnPlayer()
  oplayerpos = V.copy(playerpos)
  mapcenter = V.copy(playerpos)
  scrollmanual = false
  end

-- read the process' lines

function readline(Scr, x1, x2, y)
  s = ""
  for x = x1,x2-1 do 
    s = s .. gch(scrget(Scr, x, y))
    end
  return s
  end

function getline(y)
  return readline(S, 0, conssize.x-1, y)
  end

-- we got a message from the game
-- (override to do special effects, e.g., sound effects)
function gotgmessage(s)
  end

-- read the game's messages
function readgmessages()
  if msgbox == nil then return true end
  local oldgmsg = gmsg
  local function testeqmsg(oldofs, count, offset)
    for y = oldofs,count do 
      if gmsg[y] ~= oldgmsg[y+offset] then 
        return false 
        end
      end
    return true
    end
  gmsg = {}
  msglines = msgbox.y1-msgbox.y0
  for y = 1,msglines do 
    gmsg[y] = readline(S, msgbox.x0, msgbox.x1, msgbox.y0-1+y)
    end
  if not oldgmsg then
    for y = 1,msglines do 
      gotgmessage(gmsg[y])
      end
    return
    end
  for nmsg = 0,msglines do
    if testeqmsg(1, msglines-nmsg, nmsg) then
      for y = 1,nmsg do 
        gotgmessage(gmsg[msglines-nmsg+y])
        end
      return
      end
    end
  end

