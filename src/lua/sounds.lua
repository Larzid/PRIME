-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- things related to audio

-- baksoundtrack backs up the soundtrack as reconfigured by user

soundtrackobj = {}

function resetmusic()
  soundtrack = {}
  end

function addmusic(s)
  table.insert(soundtrack, s)
  end

function clonetrack(src, dst)
  dst = {}
  for k,v in pairs(src) do
    dst[k] = v
    end
  end

if baksoundtrack then
  clonetrack(baksoundtrack, soundtrack)

else
  resetmusic()
  addmusic(sounddir.."music1.ogg")
  addmusic(sounddir.."music2.ogg")
  end

mid = 0

function checkmusic()
  if appactive and not musicon() then 
    if volmusic == 0 then return end

    if #soundtrack == 0 then
      return
      end
    local s = soundtrack[1 + mid % #soundtrack]
    mid = mid + 1
    if not soundtrackobj[s] then
      soundtrackobj[s] = loadmusic(s)
      end
    local obj = soundtrackobj[s]
    if not obj then
      return
      end

    musicvolume(volmusic)
    playmusic(obj)
    end
  end

function disablemusic()
  if not forcemusic then
    resetmusic()
    musicvolume(0)
    havemusic = false
    end
  end

-- loading and garbage-collection music 

Musics = {}
MusicTime = {}

function freeoldmusic(maximum)
  local maxt = os.clock()
  local q = 0
  for n,t in pairs(MusicTime) do
    if t < maxt then maxt = t end
    q = q + 1
    end
  for n,t in pairs(MusicTime) do
    if t == maxt and q > maximum then 
      print("freeing: "..n)
      delete(Musics[n])
      Musics[n] = nil
      MusicTime[n] = nil
      end
    end
  end

function loadmusic2(filename)
  if not Musics[filename] then
    local m = loadmusic(musicdir..filename)
    if m > 0 then Musics[filename] = m end
    end
  MusicTime[filename] = os.clock()
  end

arrowstochange = {"You can also increase and decrease the volume with \vearrow keys\vp."}

function musicarrows(ev, val)
  volmusic = tonumber(val) or volmusic
  musicvolume(volmusic)
  return msarrows(ev, val)
  end

function soundarrows(ev, val)
  volsound = tonumber(val) or volsound
  if ev.type == evKeyUp then samplesound() end
  return msarrows(ev, val)
  end

function msarrows(ev, val)
  if ev.type == evKeyDown then
    local xval = tonumber(val) or 64
    if ev.symbol == SDLK_RIGHT or ev.chr == av("+") or ev.chr == av("=") or symbol == SDLK_KP6 then
      return math.min(xval+1, 128)
    elseif ev.symbol == SDLK_LEFT or ev.chr == av("-") or symbol == SDLK_KP4 then
      return math.max(xval-1, 0)
    elseif ev.symbol == SDLK_UP or symbol == SDLK_KP8 then
      return math.min(xval+8, 128)
    elseif ev.symbol == SDLK_DOWN or symbol == SDLK_KP2 then
      return math.max(xval-8, 0)
    elseif ev.symbol == SDLK_PAGEDOWN or symbol == SDLK_KP3 then
      return math.min(xval+32, 128)
    elseif ev.symbol == SDLK_PAGEUP or symbol == SDLK_KP9 then
      return math.max(xval-32, 0)
    elseif ev.symbol == SDLK_HOME or symbol == SDLK_KP7 then
      return 0
    elseif ev.symbol == SDLK_END or symbol == SDLK_KP1 then
      return 128
      end
    end
  end

function samplesound()
  end

addtomenu(mainmenu, "v", writechoicef(function()
    return "configure audio (sound volume \ve"..volsound.."\vp, music volume \ve"..volmusic.."\vp)" end),
  function()
    ret = askstr("" .. volsound, "enter the sound volume (1-128, or 0 to turn off):", arrowstochange, soundarrows)
    if tonumber(ret) then volsound = tonumber(ret)
      if volsound < 0 then volsound = 0 end
      if volsound > 128 then volsound = 128 end
      end
    ret = askstr("" .. volmusic, "enter the music volume (1-128, or 0 to turn off):", arrowstochange, musicarrows)
    if tonumber(ret) then volmusic = tonumber(ret)
      if volmusic < 0 then volmusic = 0 end
      if volmusic > 128 then volmusic = 128 end
      end
    pickvolume(volsound, volmusic)
    return 1
    end,
  function() return not havemusic end
  )

function pickvolume(vs, vm)
  cfgscripts.volume = function(file)
    file:write("pickvolume("..volsound..","..volmusic..")\n")
    end
  volsound = vs volmusic = vm
  musicvolume(volmusic)
  if volmusic == 0 then musichalt() end
  end

function stopmusic()
  fadeoutmusic(1000)
  end

Sounds = {}
SoundsTime = {}

function playsoundfromfile(filename, relvol)
  local s = Sounds[filename]
  if not s then
    s = loadsound(filename)
    if s > 0 then Sounds[filename] = s 
    else
      print("warning: sound file could not be loaded: "..filename)
    end
    end
  if s then
    SoundsTime[filename] = os.clock()
    playsound(s, (volsound * relvol + 66) / 100)
    end
  end
