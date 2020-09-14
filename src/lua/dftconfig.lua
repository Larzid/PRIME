-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- NotEye configuration file

-- default visual settings:
----------------------------

-- start in fullscreen mode?
fscr = false

-- keep the original resolution in fullscreen mode?
fscrmode = true

-- use OpenGL initially?
useGL = false

-- which mode to start in (modeASCII, modeTiles, modeFPP, modeISO, modeMini)
-- mode = modeTiles
mode = modeTiles
modepower = 10

-- the default palette (use palpower > 7 to force it even if the game has its own
-- palette, like Frozen Depths or Hydra Slayer)
setpalette(palettes.dos)
palpower = 0

-- default font
-- curfont = "cp437-16"
curfont = "dejavu-20"
curfonttype = fontadj.simpleA
fontsize = {rx=32, ry=8}
reqres = {f = V(1,1)}

-- reserve lines for messages below the screen
msgreserve = 0

-- have graphics
havegfx = true

-- have ascii (Note: in ASCII, NotEye menu is ran with Ctrl+[, not Ctrl+M)
-- also, translating TrueColor games back to 16 colors does not work well yet
haveascii = false

-- input settings
------------------

-- allow to use SHIFT/CTRL for diagonal movement
diagleft = true
diagright = true

-- rotate both numpad and arrow keys
rotatenumpad = true
rotatearrowkeys = true

-- see generic.lua for other names
-- initialKeymap = mapWASDplus

-- rec/view/server/client configuration
-----------------------------------------

-- initial recording and viewing speed

recspeed = 1000
viewspeed = 1000

-- default filenames

recfile = recfile or ((userdir or "").."record.nev")
viewfile = viewfile or ((userdir or "").."view.nev")

-- binary flags: enter "false" or "true"
-- automatically start recording
autorec = false
-- start in view mode instead of game mode
viewmode = false
-- start server automatically (make sure that you know what you are doing!)
autoserve = false
-- record mode changes by default?
servemode = true
-- accept input from clients by default?
serveinput = true
-- should we get font from the stream?
takefont = false

-- default network configuration
--------------------------------

-- for client:
portclient = 6677
serverhost = "localhost"

-- libtcod client:
libtcodhost = "localhost"
libtcodport = 6678

-- for server:
portserver = 6677

-- audio options
----------------

volsound = 100
volmusic = 80

-- use the default music even if the game's noe file
-- disables it, or defines its own soundtrack
forcemusic = false

-- user-defined music:
-- resetmusic()
-- addmusic("sound/music1.ogg")
-- addmusic("sound/music2.ogg")

-- misc options
---------------

-- allow taking screenshots quickly with F2
-- quickshots = true

-- enable this to see the coordinates and RGB colors of characters in libtcod on mouseover
conshelp = false

-- do you want to force external Hydra Slayer
externalhydra = false

-- change this to true if you want to start the games yourself
-- (useful e.g. for playing via a telnet/ssh server)
manualcall = false

-- user key remapping
---------------------

-- user key remappings: some examples

-- uncomment if you want them to be disabled by default
-- remapsoff = true

-- replace "1" and "2" in Hydra Slayer, only while viewing the map
-- remapkey("1", "2", onlymap(ingame("hydra")))
-- remapkey("2", "1", onlymap(ingame("hydra")))

-- replace F1 and F2 in Hydra Slayer (see sdlkeys.lua for other key names)
-- remapkey(SDLK_F1, SDLK_F2, ingame("hydra"))
-- remapkey(SDLK_F2, SDLK_F1, ingame("hydra"))

-- replace "z" and "y" in all games
--remapkey("z", "y", inallgames)
--remapkey("y", "z", inallgames)

-- make Enter start Hydra Slayer in the main menu
--remapkey(SDLK_RETURN, "h", inmenu("noteyemain"))
