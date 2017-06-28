import 'regent'

local A = require 'admiral'

-------------------------------------------------------------------------------
-- Module parameters
-------------------------------------------------------------------------------

return function(cellsRel, particlesRel)

local p_cells   = cellsRel:primPartSymbol()
local cellsType = cellsRel:regionType()
local p_particles   = particlesRel:primPartSymbol()
local particlesType = particlesRel:regionType()
local tiles = A.primColors()

-------------------------------------------------------------------------------
-- Local tasks
-------------------------------------------------------------------------------

local task Render(cells : cellsType, particles : particlesType)
where
  reads(particles.{__valid, cell}, cells.temperature),
  reads writes(particles.particle_temperature)
do
  for p in particles do
    if p.__valid then
      p.particle_temperature = cells[p.cell].temperature
    end
  end
end

-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

exports.Render = rquote
  for tile in tiles do
    Render(p_cells[tile], p_particles[tile])
  end
end

-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
