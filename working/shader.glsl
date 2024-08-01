INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec3, vToCamera);
VARYING(vec2, vTexCoord);
OUTPUT(vec4, oColor, 0);
UNIFORM(mat4, uTransform);
UNIFORM(mat4, uView);
UNIFORM(mat4, uProjection);
UNIFORM(sampler2D, uSampler);
CONST(vec3, kLightDirection, normalize(vec3(0, 0, -1)));
CONST(vec3, kLightColor, vec3(1.0, 1.0, 1.0));

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    gl_Position = uProjection * uView * worldSpace;
    vTexCoord = iTexCoord;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(uView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
}
#endif

#ifdef FRAG
void main(void) {
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    oColor = texture(uSampler, vTexCoord);
    oColor.rgb *= max(kLightColor * dot(unitNormal, -kLightDirection), 0.2);
    vec3 reflectedLight = reflect(kLightDirection, unitNormal);
    oColor.rgb += kLightColor * pow(max(dot(reflectedLight, unitToCamera), 0.0), 10.0);
}
#endif