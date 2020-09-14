-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- various basic utilities

function mround(x)
  return math.floor(x + 0.5)
  end

function renewscreen(S, x, y)
  if S then
    local oldsize = scrgetsize(S)
    if oldsize.x ~= x or oldsize.y ~= y then
      scrsetsize(S, x, y)
      end
    return S
  else
    S = newscreen(x, y)
    return S
    end
  end

function file_exists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

function file_copy(srcname, dstname)
  local srcf = io.open(srcname, "rb")
  local dstf = io.open(dstname, "wb")
  local size = 8192
  while true do
    local block = srcf:read(size)
    if not block then break end
    dstf:write(block)
    end

  srcf:close()
  dstf:close()
  end

function boolonoff(x)
  return x and "on" or "off"
  end

function boolyesno(x)
  return x and "yes" or "no"
  end

function booltf(x)
  return x and "true" or "false"
  end

function curdate()
  return os.date("%Y-%m-%d-%H-%M-%S")
  end

function ashex(v)
  return string.format("%02X", v)
  end

-- table utilities

function table.copy(t)
  local t2 = {}
  for k,v in pairs(t) do
    t2[k] = v
  end
  return t2
end

function table.merge(t2, t)
  for k,v in pairs(t) do
    t2[k] = v
  end
  return t2
end

-- ascii value

function av(x)
  return string.byte(x,1)
  end

function avf(x)
  return string.byte(x,1)
  end

-- identity for scrcopy

function eq(C)
  return C
  end

-- various cell transformations

function cellshade(x)
  if x == 0 then return 0 end
  return tilemerge(tileshade(gba(x)), gp2(x))
  end

function celltrans(x)
  return gp2(x)
  end

function celltransorshade(x)
  local ba = gba(x)
  if x == 0 or gchv(x) == 0 then
    return x
  elseif ba < 0 or ba == vgaget(0) then
    return gp2(x)
  else
    return tilemerge(tileshade(ba), gp2(x))
    end
  end

hex808080 = tonumber("808080", 16)
hexFFFFFF = tonumber("FFFFFF", 16)
hexC0C0C0 = tonumber("C0C0C0", 16)
hexF000000 = tonumber("F000000", 16)
hexFF000000 = tonumber("FF000000", 16)

function cellblack(x)
  if bAND(gco(x),hexF000000) == 0 then return 0
  else
    return tilecol(gp2(x), hexFF000000, recMult)
    end
  end

function cellbackshade(x)
  if x==0 then return 0 end
  return tileshade(gba(x))
  end

function cellbackshade75(x)
  if x==0 then return 0 end
  return tilealpha(gba(x), hexC0C0C0)
  end

function cellbackfull(x)
  if x==0 then return 0 end
  return tilefill(gba(x))
  end

function cellblacknb(x)
  return tilecol(x, hexFF000000, recMult)
  end

function getlayerX(val)
  return function(x) return getlayer(x, val) end
  end

-- identity matrix
ffid = freeformparam(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)

-- two-sided identity matrix
ffid_0 = freeformparam(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
freeformparamflags(ffid_0, 0, false)

-- profiling

if not profiling then profiling = {} profilingq = {} end

function profstart(x)
  if not profiling[x] then profiling[x] = 0 profilingq[x] = 0 end
  profiling[x] = profiling[x] - os.clock()
  profilingq[x] = profilingq[x] + 1
end

function profend(x)
  profiling[x] = profiling[x] + os.clock()
end

function profinfo()
  logprint("profile results:\n")
  for k,v in pairs(profiling) do
    logprint(k.."="..v.." ("..profilingq[k]..")\n")
    end
  logprint("\n")
  end

-- functions and constants which were once in the NotEye engine,
-- but were removed due to new features

function tilefill(x)
  return tilealpha(x, hexFFFFFF)
  end

function tileshade(x)
  return tilealpha(x, hex808080)
  end

function tilerecolor(x, y)
  return tilecol(x, y, recDefault)
  end

spIWall = spIWallL + spIWallR
spWall = spWallN + spWallE + spWallS + spWallW

function tiletransform(t1, dx,dy, sx,sy)
  return tilexf(t1,dx,dy,sx,sy,0,0)
  end

function tilemerge3(a,b,c)
  return tilemerge(tilemerge(a,b),c)
  end

function tilemerge4(a,b,c,d)
  return tilemerge(tilemerge3(a,b,c),d)
  end

-- config scripts
if not cfgscripts then
  cfgscripts = {}
  end

function getgameconfigname()
  return string.gsub(configfileformat, "GAMENAME", gamename)
  end

function loadgameconfig()
  local s = getgameconfigname()
  if file_exists(s) then
    dofile(s)
    end
  end

function savegameconfig()
  local s = getgameconfigname()
  print("saving the configuration to: "..s)
  
  local file = io.open(s, "w")
  for k,v in pairs(cfgscripts) do
    v(file)
    end
  file:close()
  end

function getnameintable(x, tab)
  for k,v in pairs(tab) do
    --print("check: "..k)
    if x == v then return k
      end
    end
  return nil
  end

function pickinteger(title, value)
  cfgscripts["integer_"..title] = function(file)
    file:write("pickinteger(\""..title.."\","..value..")\n")
    end
  _G[title] = value
  end

function pickbool(title, value)
  cfgscripts["bool_"..title] = function(file)
    file:write("pickbool(\""..title.."\","..booltf(value)..")\n")
    end
  _G[title] = value
  end

function picknumber(title, value)
  cfgscripts["number_"..title] = function(file)
    file:write("picknumber(\""..title.."\","..value..")\n")
    end
  _G[title] = value
  end

function pickstring(title, value)
  cfgscripts["string_"..title] = function(file)
    if value then
      file:write("pickstring(\""..title.."\",\""..value.."\")")
    else
      file:write("pickstring(\""..title.."\", nil)\n")
      end
    end
  _G[title] = value
  end

