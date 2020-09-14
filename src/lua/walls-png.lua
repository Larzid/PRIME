-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

if WallsPNG == nil then

  WallsPNG = loadimage(gfxdir.."walls.png")
  
  WT = {}
  
  for y=0,12 do WT[y] = {} end
  
  for y=0,12 do 
    for x = 0,4 do
      WT[y][x] = addtile(WallsPNG, x*32, y*32, 32, 32, -1)
      end
    end
  end

