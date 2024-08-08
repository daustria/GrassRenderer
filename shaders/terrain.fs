#version 430 core

in float Height;
in vec3 tes_normal;
in vec3 tes_frag_pos;

out vec4 FragColor;

uniform vec3 light_dir;
uniform vec3 light_colour;
uniform vec3 cam_pos;

void main()
{		
	vec3 view_dir = normalize(cam_pos - tes_frag_pos);
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float shininess = 5.0f;
	float spec = pow(max(dot(tes_normal, halfway_dir), 0.0), shininess);
	vec3 specular = light_colour * spec;

	float h = (Height + 16)/64.0f;	

	vec3 ambient = 1.0f*light_colour;
	vec3 diffuse = max(dot(tes_normal, light_dir), 0.0f) * light_colour;

	vec3 colour = (specular+ambient+diffuse) * vec3(h,h,h);
	FragColor = vec4(colour, 1.0);	

	// vec3 brown = vec3(0.6, 0.4, 0);
	// FragColor = vec4(brown*vec3(h,h,h), 1.0);
}