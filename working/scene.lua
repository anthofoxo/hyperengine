local objects = {
  {
    translation = { 0, 0, 0 },
    scale = { 75, 75, 75 },
    name    = "ground",
    mesh    = "plane.obj",
    shader  = "shaders/terrain_basic.glsl",
    textures = {
      tAlbedo0 = "grass.png",
      tAlbedo1 = "dirt.png",
      tAlbedo2 = "flowers.png",
      tAlbedo3 = "rocks.png",
      tBlendmap = "blendmap.png"
    },
    material = { uTiling = { 50, 50, 50, 100 } }
  },
  {
    translation = { -10, 1, 0 },
    scale = { 5, 5, 5 },
    name    = "varoom",
    mesh    = "varoom.obj",
    shader  = "shaders/opaque.glsl",
    textures = { tAlbedo = "varoom.png" },
    material = { uColor = { 0.0, 1.0, 1.0 } }
  },
  {
    translation = { -10, 0, -45 },
    scale = { 1, 1, 1 },
    name    = "dragon",
    mesh    = "dragon.obj",
    textures = { tAlbedo = "internal://white.png", tSpecular = "internal://white.png" },
    specular = "internal://white.png",
    shader  = "shaders/opaque.glsl",
    material = { uColor = { 1.0, 0.75, 0.4 } }
  },
  {
    translation = { 13, 1.8, -15 },
    scale = { 0.3, 0.3, 0.3 },
    name    = "barrel",
    mesh    = "barrel.obj",
    textures = { tAlbedo = "barrel.png", tSpecular = "barrelS.png", tNormal = "barrelN.png" },
    shader  = "shaders/normal.glsl"
  },
  {
    translation = { 8, 1, 0 },
    scale = { 0.01, 0.01, 0.01 },
    name    = "crate",
    mesh    = "crate.obj",
    textures = { tAlbedo = "crate.png", tNormal = "crateN.png" },
    shader  = "shaders/normal.glsl",
  },
  {
    translation = { 3, 1.7, -3 },
    scale = { 1, 1, 1 },
    name    = "fox",
    mesh    = "fox.obj",
    textures = { tAlbedo = "fox.png" },
    shader  = "shaders/opaque.glsl",
    material = { uColor = { 1.0, 1.0, 1.0 } }
  },
  {
    translation = { -2, 0, 2 },
    scale = { 0.4, 0.4, 0.4 },
    name    = "fern",
    mesh    = "fern.obj",
    textures = { tAlbedo = "fern.png" },
    shader  = "shaders/cutout.glsl",
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
        shader  = "shaders/cutout.glsl",
      })
    end
  end
end

return objects