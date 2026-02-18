#version 330 core

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vLightSpacePos;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;
uniform vec3 uColor;

uniform sampler2D uShadowMap;
uniform vec3 uLightDir;   // direction of light rays (from light -> scene), normalized

float ShadowFactor(vec4 lightSpacePos, vec3 normal)
{
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
    proj = proj * 0.5 + 0.5;

    // outside the shadow map => no shadowing
    if (proj.z > 1.0) return 1.0;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 1.0;

    float current = proj.z;

    // Bias (bigger at grazing angles)
    vec3 L = normalize(-uLightDir);
    float bias = max(0.0035 * (1.0 - dot(normal, L)), 0.0012);

    // 3x3 PCF
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float closest = texture(uShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (current - bias) > closest ? 0.0 : 1.0;
        }
    }

    return shadow / 9.0;
}

void main()
{
    vec3 albedo = (uUseTexture != 0) ? texture(uTexture, vUV).rgb : uColor;

    // Flat geometric normal from derivatives
    vec3 dx = dFdx(vWorldPos);
    vec3 dy = dFdy(vWorldPos);
    vec3 N  = normalize(cross(dx, dy));

    // IMPORTANT: fix inconsistent winding (this is what causes “checker” lighting)
    if (!gl_FrontFacing)
        N = -N;

    vec3 L = normalize(-uLightDir);

    float ndotl = max(dot(N, L), 0.0);

    float ambient = 0.28;
    float shadow  = ShadowFactor(vLightSpacePos, N);

    vec3 lit = albedo * (ambient + ndotl * shadow);

    FragColor = vec4(lit, 1.0);
}
