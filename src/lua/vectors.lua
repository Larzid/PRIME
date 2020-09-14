-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- vector and rectangle handling

V = {}

V.mt = {}

function V.isvec(a)
  return type(a) == "table" and a.isvec
  end

function V.isrect(a)
  return type(a) == "table" and a.isrect
  end

function vecerror(x)
  -- for debugging: crash NotEye
  -- error(x)
  -- for release: just print the error (sometimes we may survive)
  local f = io.open("veclog.txt", "wt")
  if x then io.write(x) end
  f:write(debug.traceback())
  io.close(f)
  if x then print(x) end
  end

function V.mt.__add(a,b)
  if type(b) == "number" then
    return V(a.x+b, a.y+b)
  elseif not V.isvec(b) then
    vecerror()
    end
  return V(a.x+b.x, a.y+b.y)
  end

function V.mt.__sub(a,b)
  if type(b) == "number" then
    return V(a.x-b, a.y-b)
  elseif not V.isvec(b) then
    vecerror()
    end
  return V(a.x-b.x, a.y-b.y)
  end

function V.mt.__mul(a,b)
  if type(b) == "number" then
    return V(a.x*b, a.y*b)
  elseif not V.isvec(b) then
    print("b = "..tostring(b))
    vecerror()
    end
  return V(a.x*b.x, a.y*b.y)
  end

function V.mt.__div(a,b)
  if type(b) == "number" then
    return V(a.x/b, a.y/b)
  elseif not V.isvec(b) then
    vecerror()
    end
  return V(a.x/b.x, a.y/b.y)
  end

function V.mt.__unm(a)
  return V(-a.x, -a.y)
  end

function V.mt.__eq(a, b)
  return a.x == b.x and a.y == b.y 
  end

function V.mt.__lt(a, b)
  return a.x < b.x and a.y < b.y 
  end

function V.mt.__le(a, b)
  return a.x <= b.x and a.y <= b.y 
  end

function V.mt.__tostring(a)
  return "("..a.x..","..a.y..")"
  end

function V.be(v)
  if type(v) == "number" then return V(v,v) end
  setmetatable(v, V.mt)
  v.isvec = true
  return v
  end

function V.floor(v)
  if not V.isvec(v) then vecerror() end
  return V(math.floor(v.x), math.floor(v.y))
  end

function V.ceil(v)
  if not V.isvec(v) then vecerror() end
  return V(math.ceil(v.x), math.ceil(v.y))
  end

function V.atan2(v)
  if not V.isvec(v) then vecerror() end
  return math.atan2(v.x, v.y)
  end

function V.max(a, b)
  if not V.isvec(a) then vecerror() end
  if not V.isvec(b) then vecerror() end
  return V(math.max(a.x,b.x), math.max(a.y, b.y))
  end
  
function V.min(a, b)
  if not V.isvec(a) then vecerror() end
  if not V.isvec(b) then vecerror() end
  return V(math.min(a.x,b.x), math.min(a.y, b.y))
  end

function V.norm(v)
  return v.x*v.x+v.y*v.y
  end

function V.copy(v)
  return V(v.x, v.y)
  end

V.cmt = {}

function V.cmt.__call(v, x, y)
  return V.be({x = x, y = y})
  end

setmetatable(V, V.cmt)

-- rectangle building

V.rmt = {}

function V.rmt.__tostring(a)
  return "("..tostring(a.top)..","..tostring(a.bot)..")"
  end

function V.rmt.__add(r, v)
  return rectTB(r.top+v, r.bot+v)
  end

function V.rmt.__sub(r, v)
  return rectTB(r.top-v, r.bot-v)
  end

function V.rmt.__mul(r, v)
  return rectTB(r.top*v, r.bot*v)
  end

function V.rmt.__div(r, v)
  return rectTB(r.top/v, r.bot/v)
  end

function rectTB(top, bot)
  local rect = {top = top, bot = bot, ctr = (top+bot)/2, size = (bot-top)}
  setmetatable(rect, V.rmt)
  rect.isrect = true
  return rect
  end

function rectXY(tx, ty, bx, by)
  return rectTB(V(tx,ty), V(bx,by))
  end

function rectS(size)
  return rectTB(V(0,0), size)
  end

function rectTS(top, size)
  return rectTB(top, top+size)
  end

function rectCS(ctr, size)
  return rectTB(ctr-size/2, ctr+size/2)
  end

function inrect(vect, rec)
  if (not vect) or not vect.isvec then
    print("SUSPICIOUS INRECT VEC")
    print(debug.traceback())
    end
  if (not rec) or not rec.isrect then
    print("SUSPICIOUS INRECT REC")
    print(debug.traceback())
    end
  local r = rectTB(V(rec.top.x, rec.top.y), V(rec.bot.x, rec.bot.y))
  local lef = getmetatable(r.top).__le
  local ltf = getmetatable(vect).__lt
  local b0 = lef(r.top, vect)
  local b1 = ltf(vect, rec.bot)
  -- I do not understand why r.top <= vect and vect < rec.bot does not work here...
  return b0 and b1
  end
  
-- vector/rectangle-based versions of NotEye functions

function V.scrgetsize(S)
  return V.be(scrgetsize(S))
  end

function V.updaterect(R)
  updaterect(R.top.x, R.top.y, R.size.x, R.size.y)
  end

function V.fillimage(output, R, color)
  if not V.isrect(R) then
    vecerror()
    end
  if not V.isvec(R.top) then
    vecerror()
    end
  if not V.isvec(R.bot) then
    vecerror()
    end
  fillimage(output, R.top.x, R.top.y, R.size.x, R.size.y, color)
  end

function V.refreshconsole(v, size)
  if not V.isvec(v) then
    vecerror()
    end
  return refreshconsole(v.y, v.x, size)
  end

function V.renewscreen(S, V)
  return renewscreen(S, V.x, V.y)
  end
  
function V.newscreen(V)
  return newscreen(V.x, V.y)
  end

function V.scrfill(S, R, T)
  return scrfill(S, R.top.x, R.top.y, R.size.x, R.size.y, T)
  end

function V.drawscreen(Img, S, at, size)
  return drawscreen(Img, S, at.x, at.y, size.x, size.y)
  end

function V.drawscreenx(Img, S, at, size, f)
  if f == eq then
    return drawscreen(Img, S, at.x, at.y, size.x, size.y)
  else
    return drawscreenx(Img, S, at.x, at.y, size.x, size.y, f)
    end
  end

function V.scrcopy(S1, V1, S2, V2, size, f)
  V1 = V.be(V1)
  V2 = V.be(V2)
  return scrcopy(S1, V1.x, V1.y, S2, V2.x, V2.y, size.x, size.y, f)
  end

function V.scrget(S, V)
  return scrget(S, V.x, V.y)
  end

function V.drawtile(Img, tile, R)
  return drawtile(Img, tile, R.top.x, R.top.y, R.size.x, R.size.y)
  end

function V.cmpcheck(A, B, C, D)
  return V(A.x < B.x and C.x or D.x, A.y < B.y and C.y or D.y)
  end

function V.imggetsize(I)
  return V.be(imggetsize(I))
  end

function V.newimage(v, color)
  return newimage(v.x, v.y, color)
  end

function V.addtile(img, R, back)
  return addtile(img, R.top.x, R.top.y, R.size.x, R.size.y, back)
  end

V.z = V(0,0)
  
-- translate V to direction, for use in mouse support

do local dirtab = {3, 2, 1, 4, 0, 0, 5, 6, 7}

function vectodir(v, mzero)

  max = math.max(math.abs(v.x), math.abs(v.y))
  
  if max*2 > mzero then
    dx = 5
    if v.x > max/2 then dx = dx + 1 end
    if v.x < -max/2 then dx = dx - 1 end
    if v.y > max/2 then dx = dx + 3 end
    if v.y < -max/2 then dx = dx - 3 end
    
    return dirtab[dx]
    end
  return -1
  end
end

V.xy = {x="x", y="y"}

function irect(R, step)
  local x = R.top.x
  local y = R.top.y
  step = step or 1
  return function ()
    if x < R.bot.x then 
      local x1 = x
      x = x + step
      return x1,y
    else 
      x = R.top.x
      if y < R.bot.y then
        local y1 = y
        y = y + step
        return x,y1
        end
      end
    end
  end

function irectv(R, step)
  local x = R.top.x
  local y = R.top.y
  step = step or 1
  return function ()
    if x < R.bot.x then 
      local x1 = x
      x = x + step
      return V(x1,y)
    else 
      x = R.top.x
      if y < R.bot.y then
        local y1 = y
        y = y + step
        return V(x,y1)
        end
      end
    end
  end

function xxy(v) 
  if not V.isvec(v) then vecerror() end
  return v.x.."x"..v.y 
  end

dirtovec = {}
dirtovec[-1] = V(0,0)
dirtovec[0] = V(1,0)
dirtovec[1] = V(1,-1)
dirtovec[2] = V(0,-1)
dirtovec[3] = V(-1,-1)
dirtovec[4] = V(-1, 0)
dirtovec[5] = V(-1,1)
dirtovec[6] = V(0 ,1)
dirtovec[7] = V(1,1)

