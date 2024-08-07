#version 430 core

uniform float ambient;
uniform float diffuse;
uniform float specular;
uniform float specular_hardness;

uniform vec3 light_dir;
uniform vec3 light_colour;
uniform vec3 cam_pos;
uniform vec2 near_far;

in vec4 tes_position;
in vec3 tes_normal;
in vec2 tes_uv;

out vec4 FragColor;

void main()
{
	vec4 colour = vec4(1.0f, 0.8f, 0.82f, 1.0f);
	// vec4 colour = vec4(1.0f, 0.8f, 0, 1.0f);
;
	colour *= tes_uv.y + 0.5f; //pseudo AO

	vec3 normal = tes_normal;
	vec3 viewRay = cam_pos - tes_position.xyz;

    float viewDist = length(viewRay);

    viewRay = viewRay / viewDist;
    viewDist = (viewDist - near_far.x) / (near_far.y - near_far.x);
	vec3 lightRay = normalize(-light_dir);

	vec3 halfVec = normalize((lightRay + viewRay) / 2.0f);

	float NdL = clamp(dot(normal, lightRay),0.0f,1.0f);
	float NdH = clamp(dot(normal, halfVec),0.0f,1.0f);

	vec3 aCol = max(ambient * colour.rgb,0.0f);
	vec3 dCol = max(diffuse * NdL * colour.rgb,0.0f);
	vec3 sCol = max(specular * pow(NdH, specular_hardness) * colour.rgb,0.0f) * tes_uv.y;
	
	aCol *= light_colour;
	dCol *= light_colour;
	sCol *= light_colour;

	FragColor = vec4(aCol + dCol + sCol, 1.0f);
	// FragColor = vec4(sCol, 1.0f);
}