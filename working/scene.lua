local objects = {
  {
    translation = { 0, 0, 0 },
    scale = { 75, 75, 75 },
    name    = "ground",
    mesh    = "plane.obj",
    shader  = "terrain_basic.glsl",
    textures = { tAlbedo = "grass.png" },
    material = { uTiling = 75 }
  },
  {
    translation = { -10, 1, 0 },
    scale = { 5, 5, 5 },
    name    = "varoom",
    mesh    = "varoom.obj",
    shader  = "opaque.glsl",
    textures = { tAlbedo = "varoom.png" },
    material = { uColor = { 0.0, 1.0, 1.0 } }
  },
  {
    translation = { 0, 0, -10 },
    scale = { 1, 1, 1 },
    name    = "dragon",
    mesh    = "dragon.obj",
    textures = { tAlbedo = "internal://white.png", tSpecular = "internal://white.png" },
    specular = "internal://white.png",
    shader  = "opaque.glsl",
    material = { uColor = { 1.0, 0.75, 0.4 } }
  },
  {
    translation = { 5.5, 1.8, -3.3 },
    scale = { 0.3, 0.3, 0.3 },
    name    = "barrel",
    mesh    = "barrel.obj",
    textures = { tAlbedo = "barrel.png", tSpecular = "barrel_s.png" },
    shader  = "opaque.glsl",
    material = { uColor = { 1.0, 1.0, 1.0 } }
  },
  {
    translation = { -20, 0, -30 },
    scale = { 1, 1, 1 },
    name    = "boulder",
    mesh    = "boulder.obj",
    textures = { tAlbedo = "boulder.png" },
    shader  = "opaque.glsl",
    material = { uColor = { 1.0, 1.0, 1.0 } }
  },
  {
    translation = { 8, 1, 0 },
    scale = { 0.01, 0.01, 0.01 },
    name    = "crate",
    mesh    = "crate.obj",
    textures = { tAlbedo = "crate.png" },
    shader  = "opaque.glsl",
    material = { uColor = { 1.0, 1.0, 1.0 } }
  },
  {
    translation = { 3, 1.7, -3 },
    scale = { 1, 1, 1 },
    name    = "fox",
    mesh    = "fox.obj",
    textures = { tAlbedo = "fox.png" },
    shader  = "opaque.glsl",
    material = { uColor = { 1.0, 1.0, 1.0 } }
  },
  {
    translation = { -8, -7, -12 },
    scale = { 1, 1, 1 },
    name    = "fern",
    mesh    = "fern.obj",
    textures = { tAlbedo = "fern.png" },
    shader  = "cutout.glsl",
    material = { uCutoff = 0.5 }
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
        textures = { tAlbedo = "pine.png" },
        shader  = "cutout.glsl",
        material = { uCutoff = 0.5 }
      })
    end
  end
end

return objects