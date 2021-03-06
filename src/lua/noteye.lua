-- Necklace of the Eye v7.5 roguelike frontend, the main script
-- Copyright (C) 2010-2014 Zeno Rogue

-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

noteyeloaded = true

if not noteyedir then
  noteyedir = (os.getenv("NOTEYEDIR") or ".").."/"
  end

if not gamesdir then
  gamesdir = noteyedir .. "lua/"
  end

if not commondir then
  commondir = noteyedir .. "lua/"
  end

if not gfxdir then
  gfxdir = noteyedir .. "gfx/"
  end

if not sounddir then
  sounddir = noteyedir .. "sound/"
  end

noeflags = not hydraver
i = 1

while argv(i) do
  if argv(i) == "-N" then noeflags = true
  elseif argv(i) == "-N0" then noeflags = false
  elseif noeflags and argv(i) == "-C" then 
    i = i+1
    configfile = argv(i)
  elseif noeflags and argv(i) == "-U" then 
    i = i+1
    userdir = argv(i)
  elseif noeflags and argv(i) == "-C1" then 
    i = i+1
    configfileformat = argv(i)
  elseif noeflags and argv(i) == "-tcod" then 
    autoconnecttcod = true
  elseif noeflags and argv(i) == "-connect" then 
    autoconnectserver = true
  elseif noeflags and argv(i) == "-L" then 
    i = i+1
    logopen(argv(i))
  elseif noeflags and argv(i) == "-X" then 
    i = i+1
    game_to_launch = argv(i)
  elseif noeflags and argv(i) == "--ascii" then 
    setascii = true
    end
  i = i+1
  end

userdir = userdir or (noteyedir.."user/")
shotdir = shotdir or userdir
recorddir = recorddir or userdir
configfile = configfile or (userdir.."config.lua")
configfileformat = configfileformat or (userdir.."config-GAMENAME.lua")

havemain = true

print("Necklace of the Eye v"..noteyeversion.." (C) 2010-2014 Zeno Rogue")
print("Homepage: http://www.roguetemple.com/z/noteye.php")
print("NotEye is distributed under GNU General Public License version 3 or later")
print("It comes with absolutely no warranty; see the file COPYING for details")

dofile(commondir.."generic.lua")

function createmain()
  dofile(commondir.."generic.lua")
  prepareconsole()
  createmainscr()
  end

function starttcod(name)
  STCOD = newscreen(80, 25)
  PTCOD = newprocess(STCOD, Font, "batch\\"..name..".bat")
  sleep(800)
  connecttotcod()
  end

function createmainscr()
  
  setwindowtitle("Necklace of the Eye v"..noteyeversion)
  scrfill(IMG, 0, 0, 80, 25, 0)
  
  local Col = vgaget(10)

  local function write(y, text)
    scrwrite(IMG, 1, y, text, Font, Col)
    end
  
  local function writef(y, text)
    scrwrite(IMG, 40, y, text, Font, Col)
    end
  
  local function writex(x, y, text)
    scrwrite(IMG, x, y, text, Font, Col)
    end
  
  write(1, "Welcome to Necklace of the Eye v"..noteyeversion.."!")

  Col = vgaget(7)
  write(2, "Copyright (C) 2010-2014 Zeno Rogue")
  write(3, "Press \veCtrl+M\vp, \veCtrl+[\vp or \veF4\vp while playing to get the NotEye menu.")

  Col = vgaget(15)
  write(5, "Choose the game to play...")
  
  if hydraver and not externalhydra then
    write(7, "\veh\vp - Hydra Slayer "..hydraver.." (integrated into NotEye)")
  else
    write(7, "\veh\vp - Hydra Slayer")
    end

  write(9,  "\ver\vp - Rogue           \vel\vp - nLarn      \veL\vp - Lua sample (included)")
  write(10, "\ved\vp - DoomRL          \vec\vp - Crawl      \veu\vp - Unstoppable")
  write(11, "\vef\vp - Frozen Depths   \ven\vp - NetHack    \vet\vp - Toby the Trapper")
  write(12, "\vek\vp - ChessRogue      \ves\vp - Gruesome")

  Col = vgaget(7)
  write(15, "check NotEye's website to see how well these games are supported currently")

  Col = vgaget(15)
  write(17, "\veg\vf - generic console roguelike (shell)")
  if not viewmode then
    write(18, "\vem\vf - NotEye menu (about NotEye, libtcod games, fonts, etc)")
    write(19, "\veq\vf - quit")
  else
    writex(1, 19, "select your configuration from the list above")
    end
  
  end

-- it is better to change this function if you intend to add special 'games'

function startbyname2()
  if gamename == "generic" then 
    conssize.x = 80
    conssize.y = 25
    setmode(1)
    dofile(commondir.."generic-tile.lua")
    setwindowtitle("NotEye: generic roguelike")
    local cmdline = linux and ("sh -l") or "cmd.exe"
    P = rungame(cmdline)
  elseif string.match(gamename, "^[a-zA-Z0-9]*$") then
    dofile(gamesdir..gamename..".lua")
  else
    print("Unknown name: "..gamename)
    end
  end

function startbyname(name)
  gamename = name
  local savemode = mode
  local savemodepower = modepower
  startbyname2()
  if not threaded then 
    mode = savemode
    modepower = savemodepower
    end
  end

function defaultmodep(x, prio)
  -- other modes simply hide a part of the display with no graphics
  if modepower >= 10 then -- do nothing
  elseif not havegfx then mode = modeASCII
  elseif crashsafe then mode = modeASCII
  elseif modepower < prio then mode = x
  end
  autoGL()
  end

function defaultmode(x)
  defaultmodep(x, 5)
  end

-- run some tests

colWHITE = tonumber("FFFFFFFF", 16)
colBLACK = tonumber("FF000000", 16)

function starttest()
  local img = newimage(64,64, 0)
  for y=0,63 do for x=0,63 do
    local col = (x+y)%2 == 1
    col = col and colWHITE or colBLACK
    setpixel(img,x,y,col)
    end end
  local til = addtile(img, 0, 0, 64, 64, transAlpha)
  while true do
  
    rot = freeformparam(
      1,0.5,0,0,
      0,0.5,0.5,0,
      0,-0.5,0.5,0,
      0,0,0,0
      )

    drawtile(1, noteyetile, 0, 0, xscrsize.x, xscrsize.y)
    drawtile(1, til, 0, 0, 64, 64)
    drawtile(1, til, 64, 0, 32, 64)
    drawtile(1, til, 96, 0, 32, 32)
    drawtile(1, tilefreeform(til, rot), 0, 64, 64,64)
    updscr()
    lev = getevent()
    if lev.type == evKeyDown then break end
    end
  end

function noteyemain()

  gamename = "auto"
  
  if noteyecrash then 
    dofile(commondir.."crashmenu.lua")
    return
    end
  
  if game_to_launch then
    havemain = false
    prepareconsole()
    startbyname(game_to_launch)
    return
    end

  createmain()
  
  local function redrawmenu()
    createmainscr()
    viewmenu()
    end
  
  redrawmenu()
  
  while true do
    if autoconnecttcod then
      connecttotcod()
      autoconnecttcod = false
      end
    if autoconnectserver then
      connecttoserver()
      autoconnectserver = false
      end
    inmenu="noteyemain"
    lev = getevent()
    
    if lev.type == evQuit then break
    elseif lev.type == evMouseDown then
      local y = math.floor(lev.y / 16)
      local ch = gch(scrget(IMG, 1, y))
      lev.type = evKeyDown
      lev.symbol = av(string.lower(ch))
      end
      
    if lev.type == evMouseMotion then

    elseif lev.type == evResize then  
      resizewindow(lev)

    elseif lev.type == evKeyDown then
      userremap(lev)
    
      if lev.chr == av("q") then break
      elseif lev.chr == av("z") then starttest()
      elseif lev.chr and menugames[string.char(lev.chr)] then startbyname(menugames[string.char(lev.chr)])
      elseif lev.chr == av("m") or ismenukey(lev) then 
        gamename = "auto"
        noteyemenu()
      elseif isfullscreenkey(lev) then
        applyfullscreenkey(lev)
      elseif isextramainmenuoption(lev) then
        end
      redrawmenu()

    else
      menudelay()
      end
    end
  end

function isextramainmenuoption(lev)
  -- add extra main menu options
  end

menugames = {}
menugames["h"] = "hydra"
menugames["f"] = "frozen"
menugames["c"] = "crawl"
menugames["d"] = "doomrl"
menugames["r"] = "rogue"
menugames["l"] = "nlarn"
menugames["L"] = "lusample"
menugames["n"] = "nethack"
menugames["u"] = "unstoppable"
menugames["t"] = "trapper"
menugames["k"] = "chessrogue"
menugames["s"] = "gruesome"
menugames["g"] = "generic"

if not file_exists(configfile) and file_exists(commondir.."dftconfig.lua") then
  file_copy(commondir.."dftconfig.lua", configfile)
  end

dofile (configfile)

if setascii then
  havegfx = false
  haveascii = true
  mode = modeASCII
  modepower = 15
  end

clonetrack(soundtrack, baksoundtrack)

selectfontfull(loadfont(curfont, fontsize.rx, fontsize.ry, curfonttype))

mode = mode or modeASCII

--function mainscript()
--  profstart("total")
--  noteyemain()
--  profend("total")
--  profinfo()
--  end

-- noteyemain()

createmain()
gamename = "auto"
loadgameconfig()

if file_exists(commondir.."../../bundle.lua") and not game_to_launch then
  dofile(commondir.."../../bundle.lua")
else
  noteyemain()
  end
