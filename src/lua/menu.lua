-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- the menu subsystem

-- == menu subsystem ==

menuy = 1

-- sleep(100) should not be very noticeable
function menudelay()
  -- do not count frames while the viewer/recorder is in menu
  lastviewframe = nil
  -- sleep for a longer time, to conserve CPU
  sleep(10)
  end

function addtomenu(M, key, writer, act, hid)
  table.insert(M, {key = key, displayAt = writer, action = act, hidden = hid})
  end

function silent(M) end

function keyname(k)
  if av(k) < 32 then return "Ctrl+"..string.char(av(k)+96)
  else return k end
  end

function writechoice(x)
  return function(M) 
    scrwrite(IMG, 1, menuy, "\ve"..keyname(M.key).."\vp\v7 - \vp"..x, Font, vgaget(15))
    menuy = menuy + 1
    end
  end

function writechoicef(x)
  return function(M) 
    scrwrite(IMG, 1, menuy, "\ve"..keyname(M.key).."\vp\v7 - \vp"..x(), Font, vgaget(15))
    menuy = menuy + 1
    end
  end

function writechoicecol(x, cf)
  return function(M) 
    scrwrite(IMG, 1, menuy, "\ve"..M.key.."\vp\v7 - \vp"..x, Font, vgaget(cf()))
    menuy = menuy + 1
    end
  end

function dfthdr(title)
  return function()
    menuy = 1
    titl = title .. "\n"
    col = vgaget(10)
    while titl ~= "" do
      local i = string.find(titl, "\n")
      if i>conssize.x then 
        scrwrite(IMG, 1, menuy, string.sub(titl, 1, conssize.x-1), Font, col)
        scrwrite(IMG, 0, menuy+1, "-", Font, vgaget(4))
        titl = string.sub(titl, conssize.x)
      else
        scrwrite(IMG, 1, menuy, string.sub(titl, 1, i-1), Font, col)
        titl = string.sub(titl, i+1)
        col = vgaget(7)
        end
      menuy = menuy+1
      end
    menuy = menuy + 1
    end
  end

function mainhdr()
  menuy = 3
  title = 
    hydraonly and "Hydra Slayer "..hydraver
    or "Necklace of the Eye "..noteyeversion.." - "..gamename
  scrwrite(IMG, 1, 1, title, Font, vgaget(10))
    
  scrwrite(IMG, 1, 23, (gamename ~= "auto") and "press \veENTER\vp to resume playing" or "press \veENTER\vp to return to the main menu", Font, vgaget(7))
  end

if noteyelogo == nil then
  noteyelogo = loadimage(gfxdir.."noteye-logo-dark.png")
  noteyetile = addtile(noteyelogo, 0, 0, 400, 300, -1)
  end

function dftmenubackground()
  drawtile(1, noteyetile, 0, 0, xscrsize.x, xscrsize.y)
  end

drawmenubackground = dftmenubackground

-- view the menu (stored in IMG) on the screen
function viewmenu()
  if havegfx then
    drawmenubackground()
    
    scrcopy(IMG, 0,0, IMGX, 0,0, conssize.x, conssize.y, cellblacknb)
                                   
    drawscreen(1, IMGX,-1, 0, fontsize.x, fontsize.y)
    drawscreen(1, IMGX, 0,-1, fontsize.x, fontsize.y)
    drawscreen(1, IMGX, 1, 0, fontsize.x, fontsize.y)
    drawscreen(1, IMGX, 0, 1, fontsize.x, fontsize.y)

    drawscreen(1, IMG, 0, 0, fontsize.x, fontsize.y)
    drawmessage()
    updscr()
    end
  if haveascii then
    scrcopy(IMG, 0, 0, mainscreen, 0,0, conssize.x, conssize.y, eq)
    drawmessage()
    refreshconsole()
    end
  end

function menuexecute(M, hdr, tail)

  drawmenubackground = dftmenubackground

  scrfill(IMG, 0, 0, conssize.x, conssize.y, 0)

  hdr()
  
  for k,v in pairs(M) do
    if not (v.hidden and v.hidden()) then
      v.displayAt(v)
      end
    end
  
  if tail then tail() end

  viewmenu()

  while true do
    inmenu = M

    lev = getevent()
    cont = false

    if lev.type == evMouseDown then
      local y = math.floor(lev.y / fontsize.y)
      local x = math.floor(lev.x / fontsize.x)
      
      local ch = gch(scrget(IMG, 1, y))
      lev.type = evKeyDown
      lev.symbol = av(string.lower(ch))
      lev.chr = av(string.lower(ch))
      end

    if lev.type == evKeyDown then
      userremap(lev)
      if lev.chr > 0 and lev.chr < 256 then
        local chr = string.char(lev.chr)
        for k,v in pairs(M) do
          if chr == v.key and not (v.hidden and v.hidden()) then
            cont = v.action(v)
            break
            end
          end
        if not cont then break end
        end
      -- redraw the menu
      return menuexecute(M, hdr, tail)
    elseif checkstreams() then
    elseif lev.type == evResize then  
      resizewindow(lev)
      -- in the resolution menu, this changes the header display
      return menuexecute(M, hdr, tail)
    elseif lev.type == 0 then
      menudelay()
      end
    end
  clrscr()
  end

-- == ask for string, and applymod ==

serverwarning = {
  "Warning: BE CAREFUL WHILE RUNNING AS A SERVER!",
  "see the website for details: ",
  "\vehttp://www.roguetemple.com/z/noteye.php\vp"
  }  
  
function askstr(text, question, note, symbolfunction)

  text = ""..text -- it was actually a number, convert to string
  
  local function redrawmenu()
    scrfill(IMG, 0, 0, conssize.x, conssize.y, 0)

    local function write(y, text)
      scrwrite(IMG, 1, y, text, Font, vgaget(15))
      end

    write(1, question)
    
    write(3, text)
    
    write(5, "\veENTER\vp to submit, \veESC\vp to cancel, \veTAB\vp to erase")
    
    if note then for k,v in pairs(note) do
      write(6+k, v)
      end end
    
    viewmenu()
    end
    
  while true do

    redrawmenu()
    inmenu = "askstr"
    lev = getevent()
    if lev.type == evKeyDown then
--    print("sym = "..(lev.symbol).." ch = "..(lev.chr).." mod = "..(lev.mod))
      userremap(lev)
      local newtext = symbolfunction and symbolfunction(lev, text)
      if newtext then 
        text = ""..newtext
        redrawmenu()
      elseif lev.symbol == 27 then 
        return
      elseif lev.symbol == SDLK_TAB then 
        text = ""
        redrawmenu()
      elseif lev.symbol == SDLK_RETURN or lev.symbol == SDLK_KP_ENTER then 
        return text
      elseif lev.symbol == SDLK_BACKSPACE then 
        text = string.sub(text, 0, #text-1)
        redrawmenu()
      elseif lev.chr > 0 and lev.chr < 256 then
        text = text .. string.char(lev.chr)
        redrawmenu()
        end
    elseif lev.type == evKeyUp then
      userremap(lev)
      local newtext = symbolfunction and symbolfunction(lev, text)
      if newtext then 
        text = ""..newtext
        redrawmenu()
        end
    elseif checkstreams() then
    elseif lev.type == evResize then  
      resizewindow(lev)
      viewmenu()
    elseif lev.type == 0 then
      menudelay()
      end
    end

  end

function asknum(num, question, default, note, symbolfunction)
  local ret = askstr(num, question, note, symbolfunction)
  if not ret or not tonumber(ret) then return default end
  return tonumber(ret)
  end

function applymod(val, mod)
  if bAND(mod, KMOD_LSHIFT + KMOD_RSHIFT) > 0 then
    return val / 2
  elseif bAND(mod, KMOD_LCTRL + KMOD_RCTRL) > 0 then
    local ret = askstr(""..val, "Enter a new value:")
    if ret and tonumber(ret) then return tonumber(ret)
    else return val end
  else
    return val * 2
    end
  end

-- == screenshots menu ==
function gameonly() return gamename == "auto" end
function nogameonly() return gamename ~= "auto" end

shotsmenu = {}

addtomenu(shotsmenu, "b", writechoice("BMP"), function()
    isactive = false
    copymap()
    drawdisplay()
    local fname = shotdir..curdate()..".bmp"
    saveimage(1, fname)
    end
  )

addtomenu(shotsmenu, "h", writechoice("HTML"), function()
    scrsave(S, shotdir..curdate()..".html", 0)
    end
  )

addtomenu(shotsmenu, "p", writechoice("phpBB"), function()
    scrsave(S, shotdir..curdate()..".txt", 1)
    end
  )

-- == tech menu ==

techmenu = {}

addtomenu(techmenu, "s", writechoicef(function()
  return "measure fps" end),
  function()
    fps_start = getticks()
    frames = 0
    end
  )

addtomenu(techmenu, "r", writechoice("refresh NotEye script files"),
  function()
    print "Refreshing configuration."
    justrefreshing = true
    dofile(commondir.."generic.lua")
    prepareconsole()
    if gamename ~= "auto" then startbyname(gamename) end
    backwardcompatible()
    prepareconsole()
    justrefreshing = false
    end
  )

if file_exists(gfxdir.."vapors.png") or file_exists(gfxdir.."rltiles.png") then
  addtomenu(techmenu, "t", writechoice("tile test"),
    function()
      dofile(commondir.."tiletest.lua")
      end
    )
  end

addtomenu(techmenu, "c", writechoice("console helper"),
  function()
    conshelp = not conshelp
    end
  )

-- == main menu ==

mainmenu = {}

-- == ok, done ==

function noteyemenu()
  
  if frames then
    localmessage("frames per second = "..frames/frametime)
    frames = nil
    fps_start = nil
    end
  
  clearmoveanimations()
  menuexecute(mainmenu, mainhdr)
  end

-- these menus should be added to the bottom of the main menu,
-- so we call moremenus() after including all files which put
-- something in the menu

function moremenus()
  addtomenu(mainmenu, "g", writechoice("grab a screenshot"),
    function()
      menuexecute(shotsmenu, dfthdr("Select the screenshot format:\n\n\v7Screenshots saved to:\n\ve"..shotdir))
      end,
    gameonly
    )
  
  addtomenu(mainmenu, "d", writechoice("NotEye development functions"),
    function()
      menuexecute(techmenu, dfthdr("NotEye stats:\n\n"..noteyestats().."\n\vaDevelopment functions:"))
      end
    )
    
  addtomenu(mainmenu, "q", writechoice("stop viewing mode"),
    function()
      stoploop = true
      end,
    function() return not viewmode end
    )

  addtomenu(mainmenu, "s", 
    writechoicef(function()
      return "save changes to: \ve"..getgameconfigname() end),
    function()
      savegameconfig()
      end
    )
  
  -- this is executed after adding everything (sounds, keys etc) to the menu
  if usermenus then usermenus() end
  end

aboutNoteye = 
  "Necklace of the Eye "..noteyeversion.."\n"..
  "(C) Zeno Rogue, 2010-2014\n"..
  "\vfhttp://roguetemple.com/z/noteye.php\vp\n\n"..
  "Necklace of the Eye (NotEye for short) is a roguelike framework. It can be\n"..
  "used as a library for developing new roguelikes (or enriching the old ones),\n"..
  "or act as a frontend for console and libtcod roguelikes.\n\n"..
  "NotEye provides lots of fancy graphical modes (classic DOS-like ASCII, tiles, \n"..
  "FPP, TPP, isometric), mouse support, scripting, taking HTML/phpBB screenshots, \n"..
  "fullscreen, smooth movement of sprites, recording and viewing videos, live \n"..
  "streaming, sound, and many more.\n\n"..
  "NotEye is distributed under GNU General Public License version 3 or later.\n"..
  "It comes with absolutely no warranty; see the file COPYING for details.\n\n"..
  "Press \veENTER\vp to return."
  
  
addtomenu(mainmenu, "a", writechoice("about NotEye"),
  function()
    menuexecute({ }, dfthdr(aboutNoteye))
    end
  )
