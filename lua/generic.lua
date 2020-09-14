-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- this file simply includes all the generic NotEye scripts

print("<NotEye reloaded>")

dofile(commondir.."vectors.lua")
dofile(commondir.."util.lua")
dofile(commondir.."menu.lua")
dofile(commondir.."viewmodes.lua")
dofile(commondir.."fonts.lua")
dofile(commondir.."graphopt.lua")
dofile(commondir.."palettes.lua")
dofile(commondir.."keymap.lua")
dofile(commondir.."sounds.lua")
dofile(commondir.."streams.lua")
dofile(commondir.."running.lua")
dofile(commondir.."readmap.lua")
dofile(commondir.."backward.lua")

moremenus()
