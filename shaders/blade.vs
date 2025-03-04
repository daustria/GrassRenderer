#version 430 core
layout(location = 0) in vec4 position; // v0
layout(location = 1) in vec4 v1;
layout(location = 2) in vec4 v2;
layout(location = 3) in vec4 up;

out vec4 vs_v1;
out vec4 vs_v2;
out vec3 vs_blade_dir;
out vec3 vs_blade_up;

uniform mat4 model;

uniform sampler2D heightMap;
uniform bool useHeightMap;
uniform vec4 heightMapBounds; // xMin zMin xLength zLength

void main()
{
    vec4 pos = model * vec4(position.xyz, 1.0f);
    vs_v1 = vec4((model * vec4(v1.xyz, 1.0f)).xyz, v1.w);
    vs_v2 = vec4((model * vec4(v2.xyz, 1.0f)).xyz, v2.w);

    vs_blade_up = normalize(up.xyz);

    if (useHeightMap) {

        // We assume that the bounds are -width/2, -height/2, where width and height are the
        // dimensions of the heightmap texture.
        // These read coordinates are the weights t in [0,1] so that t is the weight 'along the line'
        // from -weight/2 to weight/2. In other words, for some t we have x = (1-t)(-w/2) +t(w/2), similar for z

        float x_t = (2 * pos.x + heightMapBounds.z) / (2 * heightMapBounds.z);
        float z_t = (2 * pos.z + heightMapBounds.w) / (2 * heightMapBounds.w);

        float height = texture(heightMap, vec2(x_t, z_t)).y * 32.0f - 0.1f;

        pos.xyz += vs_blade_up * height;
        vs_v1.xyz += vs_blade_up * height;
        vs_v2.xyz += vs_blade_up * height;
    }

    gl_Position = pos;

    float dir = position.w;
    float sd = sin(dir);
    float cd = cos(dir);
    vec3 tmp = normalize(vec3(sd, sd + cd, cd));
    vs_blade_dir = normalize(cross(vs_blade_up, tmp));
}
