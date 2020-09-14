-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- contains functions for running the game

-- the main loop

appactive = true
mouse_inside = true

if not justrefreshing then
  function noteyesleep()
    handleviewspeed()
    sleep(1)
    end
  end

-- scheduling drawing frames, and other scheduled tasks

maxframeactive = 15
maxframepassive = 15
lastframe = 0

scheduledTasks = {}

function scheduleTask(time, action)
  local t = getticks() + time
  local old = scheduledTasks[t]
  if old then 
    scheduledTasks[t] = function() old() action() end
  else
    scheduledTasks[t] = action
    end
  end

function checkframe()
  if lastframe and getticks() < lastframe + (isactive and 1000/maxframeactive or 1000/maxframepassive) then
    return false
    end
  lastframe = getticks()

  for k,v in pairs(scheduledTasks) do
    if k < lastframe then
      scheduledTasks[k] = nil
      v()
      end
    end

  return true
  end

-- event handlers and auxiliary functions; keys and mouse are handled later

evhandlers = {}

evhandlers[0] = function(ev)
  if VIEW and viewpause <= 0 then
    if neof(VIEW) then
      delete(VIEW)
      VIEW = nil
      finishviewer()
      localmessage("recording finished")
    else
      readdata(VIEW, true, true)
      end
  elseif checkstreams() then
  else
    if checkframe() then
      isactive = false
      copymap()
      drawandrefresh()
      checkmusic()
      end
    noteyesleep()
    end
  end

evhandlers[evKeyUp] = handlekeyevent
evhandlers[evKeyDown] = handlekeyevent
evhandlers[evMouseUp] = function(ev) sendmouse(ev) end
evhandlers[evMouseDown] = function(ev) sendmouse(ev) end
evhandlers[evMouseMotion] = function(ev) sendmouse(ev) end

evhandlers[evBell] = function(ev)
  if not SndBell then
    SndBell  = loadsound(sounddir.."rune.ogg")
    end
  playsound(SndBell, volsound)
  end

evhandlers[evQuit] = function(ev)
  if VIEW then
    delete(VIEW)
    VIEW = nil
    end
  if SERVER then
    delete(SERVER)
    SERVER = nil
    end
  sendquit()
  end

evhandlers[evActive] = function(ev)
  if bAND(ev.state, SDL_APPINPUTFOCUS + SDL_APPACTIVE) > 0 then
    appactive = (ev.gain == 1)
    if not appactive then stopmusic() end

  elseif bAND(ev.state, SDL_APPMOUSEFOCUS) > 0 then
    if ev.gain == 0 then
      mouse_inside = false
      mousepos = nil
    else
      mouse_inside = true
      end
    end
  end

evhandlers[evProcScreen] = function(ev)
  isactive = true
  if P == LUAPROCESS then setcursor(luag.cursor)
  elseif P then setcursor(proccur(P)) end
  copymap()
  broadcast(sendscreen)
  broadcast(nflush)
  end

evhandlers[evProcQuit] = function(ev)
  EXIT = ev.exitcode
  stoploop = true
  return 0
  end

evhandlers[evResize] = resizewindow

function mainloopcyc()
  inmenu = nil
  local ev = getevent()
  
  local handler = evhandlers[ev.type]
  
  return (handler and handler(ev)) or 1
  end

function mainloop()
  while mainloopcyc() > 0 and not stoploop do
    end
  end

-- == key handling ==

function handlekeyevent(ev)

  if ev.type == evKeyUp and (ev.symbol == SDLK_LCTRL or ev.symbol == SDLK_RCTRL) then
    ctrlzoomingactive = false
    scrollmanual = false
    end

  if ismenukey(ev) then
    ev.type = evKeyUp
    ev.symbol = SDLK_LCTRL
    nsendkey(ev)
    noteyemenu()
    drawandrefresh()
  elseif ev.type == evKeyDown and ev.symbol == SDLK_F2 and quickshots then
    local fname = shotdir..curdate()..".bmp"
    saveimage(1, fname)
  elseif isfullscreenkey(ev) then
    applyfullscreenkey(ev)
    drawandrefresh()
  elseif modeconfigactive then
    userremap(ev)
    handlemodeconfig(ev)
  else
    userremap(ev)
    sendrotated(ev)
    end
  end

-- == mouse handling ==

-- minimovable == 1: minimap is movable, but is not moving currently
-- minimovable == 2: minimap is moving currently
-- minimovable == 3: we are centering the big map based on the minimap
minimovable = 1

minimoved = false

function pickminipos(x, y)
  cfgscripts.minipos = function(file)
    file:write("pickminipos("..x..","..y..")\n")
    end
  minimoved = true
  D.cmini = {top = V(x,y)}
  end

function minileftclick(ev)
  V.be(ev)
  if inrect(ev, cmini) then
    local v = minipixeltoprocess(ev)
    if v then 
      if mode == modeFPP then
        fpplookat = v 
      else
        mapcenter = v 
        scrolltoplayer = false
        end
      minimovable = 3
      end
    end
  end

function moveminimap(ev)

  if mapon and cmini and (minimovable==1 or minimovable == 4) and ev.type == evMouseDown then
    if cmini and cmini.top and cmini.bot and inrect(ev, cmini) then
      if ev.button == 4 or ev.button == 5 then
        local d = ev.button == 4 and 1 or (-1)
        if getkeystate(SDLK_LCTRL)+getkeystate(SDLK_RCTRL) ~= 0 then
          minimapsize.x = minimapsize.x+d
          D.cmini = nil
          if getkeystate(SDLK_LSHIFT)+getkeystate(SDLK_RSHIFT) ~= 0 then
            minimapsize.y = minimapsize.y+d
            end
        elseif getkeystate(SDLK_LSHIFT)+getkeystate(SDLK_RSHIFT) ~= 0 then
          minimapsize.y = minimapsize.y+d
          D.cmini = nil
        else
          local v = minipixeltoprocess(ev)
          if v then
            wheelmovement(ev, v, vectodir(v-playerpos-0.5, 0.8))
            minimovable = 4
            end
          end
      elseif ev.button == 2 and minimode ~= 0 and not midclicksavedmode then
        midclicksavedmode = mode
        mode = minimode
        if mode == modeBlock then mode = modeMini end
        minimode = 0
      elseif ev.button == 3 then
        if minimoved then
          minimoved = false
          cmini = nil
          cfgscripts.minipos = nil
        else
          minimovable = 2
          minimoved = true
          maprel = ev-cmini.top
          end
      elseif ev.button == 1 then
        minileftclick(ev)
        end
      return true
      end

  elseif ev.type == evMouseUp and midclicksavedmode and ev.button == 2 then
    if mode == modeMini then mode = modeBlock end
    minimode = mode
    mode = midclicksavedmode
    midclicksavedmode = nil
    return true
  
  elseif minimovable == 4 and ev.type == evMouseMotion then
    if not inrect(ev, cmini) then
      minimovable = 1
      end
    return true
  
  elseif minimovable == 2 then
    local mp = ev - maprel
    pickminipos(mp.x, mp.y)
    if ev.type == evMouseUp then minimovable = 1 end
    return true

  elseif minimovable == 3 then
    minileftclick(ev)
    if ev.type == evMouseUp then 
      minimovable = 1 
      fpplookat = nil
      end
    return true
    end

  return false
  end

-- actually also called on ev.button == 1

function wheelmovement(ev, v, d)

  if not d then return end  

  if ev.button == 4 or ev.button == 5 then
    if moveonwheel then
      moveonwheel = false
    else
      return
      end
    end
  
  if ev.button == 4 and d ~= -1 then
    d = (d+4) % 8
    end
  
  if d ~= -1 then
    ev.type = evKeyDown
    ev.mod = 0
    assignkey(ev, dirkeys[d])
    sendrotated(ev)
  else
    ev.type = evKeyDown
    ev.symbol = av(".")
    ev.chr = av(".")
    ev.mod = 0
    nsendkey(ev)
    end
  end

function sendmouse(ev)
  if ev.type == evMouseMotion then
    scrollmanual = true
    end

  mousepos = V.be(ev)
  local d

  if moveminimap(ev) then
    return
    end
  
  if ev.type == evMouseMotion then
    lastmm = ev
    return
    end
  
  if ev.type == evMouseUp then
    return
    end

  if ev.button == 4 and getkeystate(SDLK_LCTRL) + getkeystate(SDLK_RCTRL) > 0 then
    if tilesize.x < maparea.size.y/3 then zoomtiles(2, ev) end
    ctrlzoomingactive = true
    scrolltoplayer = false
    return
    end
  
  if ev.button == 5 and getkeystate(SDLK_LCTRL) + getkeystate(SDLK_RCTRL) > 0 then
    if not (tilesize * map.size < maparea.size) then zoomtiles(.5, ev) end
    ctrlzoomingactive = true
    scrolltoplayer = false
    return
    end

  wheelmovement(ev, pixeltoprocess(ev), pixeltodir(ev))
  end

-- == start the game process ==

function caller(name, dir, uname, wname)
  if manualcall and linux then
    return "sh -l"
  elseif manualcall then
    return "cmd"
  elseif linux then
    return "sh "..noteyedir.."caller.sh "..dir.." "..uname.." \""..name.."\""
  else
    return ".\\caller.bat "..dir.." "..wname.." \""..name.."\""
    end
  end

function caller3(name, x)
  return caller(name, x, x, x)
  end

function rungamex(cmdline)
  P = newprocess(S, Font, cmdline)
  end

function rungame2(cmdline)

  backwardcompatible()
  prepareconsole()
  stoploop = false
  pcursor = V(0,0)
  
  autostreaming()
  
  if not viewmode then
    rungamex(cmdline)
    end

  mainloop()
  
  stopstreaming()

  if havemain then createmain() end
  if P == LUAPROCESS then P = nil end
  if P then delete(P); P = nil end
  end

uisleep_replace = uisleep

function rungame(cmdline)

  loadgameconfig()
  if justrefreshing then
    return
    end
  
  if luagame and not viewmode then
  
    function rungamex(cmdline)
      P = LUAPROCESS
      luagame()
      end
    
    rungame2(cmdline)

  elseif threaded and not viewmode then

    local lnoteyesleep = noteyesleep
    
    function noteyesleep()
      uisleep_replace()
      lnoteyesleep()
      end
  
    function rungamex(cmdline)

      P = internal(S, Font, cmdline)
      end
    
    function rg()
      rungame2(cmdline)
      end
  
    uicreate(rg)
  
  else
    rungame2(cmdline)
    end
  end

function sendquit()
  -- for libtcod as libnoteye user (not officially released)
  if vialibtcod then
    quitlibtcod()
    end
  end

addtomenu(mainmenu, "c", writechoicef(function()
    return "limit the framerate to save CPU power (\ve"..maxframeactive.."\vp fps)" end),
  function()
    ret = askstr("" .. maxframeactive, "enter the max frame rate:")
    if tonumber(ret) then maxframeactive = tonumber(ret) end
    if maxframeactive <= 0 then maxframeactive = 999999 end
    pickframerate(maxframeactive)
    return 1
    end
  )

function pickframerate(framerate)
  cfgscripts.framerate = function(file)
    file:write("pickframerate("..framerate..")\n")
    end
  maxframeactive = framerate
  maxframepassive = framerate
  end

-- for Lua games:

-- if P equals LUAPROCESS, then the game is written in Lua

LUAPROCESS = -1

luag = {kq = {}, kqfirst = 0, kqlast = 0, cursor = V(11,11)}

luag.dxy = { V(1,-1), V(0,-1), V(-1,-1), V(-1,0), V(-1,1), V(0,1), V(1,1), V(0,0) }
luag.dxy[0] = V(1,0)

luag.symboltodxy = {}

for c=0,7 do
  luag.symboltodxy[keytabs.numpad[c]] = luag.dxy[c]
  luag.symboltodxy[keytabs.arrow[c]] = luag.dxy[c]
  end

luag.symboltodxy[keytabs.numpad[8]] = luag.dxy[8]

function luag.refresh()
  evhandlers[evProcScreen]()
  mainloopcyc()
  end

function luag.sendkey(ev)
  luag.kq[luag.kqlast] = ev
  luag.kqlast = luag.kqlast+1
  end

function luag.getch(releases)
  if luag.kqfirst < luag.kqlast then
    local ret = luag.kq[luag.kqfirst]
    luag.kq[luag.kqfirst] = nil
    luag.kqfirst = luag.kqfirst+1
    if ret.type == 2 and not releases then return end
    if ret.chr then ret.char = string.char(ret.chr) end
    ret.dxy = luag.symboltodxy[ret.symbol]
    return ret
    end
  end
