-- which version of NotEye is this noe-file compatible with
gamenoteyever = 0x750

minipos = {x=1, y=0.1}
mapregion = {x0=0, y0=2, x1=80, y1=22}

if not noteyedir then
  noteyedir = (os.getenv ("NOTEYEDIR") or ".").."/"
  end

if not noteyeloaded then
  --game_to_launch = "prime"
  dofile (noteyedir.."lua/noteye.lua")
  end

dofile(commondir.."generic-tile.lua")

function xtileaux2(ch, co, av, ba)

  if ch == "/" and (co == vgaget(6) or co == vgaget(4)) then
    return tspatc(rb, spFlat + spFloor + spCeil + spIFloor, ch)
  elseif ch == "+" and (co == vgaget(6) or co == vgaget(4)) then
    return tspatm(wallcol(co), rb, ssWall, ch)
  elseif ch == "=" then
    return lavacol(co)
  elseif ch == "%" then
    return tspatc(rb, spFloor + spIItem, ch)
  elseif ch == "0" or ch == "!" then
    return tspat(rb, ssMon, ch)
    end
  end

disablemusic()

dirkeys = keytabs.arrow

defaultmodep(modeASCII, 6)
setwindowtitle("Doom: the Roguelike")
--rungame(caller3("DoomRL", "doomrl"))
rungame("whatever")
