layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    mat4 gLightMat;
    vec3 gSkyColor;
    float gFarPlane;
    vec3 gSunDirection;
    vec3 gSunColor;
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
VARYING(vec3, vPosition);
VARYING(vec4, vFragPosLightSpace);
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo0;
uniform sampler2D tAlbedo1;
uniform sampler2D tAlbedo2;
uniform sampler2D tAlbedo3;
uniform sampler2D tBlendmap;

uniform sampler2D tShadowMap;

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vPosition = viewSpace.xyz;
    vFragPosLightSpace = gLightMat * worldSpace;
}
#endif

#ifdef FRAG

vec4 hash4(vec2 p) {
    return fract(sin(vec4(1.0 + dot(p, vec2(37.0, 17.0)),
                          2.0 + dot(p, vec2(11.0, 47.0)),
                          3.0 + dot(p, vec2(41.0, 29.0)),
                          4.0 + dot(p, vec2(23.0, 31.0)))) * 103.0);
}

vec4 textureNoTile(sampler2D samp, in vec2 uv) {
    ivec2 iuv = ivec2(floor(uv));
    vec2 fuv = fract(uv);

    // generate per-tile transform
    vec4 ofa = hash4(iuv + ivec2(0, 0));
    vec4 ofb = hash4(iuv + ivec2(1, 0));
    vec4 ofc = hash4(iuv + ivec2(0, 1));
    vec4 ofd = hash4(iuv + ivec2(1, 1));
    
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);

    // transform per-tile uvs
    ofa.zw = sign(ofa.zw - 0.5);
    ofb.zw = sign(ofb.zw - 0.5);
    ofc.zw = sign(ofc.zw - 0.5);
    ofd.zw = sign(ofd.zw - 0.5);
    
    // uv's, and derivatives (for correct mipmapping)
    vec2 uva = uv * ofa.zw + ofa.xy, ddxa = ddx * ofa.zw, ddya = ddy * ofa.zw;
    vec2 uvb = uv * ofb.zw + ofb.xy, ddxb = ddx * ofb.zw, ddyb = ddy * ofb.zw;
    vec2 uvc = uv * ofc.zw + ofc.xy, ddxc = ddx * ofc.zw, ddyc = ddy * ofc.zw;
    vec2 uvd = uv * ofd.zw + ofd.xy, ddxd = ddx * ofd.zw, ddyd = ddy * ofd.zw;
    
    // fetch and blend
    vec2 b = smoothstep(0.25, 0.75, fuv);
    
    return mix(mix(textureGrad(samp, uva, ddxa, ddya),
                   textureGrad(samp, uvb, ddxb, ddyb), b.x),
               mix(textureGrad(samp, uvc, ddxc, ddyc),
                   textureGrad(samp, uvd, ddxd, ddyd), b.x), b.y);
}

// https://habr.com/en/articles/442924/

const float kGamma = 2.2;

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
     if(projCoords.z > 1.0)
        return 0.0;
    
    vec2 texelSize = 1.0 / textureSize(tShadowMap, 0);
   
    
    float currentDepth = projCoords.z;
    float bias = max(0.003 * (1.0 - dot(vNormal, gSunDirection)), 0.0);
    
    float shadow = 0.0;
   
    
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            float pcfDepth = texture(tShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    
    return shadow / 9.0;
}

void main(void) {
    vec3 blendmapColor = texture(tBlendmap, vTexCoord).rgb;
    vec4 bias = vec4(1.0 - (blendmapColor.r + blendmapColor.g + blendmapColor.b), blendmapColor);
    vec4 color0 = pow(textureNoTile(tAlbedo0, vTexCoord * uTiling.x), vec4(kGamma));
    vec4 color1 = pow(textureNoTile(tAlbedo1, vTexCoord * uTiling.y), vec4(kGamma));
    vec4 color2 = pow(textureNoTile(tAlbedo2, vTexCoord * uTiling.z), vec4(kGamma));
    vec4 color3 = pow(textureNoTile(tAlbedo3, vTexCoord * uTiling.w), vec4(kGamma));

#ifdef USE_OLD_BLENDING
    oColor = vec4(color0.rgb * bias.x + color1.rgb * bias.y + color2.rgb * bias.z + color3.rgb * bias.w, 1.0);
#else
    color0.a = dot(color0.rgb, vec3(0.299, 0.587, 0.114));
    color1.a = dot(color1.rgb, vec3(0.299, 0.587, 0.114));
    color2.a = dot(color2.rgb, vec3(0.299, 0.587, 0.114));
    color3.a = dot(color3.rgb, vec3(0.299, 0.587, 0.114));

    float depth = 0.2;
    float ma = max(max(color0.a + bias.x, color1.a + bias.y), max(color2.a + bias.z, color3.a + bias.w)) - depth;
    float b0 = max(color0.a + bias.x - ma, 0.0);
    float b1 = max(color1.a + bias.y - ma, 0.0);
    float b2 = max(color2.a + bias.z - ma, 0.0);
    float b3 = max(color3.a + bias.w - ma, 0.0);
    oColor = vec4((color0.rgb * b0 + color1.rgb * b1 + color2.rgb * b2 + color3.rgb * b3)/(b0+b1+b2+b3), 1.0);
#endif

    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    
    float shadow = ShadowCalculation(vFragPosLightSpace);
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * (1.0 - shadow), 0.2);
    
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, length(vPosition)));
}
#endif