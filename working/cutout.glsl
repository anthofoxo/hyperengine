@property cull = 0
INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec3, vToCamera);
VARYING(vec2, vTexCoord);
VARYING(float, vDistance);
OUTPUT(vec4, oColor, 0);
UNIFORM(mat4, uTransform);
UNIFORM(mat4, uView);
UNIFORM(mat4, uProjection);
UNIFORM(sampler2D, uSampler);
UNIFORM(float, uFarPlane);
UNIFORM(vec3, uSkyColor);
CONST(vec3, kLightDirection, normalize(vec3(0, 0, -1)));
CONST(vec3, kLightColor, vec3(1.0, 1.0, 1.0));

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = uView * worldSpace;
    gl_Position = uProjection * viewSpace;
    vTexCoord = iTexCoord;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(uView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vDistance = length(viewSpace);
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(uSampler, vTexCoord);
    if (oColor.a < 0.5) discard;
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    if (!gl_FrontFacing) unitNormal *= -1.0;
    oColor.rgb *= max(kLightColor * dot(unitNormal, -kLightDirection), 0.2);
    vec3 reflectedLight = reflect(kLightDirection, unitNormal);
    oColor.rgb += kLightColor * pow(max(dot(reflectedLight, unitToCamera), 0.0), 10.0);
    oColor.rgb = mix(oColor.rgb, uSkyColor, smoothstep(uFarPlane * 0.6, uFarPlane, vDistance));
}
#endif