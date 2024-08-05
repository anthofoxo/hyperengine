layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    vec3 gSkyColor;
    float gFarPlane;
};

uniform Material {
    vec4 uTiling;
};

INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec3, vToCamera);
VARYING(vec2, vTexCoord);
VARYING(float, vDistance);
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo0;
uniform sampler2D tAlbedo1;
uniform sampler2D tAlbedo2;
uniform sampler2D tAlbedo3;
uniform sampler2D tBlendmap;
const vec3 kLightDirection = normalize(vec3(0, -3, -1));
const vec3 kLightColor = vec3(1.0, 1.0, 1.0);

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vDistance = length(viewSpace.xyz);
}
#endif

#ifdef FRAG

vec4 hash4( vec2 p ) { return fract(sin(vec4( 1.0+dot(p,vec2(37.0,17.0)), 
                                              2.0+dot(p,vec2(11.0,47.0)),
                                              3.0+dot(p,vec2(41.0,29.0)),
                                              4.0+dot(p,vec2(23.0,31.0))))*103.0); }

vec4 textureNoTile( sampler2D samp, in vec2 uv ) {
    ivec2 iuv = ivec2( floor( uv ) );
     vec2 fuv = fract( uv );

    // generate per-tile transform
    vec4 ofa = hash4( iuv + ivec2(0,0) );
    vec4 ofb = hash4( iuv + ivec2(1,0) );
    vec4 ofc = hash4( iuv + ivec2(0,1) );
    vec4 ofd = hash4( iuv + ivec2(1,1) );
    
    vec2 ddx = dFdx( uv );
    vec2 ddy = dFdy( uv );

    // transform per-tile uvs
    ofa.zw = sign( ofa.zw-0.5 );
    ofb.zw = sign( ofb.zw-0.5 );
    ofc.zw = sign( ofc.zw-0.5 );
    ofd.zw = sign( ofd.zw-0.5 );
    
    // uv's, and derivatives (for correct mipmapping)
    vec2 uva = uv*ofa.zw + ofa.xy, ddxa = ddx*ofa.zw, ddya = ddy*ofa.zw;
    vec2 uvb = uv*ofb.zw + ofb.xy, ddxb = ddx*ofb.zw, ddyb = ddy*ofb.zw;
    vec2 uvc = uv*ofc.zw + ofc.xy, ddxc = ddx*ofc.zw, ddyc = ddy*ofc.zw;
    vec2 uvd = uv*ofd.zw + ofd.xy, ddxd = ddx*ofd.zw, ddyd = ddy*ofd.zw;
        
    // fetch and blend
    vec2 b = smoothstep( 0.25,0.75, fuv );
    
    return mix( mix( textureGrad( samp, uva, ddxa, ddya ), 
                     textureGrad( samp, uvb, ddxb, ddyb ), b.x ), 
                mix( textureGrad( samp, uvc, ddxc, ddyc ),
                     textureGrad( samp, uvd, ddxd, ddyd ), b.x), b.y );
}

void main(void) {
    vec3 blendmapColor = texture(tBlendmap, vTexCoord).rgb;
    vec4 color0 = textureNoTile(tAlbedo0, vTexCoord * uTiling.x) * (1.0 - (blendmapColor.r + blendmapColor.g + blendmapColor.b));
    vec4 color1 = textureNoTile(tAlbedo1, vTexCoord * uTiling.y) * blendmapColor.r;
    vec4 color2 = textureNoTile(tAlbedo2, vTexCoord * uTiling.z) * blendmapColor.g;
    vec4 color3 = textureNoTile(tAlbedo3, vTexCoord * uTiling.w) * blendmapColor.b;
    oColor = color0 + color1 + color2 + color3;
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    oColor.rgb *= max(kLightColor * dot(unitNormal, -kLightDirection), 0.2);
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif