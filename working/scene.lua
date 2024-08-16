local objects = {
	{
		GameObject = {
			name = "sun",
			uuid = 0xf74222b26926a6db,
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
			uuid = 0xcc2481208078944e,
			translation = { 0, 0, 0 },
			scale = { 1.5, 1.0, 1.5 },
		},
		MeshFilter = {
			resource = "terrain.obj",
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
			uuid = 0x534a125132178692,
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
			uuid = 0x4bf6adf8d8a24128,
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
			uuid = 0x69a0cdc1d627f863,
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
			uuid = 0x9fade471bbb80cd9,
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
			uuid = 0x73a8d7b070a84b35,
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
			uuid = 0xb52df808435d3c33,
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

local pineUuids = {
	0x325779d1a5844b56,
	0xd8db236497070392,
	0xd58301b50ec20595,
	0x58fd59165c27f68a,
	0x601f724da4f60394,
	0xe25aeb72ebdbcc2c,
	0x9677d14cd5c73664,
	0xa7764c367b38b5a7,
	0x26c7c3d87b553c8c,
	0x792f974059533d5f,
	0xd6bf9cf2c4ebb1a4,
	0xb7a6d55f92481598,
	0x9376c4a85bca8cd5,
	0x18c8bcb029e9c883,
	0xc7a50bb78c71670b,
	0xae7ac4a48c1000ae,
	0x29a4b8398e625895,
	0xed0310eb87010798,
	0x6f448817816b96a1,
	0xab9db54dc54a42fd,
	0x7ce0eed74eff3e79,
	0x67037c70b441a99c,
	0xc4964b57544ff333,
	0x67909398a15e6454,
	0x69ad177ac6fb299c,
	0xc6615e55c00d2474,
	0x7c313e5bf5777854,
	0x76f0ee2d7e87ea09,
	0xfe4bbed7ef5fb06f,
	0x68caf83a0193d8b3,
	0xbabad0d7f6de77a9,
	0x341ce523f1f100bf,
	0xb9dc15d9e3407423,
	0x0fb764b705f2cad4,
	0x36507106bf5e6343,
	0x48b20825c50b0b3f,
	0x06931bba852953df,
	0x595fa02f2ee51a49,
	0xe8a848e1135ac735,
	0xcdc8c617d0b1ebfa,
	0x12b3afa9f46f449b,
	0xf340cd6b171199b4,
	0x2f12545ae4eb74c8,
	0xfbe93af1fb8ef847,
	0xde744fc468a63601,
	0x924c71c55ababab5,
	0x421240d522bffdd8,
	0xc41e9ba46bfd755b,
}

local uuidIdx = 1

for z=-3,3 do
	for x=-3,3 do
		if not(x == 0 and z == 0) then
			table.insert(objects, {
				GameObject = {
					name = "pine " .. x .. "_" .. z,
					uuid = pineUuids[uuidIdx],
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
			uuidIdx = uuidIdx + 1
		end
	end
end

return objects