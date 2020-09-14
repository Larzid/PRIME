-- NotEye configuration file

-- default visual settings:
----------------------------

-- start in fullscreen mode
fscr = false

-- use OpenGL initially?
useGL = false

-- which mode to start in (modeASCII, modeTiles, modeFPP, modeISO, modeMini)
mode = modeTiles -- modeISO

-- modepower == 10 : always use the mode above
-- modepower ==  0 : always start in the default mode for the given game
-- intermediate values : depending on the game
-- (e.g. DoomRL switches to FPP if initmodepower<7)
modepower = 0

-- default font
-- curfont = "cp437-16"
curfont = "dejavu-20"
curfonttype = fontSimpleA
fontsize = {rx=32, ry=8}
fontscale = {x=1, y=1}

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

-- see generic.noe for other names
-- initialKeymap = mapWASDplus

-- rec/view/server/client configuration
-----------------------------------------

-- initial recording and viewing speed

recspeed = 1000
viewspeed = 1000

-- default filenames

recfile = "record.nev"
viewfile = "view.nev"

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
volmusic = 0

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
quickshots = false

-- enable this to see the coordinates and RGB colors of characters in libtcod on mouseover
conshelp = false
