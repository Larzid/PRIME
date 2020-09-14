-- Necklace of the Eye v7.6 roguelike frontend
-- Copyright (C) 2010-2014 Zeno Rogue, see noteye.lua for details

-- this file is supposed to ensure backward compatibility.
-- The function backwardcompatible() tries to fix some outdated things about
-- old NOE files, or at least emit a warning about what should be done.
-- (Currently it does not do much, unfortunately.)
-- Look at the comments at the bottom to read about other changes that might
-- affect your old NOE file.

mapregion = nil

function backwardcompatible()
  if D.minipos then V.be(D.minipos) end
  if mapregion then
    D.map = rectXY(mapregion.x0, mapregion.y0, mapregion.x1, mapregion.y1)
    end
  V.be(D.tilesize)
  end


-- Other changes:

-- NotEye uses vectors (see vectors.lua) instead of {x=..., y=...} tables.
-- If you have {x=XX, y=YY} somewhere, you should probably use V(XX,YY)
-- instead.

-- There are also major changes in the display-related functions.
