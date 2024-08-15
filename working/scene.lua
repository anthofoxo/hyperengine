local objects = {
	{
		GameObject = {
			name = "sun",
			translation = { 0, 0, 0 },
			orientation = { -0.266697556, -0.760954082, -0.271479785, 0.525471270 },
			scale = { 1, 1, 1 },
		},
		Light = {
			color = { 1.0, 1.0, 1.0 },
			strength = 3.0,
		},
	},
	{
		GameObject = {
			name = "ground",
			translation = { 0, 0, 0 },
			scale = { 75, 75, 75 },
		},
		MeshFilter = {
			resource = "plane.obj",
		},
		MeshRenderer = {
			shader = "shaders/terrain_basic.glsl",
			textures = {
				tAlbedo0 = "grass.png",
				tAlbedo1 = "dirt.png",
				tAlbedo2 = "flowers.png",
				tAlbedo3 = "rocks.png",
				tBlendmap = "blendmap.png",
			},
			material = {
				uTiling = { 50, 50, 50, 100 },
			},
		},
	},
	{
		GameObject = {
			name = "varoom",
			translation = { -10, 1, 0 },
			scale = { 5, 5, 5 },
		},
		MeshFilter = {
			resource = "varoom.obj",
		},
		MeshRenderer = {
			shader = "shaders/opaque.glsl",
			textures = {
				tAlbedo = "varoom.png",
			},
			material = {
				uColor = { 0.0, 1.0, 1.0 },
			},
		},
	},
	{
		GameObject = {
			name = "dragon",
			translation = { -10, 0, -45 },
			scale = { 1, 1, 1 },
		},
		MeshFilter = {
			resource = "dragon.obj",
		},
		MeshRenderer = {
			shader = "shaders/opaque.glsl",
			textures = {
				tAlbedo = "internal://white.png",
				tSpecular = "internal://white.png",
			},
			material = {
				uColor = { 1.0, 0.75, 0.4 },
			},
		},
	},
	{
		GameObject = {
			name = "barrel",
			translation = { 13, 1.8, -15 },
			scale = { 0.3, 0.3, 0.3 },
		},
		MeshFilter = {
			resource = "barrel.obj",
		},
		MeshRenderer = {
			shader = "shaders/normal.glsl",
			textures = {
				tAlbedo = "barrel.png",
				tSpecular = "barrelS.png",
				tNormal = "barrelN.png",
			},
		},
	},
	{
		GameObject = {
			name = "crate",
			translation = { 8, 1, 0 },
			scale = { 0.01, 0.01, 0.01 },
		},
		MeshFilter = {
			resource = "crate.obj",
		},
		MeshRenderer = {
			shader = "shaders/normal.glsl",
			textures = {
				tAlbedo = "crate.png",
				tNormal = "crateN.png",
			},
		},
	},
	{
		GameObject = {
			name = "fox",
			translation = { 3, 1.7, -3 },
			scale = { 1, 1, 1 },
		},
		MeshFilter = {
			resource = "fox.obj",
		},
		MeshRenderer = {
			shader = "shaders/opaque.glsl",
			textures = {
				tAlbedo = "fox.png",
			},
			material = {
				uColor = { 1.0, 1.0, 1.0 },
			},
		},
	},
	{
		GameObject = {
			name = "fern",
			translation = { -2, 0, 2 },
			scale = { 0.4, 0.4, 0.4 },
		},
		MeshFilter = {
			resource = "fern.obj",
		},
		MeshRenderer = {
			shader = "shaders/cutout.glsl",
			textures = {
				tAlbedo = "fern.png",
			},
		},
	},
}

for x=-3,3 do
	for z=-3,3 do
		if not(x == 0 and z == 0) then
			table.insert(objects, {
				GameObject = {
					name = "pine " .. x .. "_" .. z,
					translation = { x * 20, 0, -z * 20 },
					scale = { 0.75, 0.75, 0.75 },
				},
				MeshFilter = {
					resource = "pine.obj",
				},
				MeshRenderer = {
					shader = "shaders/cutout.glsl",
					textures = {
						tAlbedo = "pine.png",
					},
				},
			})
		end
	end
end

return objects