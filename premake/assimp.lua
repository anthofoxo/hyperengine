project "assimp"

location "%{wks.location}/vendor/%{prj.name}"
language "C++"
warnings "Off"
optimize "Speed"

defines
{
    -- The following formats are enabled
    --"ASSIMP_BUILD_NO_OBJ_IMPORTER",
    --"ASSIMP_BUILD_NO_COLLADA_IMPORTER",
    --"ASSIMP_BUILD_NO_BLEND_IMPORTER",
    -- The following does not work
    "ASSIMP_BUILD_NO_C4D_IMPORTER",
    "ASSIMP_BUILD_NO_STEP_IMPORTER",
    "ASSIMP_BUILD_NO_IFC_IMPORTER",
    -- The following are not currently used
    "ASSIMP_BUILD_NO_X_IMPORTER",
    "ASSIMP_BUILD_NO_MD3_IMPORTER",
    "ASSIMP_BUILD_NO_MDL_IMPORTER",
    "ASSIMP_BUILD_NO_MD2_IMPORTER",
    "ASSIMP_BUILD_NO_PLY_IMPORTER",
    "ASSIMP_BUILD_NO_ASE_IMPORTER",
    "ASSIMP_BUILD_NO_HMP_IMPORTER",
    "ASSIMP_BUILD_NO_SMD_IMPORTER",
    "ASSIMP_BUILD_NO_MDC_IMPORTER",
    "ASSIMP_BUILD_NO_MD5_IMPORTER",
    "ASSIMP_BUILD_NO_STL_IMPORTER",
    "ASSIMP_BUILD_NO_LWO_IMPORTER",
    "ASSIMP_BUILD_NO_DXF_IMPORTER",
    "ASSIMP_BUILD_NO_NFF_IMPORTER",
    "ASSIMP_BUILD_NO_RAW_IMPORTER",
    "ASSIMP_BUILD_NO_OFF_IMPORTER",
    "ASSIMP_BUILD_NO_AC_IMPORTER",
    "ASSIMP_BUILD_NO_BVH_IMPORTER",
    "ASSIMP_BUILD_NO_IRRMESH_IMPORTER",
    "ASSIMP_BUILD_NO_IRR_IMPORTER",
    "ASSIMP_BUILD_NO_Q3D_IMPORTER",
    "ASSIMP_BUILD_NO_B3D_IMPORTER",
    "ASSIMP_BUILD_NO_TERRAGEN_IMPORTER",
    "ASSIMP_BUILD_NO_CSM_IMPORTER",
    "ASSIMP_BUILD_NO_3D_IMPORTER",
    "ASSIMP_BUILD_NO_LWS_IMPORTER",
    "ASSIMP_BUILD_NO_OGRE_IMPORTER",
    "ASSIMP_BUILD_NO_MS3D_IMPORTER",
    "ASSIMP_BUILD_NO_COB_IMPORTER",
    "ASSIMP_BUILD_NO_Q3BSP_IMPORTER",
    "ASSIMP_BUILD_NO_NDO_IMPORTER",
    "ASSIMP_BUILD_NO_M3_IMPORTER",
    "ASSIMP_BUILD_NO_XGL_IMPORTER",
    -- Build openddl as a static lib to avoid dynamic lib errors
    "OPENDDL_STATIC_LIBARY"
}
includedirs
{
    -- We provide our own assimp/config.h file so you dont have to touch cmake
    "%{wks.location}/vendor/assimp_config",
    "%{prj.location}/include",
    "%{prj.location}/code",
    "%{prj.location}",
    -- Contrib
    "%{prj.location}/contrib/irrXML",
    "%{prj.location}/contrib/rapidjson/include",
    "%{prj.location}/contrib/unzip",
    "%{prj.location}/contrib/zlib", --built separate
    --vendor_loc .. "zlib", --^^
    "%{prj.location}/contrib/openddlparser/include",
}
files
{
    "%{prj.location}/code/**.c",
    "%{prj.location}/code/**.cpp",
    "%{prj.location}/code/**.h",
    -- Contrib
    "%{prj.location}/contrib/irrXML/**.cpp",
    "%{prj.location}/contrib/irrXML/**.h",
    "%{prj.location}/contrib/unzip/*.c",
    "%{prj.location}/contrib/unzip/*.h",
    "%{prj.location}/contrib/zlib/*.c", --built separate
    "%{prj.location}/contrib/zlib/*.h", --built separate
    "%{prj.location}/contrib/openddlparser/code/*.cpp",
}
filter "system:windows"
    defines "_CRT_SECURE_NO_WARNINGS"