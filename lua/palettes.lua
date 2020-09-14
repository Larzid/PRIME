-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

palettesmenu = {}

palettes = {}

function setpalette(p, power)
  if power and power == 7 and palpower and palpower > power then return end
  for i=0,15 do
    vgaset(i, p[i])
    end
  curpalette = p
  
  if S then
    ncs = scrgetsize(S)
    scrcopy(S, 0, 0, S, 0, 0, ncs.x, ncs.y, switchpal)
    end
  end

hex1000000 = tonumber("1000000", 16)
hexFF000000 = tonumber("FF000000", 16)

function switchpalc(C)
  return vgaget(bAND(C, hexFF000000) / hex1000000)
  end

function switchpal(T)
  local O = getobjectinfo(T)
  if O.type == 33 then
    return tilecol(O.t1, switchpalc(O.color), O.mode)

  elseif O.type == 32 then
    return tilealpha(switchpalc(O.color), O.alpha)

  elseif O.type == 24 or O.type == 18 then
    return (O.type == 24 and tilemergeover or tilemerge)(switchpal(O.t1), switchpal(O.t2))
  
  else return T
    end
  end

function addpalette(key, dkey, desc, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15)
  palettes[dkey] = {c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15}
  palettes[dkey][0] = c0
  palettes[dkey].name = dkey
  palettes[dkey].desc = desc
  addtomenu(palettesmenu,
    key,
    writechoice(desc),
    function() pickpalette(palettes[dkey]) return true end
    )
  end

function pickpalette(P)
  cfgscripts.palette = function(file)
    file:write("pickpalette(palettes."..curpalette.name..")\n")
    end
  setpalette(P)
  palpower = 10 
  end

function hx(s) return tonumber(s, 16) end

addpalette("d", "dos", "DOS palette (EGA/VGA)",
  hx("0000000"), hx("10000aa"), hx("200aa00"), hx("300aaaa"), 
  hx("4aa0000"), hx("5aa00aa"), hx("6aa5500"), hx("7aaaaaa"),
  hx("8555555"), hx("95555ff"), hx("A55ff55"), hx("B55ffff"), 
  hx("Cff5555"), hx("Dff55ff"), hx("Effff55"), hx("Fffffff"))

addpalette("w", "windows", "MS Windows: default palette in the Windows console",
  hx("0000000"), hx("1000080"), hx("2008000"), hx("3008080"),
  hx("4800000"), hx("5800080"), hx("6808000"), hx("7C0C0C0"),
  hx("8808080"), hx("90000FF"), hx("A00FF00"), hx("B00FFFF"),
  hx("CFF0000"), hx("DFF00FF"), hx("EFFFF00"), hx("FFFFFFF"))

addpalette("h", "hydra", "Hydra Slayer: more natural-looking colors",
  hx("0000000"), hx("10F52BA"), hx("23C965A"), hx("3188484"), 
  hx("4841B2D"), hx("5664488"), hx("6964B00"), hx("78C8C8C"), 
  hx("851484F"), hx("971A6D2"), hx("A32CD32"), hx("B7FFFD4"), 
  hx("Cff5555"), hx("DE164D1"), hx("EFFD700"), hx("FC0C0C0"))

addpalette("f", "frozen", "Frozen Depths: cold palette",
  hx("0000000"), hx("10000e1"), hx("200e100"), hx("3009696"),
  hx("4e10000"), hx("5960096"), hx("6A05028"), hx("7C0C0C0"),
  hx("8646464"), hx("95050FF"), hx("A50FF50"), hx("B50FFFF"),
  hx("CFF5050"), hx("DFF50FF"), hx("EFFFF50"), hx("FFFFFFF"))

addpalette("t", "tango", "Tango (from Gnome Terminal)",
  hx("0000000"), hx("13464a3"), hx("24d9a05"), hx("305979a"), 
  hx("4cc0000"), hx("5754f7b"), hx("6c3a000"), hx("7d3d6cf"), 
  hx("8545652"), hx("9729ecf"), hx("a89e234"), hx("b34e2e2"), 
  hx("cef2828"), hx("dac7ea8"), hx("efbe84f"), hx("fededeb"))

addpalette("x", "xterm", "xterm",
  hx("0000000"), hx("11d90ff"), hx("200cd00"), hx("300cdcd"), 
  hx("4cd0000"), hx("5cd00cd"), hx("6cdcd00"), hx("7e4e4e4"), 
  hx("84b4b4b"), hx("94682b3"), hx("a00ff00"), hx("b00ffff"), 
  hx("cff0000"), hx("dff00ff"), hx("effff00"), hx("fffffff"))

addpalette("r", "rxvt", "rxvt",    
  hx("0000000"), hx("10000cd"), hx("200cd00"), hx("300cdcd"), 
  hx("4cd0000"), hx("5cd00cd"), hx("6cdcd00"), hx("7f9ebd6"), 
  hx("83f3f3f"), hx("90000ff"), hx("a00ff00"), hx("b00ffff"), 
  hx("cff0000"), hx("dff00ff"), hx("effff00"), hx("fffffff"))

addpalette("m", "matrix", "matrix-like palette",
  hx("0000000"), hx("1001100"), hx("2002200"), hx("3003300"), 
  hx("4004400"), hx("5005500"), hx("6006600"), hx("7007700"), 
  hx("8008800"), hx("9009900"), hx("a00aa00"), hx("b00bb00"), 
  hx("c00cc00"), hx("d00dd00"), hx("e00ee00"), hx("f00ff00"))

addpalette("b", "black", "black-on-white",
  hx("0FFFFFF"), hx("10000AA"), hx("200AA00"), hx("300AAAA"), 
  hx("4AA0000"), hx("5AA00AA"), hx("6AA5500"), hx("7AAAAAA"), 
  hx("8555555"), hx("95555FF"), hx("A55FF55"), hx("B55FFFF"), 
  hx("CFF5555"), hx("DFF55FF"), hx("EFFFF55"), hx("F000000"))

addpalette("u", "user", "user-defined palette",
  hx("0000000"), hx("10000aa"), hx("200aa00"), hx("300aaaa"),
  hx("4aa0000"), hx("5aa00aa"), hx("6aa5500"), hx("7aaaaaa"),
  hx("8555555"), hx("95555ff"), hx("A55ff55"), hx("B55ffff"),
  hx("Cff5555"), hx("Dff55ff"), hx("Effff55"), hx("Fffffff"))


function palettehdr()
  menuy = 7

  scrwrite(IMG, 1, 1, "Current palette: \ve"..curpalette.desc.."\vp", Font, vgaget(10))
  
  for x = 0, 7 do
    scrwrite(IMG, 1+3*x, 3, x, Font, vgaget(15))
    -- scrwrite(IMG, 1+3*x, 4, string.char(219, 219, 219), Font, vgaget(x))
    -- scrwrite(IMG, 1+3*x, 5, string.char(219, 219, 219), Font, vgaget(x+8))
    end

  for x = 0, 7 do for ax = 1,3 do
    scrset(IMG, ax+3*x, 4, tilefill(vgaget(x)))
    scrset(IMG, ax+3*x, 5, tilefill(vgaget(x+8)))
    end end
  
  scrwrite(IMG, 26, 4, "to edit the palette, press \ve0-7\vp, ", Font, vgaget(7))
  scrwrite(IMG, 26, 5, "then enter the new hex values", Font, vgaget(7))
  end

for k=0, 7 do
  addtomenu(palettesmenu,
    ""..k, silent,
    function() 
      editcolor(k)
      editcolor(k+8)
      return true end
    )
  end

function editcolor(cno)
  for k=0,15 do palettes.user[k] = curpalette[k] end
  hex = string.format("%06X", bAND(palettes.user[cno], hexFFFFFF))
  hex = askstr(hex, "Enter the hex value for color "..cno..":")
  if not hex then return end
  hex = tonumber(hex, 16)
  if hex then
    dec = bAND(hex, hexFFFFFF)
    cfgscripts.userpalette = saveuserpalette
    palettes.user[cno] = bOR(bAND(palettes.user[cno], hexFF000000), dec)
    pickpalette(palettes.user)
    end
  end

function saveuserpalette(file)
  file:write("pickuserpalette({")
  for k=0,15 do 
    file:write(string.format("hx(\"%07X\")", palettes.user[k]))
    if k<15 then file:write(",") end
    end;
  file:write("})\n")
  end

function pickuserpalette(tab)
  cfgscripts.userpalette = saveuserpalette
  for k=0,15 do palettes.user[k] = tab[k+1] end
  if curpalette == palettes.user then
    pickpalette(palettes.user)
    end
  end

addtomenu(graphmenu, "k", writechoice("select NotEye's palette"),
  function()
    menuexecute(palettesmenu, palettehdr)
    end
  )

