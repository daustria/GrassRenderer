#version 430 core
layout (location = 0) in vec4 position; // v0
layout (location = 1) in vec4 v1;
layout (location = 2) in vec4 v2;
layout (location = 3) in vec4 up;

out vec4 vs_v1;
out vec4 vs_v2;
out vec3 vs_blade_dir;
out vec3 vs_blade_up;

uniform mat4 model;
uniform sampler2D heightMap;

void main()
{
    vec4 pos = model * vec4(position.xyz, 1.0f);
    vs_v1 = vec4((model * vec4(v1.xyz, 1.0f)).xyz, v1.w);
    vs_v2 = vec4((model * vec4(v2.xyz, 1.0f)).xyz, v2.w);

    vs_blade_up = normalize(up.xyz);

    gl_Position = pos;

    float dir = position.w;
	float sd = sin(dir);
	float cd = cos(dir);
	vec3 tmp = normalize(vec3(sd, sd + cd, cd));    
    vs_blade_dir = normalize(cross(vs_blade_up, tmp));
    // vs_blade_dir = vec3(0.1f, 1.0f, 0.0f);
}