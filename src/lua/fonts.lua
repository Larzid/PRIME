-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- things related to fonts

if not Fonts then Fonts = {} end

function loadfont(fname, x, y, adjuster)
  if not Fonts[fname] then
    local F = {}
    local P = loadimage(gfxdir..fname..".png")
    if P == 0 then
      print("font not found: "..fname..", replacing by the default font")
      -- replace with 
      P = loadimage(gfxdir.."cp437-8.png")
      x = 16
      y = 16
      end
    local Psize = imggetsize(P)
    F.png = P
    
    F.font = adjuster(P,x,y,Psize.x,Psize.y)
    
    F.alias = imagealias(F.png, fname..".png")
    F.size = V.be({x=Psize.x/x, y=Psize.y/y, rx=x, ry=y, sx=1, sy=1})
    F.name = fname
    F.adjuster = adjuster
    F.adjustername = getnameintable(adjuster, fontadj)
    Fonts[fname] = F
    end
  return fname
  end

function selectfont(fname)
  curfont = fname
  FontData = Fonts[fname]
  Font = FontData.font
  StreamSetFont = 2
  end

fontadj = {}

hex1000000 = tonumber("1000000", 16)
hex1000001 = tonumber("1000001", 16)
hexFF0000FF = tonumber("FF0000FF", 16)

function fontadj.simple(P,fx,fy,sx,sy)

  for x=0,sx-1 do
  for y=0,sy-1 do
    local c = getpixel(P,x,y)
    -- c = colorset(c, 3, colorpart(c, 0))
    c = hex1000001 * colorpart(c, 0)
    setpixel(P, x,y, c)
    end end

  return newfont(P, fx, fy, 0)
  end

function fontadj.simpleA(P,fx,fy,sx,sy)

  for x=0,sx-1 do
  for y=0,sy-1 do
    local c = getpixel(P,x,y)
    -- c = colorset(c, 3, colorpart(c, 0))
    c = hex1000000 * colorpart(c, 0) + 255
    setpixel(P, x,y, c)
    end end

  return newfont(P, fx, fy, transAlpha)
  end

function fontadj.alpha(P,fx,fy,sx,sy)
  return newfont(P,fx,fy,transAlpha)
  end

function fontadj.brogue(P,fx,fy,sx,sy)
  for x=0,sx-1 do
  for y=0,sy-1 do
    pix = getpixel(P, x,y)
    if bAND(pix, 255) > 192 then pix = bAND(pix, hexFF0000FF)
    else pix = 0
    end
    setpixel(P, x, y, pix)
    end end
  return newfont(P,fx,fy,transAlpha)
  end

function selectfontfull(fname)
  if fname then
    selectfont(fname)
    -- {x = Fonts[fname].size.x*2, y = Fonts[fname].size.y*2 }
    prepareconsole()
    if P then setfont(P, Font)
    elseif S then setfont(S, Font) end
    end
  end

-- == fonts menu ==

fontsmenu = {}

function addFont(key,name,a,b,c,d,menu)
  addtomenu(menu or fontsubmenu,
    key,
    writechoice(name),
    function () 
      pickfont(a,b,c,d)
      return true
      end
    )
  end

resolutions = {
  {fx=1, fy=1, name="original"},
  {fx=2, fy=2, name="double size"},
  {fx=1.5, fy=1.5, name="3/2 size"},
  {fx=0.5, fy=0.5, name="half size"},
  {fx=1, fy=0.5, name="half height"},
  {fx=2, fy=1, name="double width"}
  }

fsmenu = {}

resmenu = {}

fontsubmenu = {}

function addfontsize(key, name, res)
  addtomenu(fsmenu,
    key,
    writechoice(name),
    function () 
      scalechoice(res)
      end
    )
  end

function addscreenres(key, name, res)
  addtomenu(resmenu,
    key,
    writechoice(name),
    function () 
      pickresolution({s = res})
      end
    )
  end

addfontsize("o", "original size", V(1  , 1  ))
addfontsize("d", "double size",   V(2  , 2  ))
addfontsize("h", "half size",     V(0.5, 0.5))
addfontsize("s", "3/2 size",      V(1.5, 1.5))
addfontsize("w", "double width",  V(2  , 1  ))
addfontsize("a", "half height",   V(1  , 0.5))
addfontsize("e", "half width",    V(0.5, 1  ))
addfontsize("t", "double height", V(1  , 2  ))

addscreenres("0", "320x200",   V(320,  200 ))
addscreenres("1", "640x400",   V(640,  400 ))
addscreenres("2", "800x500",   V(800,  500 ))
addscreenres("3", "1024x600",  V(1024, 600 ))
addscreenres("4", "1280x800",  V(1280, 800 ))
addscreenres("5", "1440x900",  V(1440, 900 ))
addscreenres("6", "1600x1200", V(1600, 1200))
addscreenres("7", "1920x1080", V(1920, 1080))

function resizewindow(ev)
  if ev.sx == xscreen.size.x and ev.sy == xscreen.size.y then return end
  local ev2 = {s = V(ev.sx, ev.sy)}
  pickresolution(ev2)
  end

--reqres = {sx=1024, sy=768}

addtomenu(fontsmenu, "m", writechoicef(
  function() 
    if fscrmode then 
      D.reqres.sfull = V.be(origvideomode())
      return "current fullscreen mode: \vekeep the original resolution ("..xxy(reqres.sfull)..")"
    else return "current fullscreen mode: \vechange to the window size"
    end
    end),
  function() 
    pickfull(fscr, not fscrmode)
    viewmenu()
    fontreshdr()
    return true
    end
  )

addtomenu(fontsmenu, "e", writechoice("switch fullscreen (also \veAlt+Enter\vp)"),
  function() 
    pickfull(not fscr, fscrmode) 
    viewmenu()
    fontreshdr()
    return true
    end
  )

addtomenu(fontsmenu,
  "f", 
  writechoice("Select the font"),
  function()                   
    menuexecute(fontsubmenu, 
      dfthdr("Select the font\n\n")
      )
    return true
    end
  )

addtomenu(fontsmenu,
  "s", 
  writechoice("Select font scaling"),
  function()                   
    function scalechoice(v) 
      pickresolution({f = v}) 
      end
    menuexecute(fsmenu,
      dfthdr("Select font scaling\n\n"..
        "Select the scale of the font, and NotEye will automatically adjust the \n"..
        "size of the window (or full screen resolution) to fit everything."
        )
      )
    return true
    end
  )

guiscalingavailable = function() return gamename == "auto" end

addtomenu(fontsmenu,
  "h", 
  writechoice("Select HUD font scaling"),
  function()
    function scalechoice(v) pickguiscaling(v) end
    menuexecute(fsmenu, 
      dfthdr("Select HUD font scaling\n\n"..
        "Scale of the font in the HUD. Works only in some games.\n"..
        "Note that menus always try to use full screen.\n"
        )
      )
    return true
    end,
  guiscalingavailable
  )

addtomenu(fontsmenu,
  "w", 
  writechoicef(
    function()
      return fscr and "Select resolution" or "Select the window size"
      end),
  function()
    menuexecute(resmenu, 
      dfthdr("Select the window size\n\n"..
        "You can request a specific resolution here.\n"..
        "The font will be scaled to fill the whole window.\n"
        )
      )
    return true
    end
  )

addFont("q", "VGA font 8x16 (DOS default)", "cp437-16", 32, 8, fontadj.simple)
addFont("a", "VGA font 8x14", "cp437-14", 32, 8, fontadj.simple)
addFont("z", "VGA font 8x8", "cp437-8", 32, 8, fontadj.simple)
addFont("f", "Fantasy font 16x32", "fantasy-32", 32, 8, fontadj.simpleA)
addFont("e", "DejaVu Mono 10x20", "dejavu-20", 32, 8, fontadj.simpleA)
addFont("d", "DejaVu Mono 8x16", "dejavu-16", 32, 8, fontadj.simpleA)
addFont("c", "Courier 8x16", "courier-16", 32, 8, fontadj.simpleA)
addFont("t", "Tiny Font 3x5", "tinyfont", 32, 8, fontadj.simpleA)
addFont("y", "Square 8", "square-8", 16, 16, fontadj.simpleA)
addFont("u", "Square 14", "square-14", 16, 16, fontadj.simpleA)
addFont("h", "Square 16", "square-16", 16, 16, fontadj.simpleA)
addFont("j", "Square 26", "square-26", 16, 16, fontadj.simpleA)
addFont("i", "SDS_6x6", "SDS_6x6", 16, 16, fontadj.simpleA)
addFont("k", "SDS_8x8", "SDS_8x8", 16, 16, fontadj.simpleA)

if havebrogue then for u=1,5 do
  addFont(""..u, "Brogue font #"..u,
    "BrogueFont"..u, 16, 16, fontadj.brogue
    )
  end end

function fontreshdr()
  menuy = 6
  title = "Select your font and resolution"

  scrwrite(IMG, 1, 1, title, Font, vgaget(10))
  
  local scrres = 
    fscr and 
      (
       (fscrmode and "Using the original full screen resolution, will return to:") or
      "Current screen resolution:" 
      ) or "Current window size:"
  
  scrwrite(IMG, 1, 3, scrres, Font, vgaget(7))
  
  if reqres.s then
    scrres = xxy(reqres.s).." (specific size requested)"
  elseif reqres.f then
    local winsize = reqres.f * conssize * FontData.size
    scrres = xxy(winsize).." (based on the "..xxy(FontData.size).." font, scaled "..xxy(reqres.f)..", "..xxy(conssize).." characters)"
  else 
    scrres = "?"
    end
  
  scrwrite(IMG, 1, 4, "\vf"..scrres, Font, vgaget(7))
  end


addtomenu(mainmenu, "r", writechoice("select font, resolution, and fullscreen mode"),
  function()
    menuexecute(fontsmenu, fontreshdr)
    end
  )

function pickresolution(res)
  cfgscripts.res = function(file)
    fontline = "pickresolution({"
    if reqres.s then fontline = fontline.."s=V("..reqres.s.x..","..reqres.s.y..")," end
    if reqres.f then fontline = fontline.."f=V("..reqres.f.x..","..reqres.f.y..")," end
    -- for k,v in pairs(reqres) do fontline = fontline..k.."="..v.."," end
    fontline = fontline.."cfg=1})\n"
    file:write(fontline)
    end
  reqres = res
  prepareconsole()
  end

function pickguiscaling(res)
  cfgscripts.guiscaling = function(file)
    file:write("pickguiscaling(V("..res.x..","..res.y.."))")
    end
  D.guiscaling = res
  prepareconsole()
  end

function pickfont(a, b, c, d)
  cfgscripts.font = function(file)
    local F = Fonts[curfont]
    fontline = "pickfont(\""..F.name.."\", "..F.size.rx..", "..F.size.ry
    fontline = fontline..", fontadj."..F.adjustername
    fontline = fontline..")\n"
    file:write(fontline)
    end
  selectfontfull(loadfont(a,b,c,d))
  end

function pickreserve(v)
  cfgscripts.reserve = function(file)
    file:write("pickreserve("..v..")\n")
    end
  msgreserve = v
  --setvideomodex()
  prepareconsole()
  end

addtomenu(fontsmenu, "r", writechoicef(
  function() 
    return "reserve \ve"..msgreserve.."\vp lines below the screen for the messages and mini-map"
    end ),
  function()
    pickreserve(asknum(msgreserve, "number of lines below the screen", msgreserve))
    return true
    end
  )

