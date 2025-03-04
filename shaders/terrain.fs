#version 430 core

in float Height;
in vec3 tes_normal;
in vec3 tes_frag_pos;
in vec2 tes_uv;

out vec4 FragColor;

uniform vec3 light_dir;
uniform vec3 light_colour;
uniform vec3 cam_pos;

uniform sampler2D albedo_map;
uniform sampler2D normal_map;
uniform sampler2D metallic_map;
uniform sampler2D roughness_map;
uniform sampler2D ao_map;

#define PI 3.141592654

// These functions are for the DFG terms in the BRDF for PBR
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 normal, vec3 halfway_dir, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(normal, halfway_dir), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 gamma_correct(vec3 colour)
{
    return vec3(pow(colour.r, 2.2), pow(colour.g, 2.2), pow(colour.b, 2.2));
}

void main()
{
    // Artists make these albedo maps in sRGB space, we need to convert to linear
    vec3 texture_albedo = texture(albedo_map, tes_uv).rgb;
    vec3 albedo = gamma_correct(vec3(0.05f, 0.9f, 0.3f)) + 0.5 * gamma_correct(texture_albedo);

    float metallic = texture(metallic_map, tes_uv).r;
    float roughness = texture(roughness_map, tes_uv).r;
    float ao = texture(ao_map, tes_uv).r;
    const float shininess = 2.0f;

    vec3 normal = 0.2 * tes_normal + 0.8 * texture(normal_map, tes_uv).rgb;
    normal = normalize(normal);
    vec3 v = normalize(cam_pos - tes_frag_pos);

    // We process all our light sources now. Note that we only have one directional light,
    // so no attenuation calculation is needed.
    vec3 l = normalize(light_dir);
    vec3 halfway_dir = normalize(v + l); // 'halfway' vector between light and view directions
    vec3 radiance = light_colour;

    float spec = pow(max(dot(tes_normal, halfway_dir), 0.0), shininess);
    vec3 specular = light_colour * spec;

    // DFG terms
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(halfway_dir, l), 0.0), F0);
    float NDF = distributionGGX(normal, halfway_dir, roughness);
    float G = GeometrySmith(normal, v, light_dir, roughness);

    // Finalize the Cook-Torrance BRDF computation
    vec3 numerator = NDF * G * F;

    float denominator = 4.0 * max(dot(normal, v), 0.0) * max(dot(normal, l), 0.0);
    denominator += 0.0001; // Prevent divisions by zero
    vec3 kS = F; // Represents energy of light getting reflected, the specular contribution of light hitting the surface.
    vec3 kD = vec3(1.0) - kS; // Any light not reflected is refracted.
    kD *= 1.0 - metallic; // Metallic surfaces dont refract light. So we nullify kD based on how metallic the surfcace is.
    float ndotl = max(dot(normal, light_dir), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * ndotl;

    // This concludes the lighting integral equation computation.
    // Now we need an improvised ambient occlusion term ...
    vec3 ambient = vec3(0.5) * albedo * ao;
    vec3 color = ambient + Lo;

    // And convert to sRGB space.
    color = color / (color + vec3(1.0)); // tonme map the HDR to LDR first. Known as the Reinhard operator.
    color = pow(color, vec3(1.0 / 2.2));

    float h = (Height + 16) / 64.0f;

    FragColor = vec4(color, 1.0);
}