local objects = {
  {
    translation = { 0, 0, 0 },
    scale = { 1, 1, 1 },
    name    = "varoom",
    mesh    = "varoom.obj",
    texture = "varoom.png",
    shader  = "opaque.glsl"
  },
  {
    translation = { 0, 0, -3 },
    scale = { 0.25, 0.25, 0.25 },
    name    = "dragon",
    mesh    = "dragon.obj",
    texture = "color.png",
    specular = "internal://white.png",
    shader  = "opaque.glsl"
  },
  {
    translation = { 5.5, 2.8, -3.3 },
    scale = { 0.3, 0.3, 0.3 },
    name    = "barrel",
    mesh    = "barrel.obj",
    texture = "barrel.png",
    specular = "barrel_s.png",
    shader  = "opaque.glsl"
  },
  {
    translation = { -20, 0, -30 },
    scale = { 1, 1, 1 },
    name    = "boulder",
    mesh    = "boulder.obj",
    texture = "boulder.png",
    shader  = "opaque.glsl"
  },
  {
    translation = { 5, 0, -3 },
    scale = { 0.01, 0.01, 0.01 },
    name    = "crate",
    mesh    = "crate.obj",
    texture = "crate.png",
    shader  = "opaque.glsl"
  },
  {
    translation = { 3, 0, -3 },
    scale = { 1, 1, 1 },
    name    = "fox",
    mesh    = "fox.obj",
    texture = "fox.png",
    shader  = "opaque.glsl"
  },
  {
    translation = { -8, -7, -12 },
    scale = { 1, 1, 1 },
    name    = "fern",
    mesh    = "fern.obj",
    texture = "fern.png",
    shader  = "cutout.glsl"
  },
}

for x=-3,3 do
  for z=-3,3 do
    if not(x == 0 and z == 0) then
      table.insert(objects, {
        translation = { x * 20, 0, -z * 20 },
        scale = { 0.75, 0.75, 0.75 },
        name    = "pine " .. x .. "_" .. z,
        mesh    = "pine.obj",
        texture = "pine.png",
        shader  = "cutout.glsl"
      })
    end
  end
end

return objects