#include "scenemanager.h"
#include "stb_image.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>

// Maybe make some sort of struct in the header file to keep track of these..
#define NUM_BLADES 2000000

// Shader Vertex attribute location
#define POSITION_LOCATION 0
#define V1_LOCATION 1
#define V2_LOCATION 2
#define UP_LOCATION 3
#define TERRAIN_POS_LOCATION 5
#define TERRAIN_TEX_LOCATION 6

// OpenGL Shader Textures
#define TEXTURE_GRASS_FORCES 0
#define TEXTURE_GRASS_DIFFUSE 1
#define TEXTURE_FLOOR_HEIGHTMAP 2
#define TEXTURE_FLOOR_ALBEDO 3
#define TEXTURE_FLOOR_METALLIC 4
#define TEXTURE_FLOOR_ROUGHNESS 5
#define TEXTURE_FLOOR_AO 6
#define TEXTURE_FLOOR_NORMAL 7

#define CHECK_GL_ERROR printf("line:%d, error:%d\n", __LINE__, glGetError())

unsigned int load_texture(char const* path, unsigned int& textureID, int& width, int& height);

void SceneManager::initialize()
{
	// Im not gonna bother deleting this in the deconstructor, who cares?
	camera = new Camera({ 30.0f, 15.0f, 2.0f }, { 0.0f, 1.0f, 0.0f });

	// Compile shaders
	compute_shader = new Shader("shaders/forces.compute");	
	grass_shader = new Shader("shaders/blade.vs", "shaders/blade.fs", nullptr, "shaders/blade.tcs", "shaders/blade.tes");

	terrain = new Terrain();
	terrain->heightmap_file = "resources/iceland_heightmap.png";
	terrain->rez = 60;
	terrain->shader = new Shader("shaders/terrain.vs", "shaders/terrain.fs", nullptr, "shaders/terrain.tcs", "shaders/terrain.tes");	

	// Set worker dimensions for grass compute shader
	grass_workers.x = (unsigned int)(sqrt(NUM_BLADES) + 1);
	grass_workers.y = grass_workers.x - 1;

	init_terrain();	
	init_grass();
}

void SceneManager::init_grass()
{
	CHECK_GL_ERROR;
	std::vector<Blade> blades;
	blades.reserve(NUM_BLADES);

	const float X = terrain->width / 4;
	const float Z = terrain->height / 4;

	for (int i = 0; i < NUM_BLADES; ++i)
	{
		Blade blade;
		glm::vec3 position;
		glm::vec3 up_dir;		

		float x_pos = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / X));
		float z_pos = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / Z));
		int rand = std::rand();

		int sgn1 = (rand & 2) == 1 ? 1 : -1;
		int sgn2 = (rand & 4) == 1 ? 1 : -1;

		position = glm::vec3(sgn1*x_pos/2, 0.0f, sgn2*z_pos/2);
		position += glm::vec3(30.0f, 0, 0);
		
		// position /= 20.0f; // Put the blades closer together. Maybe I cans put a density sort of factor...
		up_dir = glm::vec3(0, 1.0f, 0);
		blade = Blade(position, up_dir, PI/2 + sgn1*sgn2*RANDOM_OFFSET*PI/2, std::max(0.5f, RANDOM_OFFSET + 0.2f), std::min(0.1f, RANDOM_OFFSET/3.0f));
				
		blades.push_back(blade);
	}	

	CHECK_GL_ERROR;
	glPatchParameteri(GL_PATCH_VERTICES, 1); // Only 1 control point on the patch!

	glGenVertexArrays(1, &grass_vao);
	glGenBuffers(4, grass_vbo);
	glBindVertexArray(grass_vao);
	CHECK_GL_ERROR;

	// Compute shader stuff 

	// Generate the texture and bind it.
	CHECK_GL_ERROR;
	glGenTextures(1, &pressure_map);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_GRASS_FORCES);
	glBindTexture(GL_TEXTURE_2D, pressure_map);
	CHECK_GL_ERROR;
	
	// Set how texture repeats/scales
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, grass_workers.x, grass_workers.y, 0, GL_RGBA, GL_FLOAT, nullptr);
	// Bind our texture so that we can access it from the compute shader.

	CHECK_GL_ERROR;
	glBindImageTexture(4, pressure_map, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	CHECK_GL_ERROR;

	// Load grass diffuse texture	
	int tmp_width = 0;
	int tmp_height = 0;
	load_texture("resources/grass_diffuse.png", grass_texture, tmp_width, tmp_height);

	// Setup vertex buffers and attributes
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POSITION_LOCATION, grass_vbo[POSITION_LOCATION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, V1_LOCATION, grass_vbo[V1_LOCATION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, V2_LOCATION, grass_vbo[V2_LOCATION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, UP_LOCATION, grass_vbo[UP_LOCATION]);	
	
	// position attribute
	std::vector<glm::vec4> vertex_data(NUM_BLADES);	
	for (int i = 0; i < NUM_BLADES; ++i) vertex_data[i] = blades[i].v0;	
	
	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), vertex_data.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// v1
	for (int i = 0; i < NUM_BLADES; ++i) vertex_data[i] = blades[i].v1;	

	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), vertex_data.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(V1_LOCATION);
	glVertexAttribPointer(V1_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// v2
	for (int i = 0; i < NUM_BLADES; ++i) vertex_data[i] = blades[i].v2;	

	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), vertex_data.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(V2_LOCATION);
	glVertexAttribPointer(V2_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// Up	
	for (int i = 0; i < NUM_BLADES; ++i) vertex_data[i] = blades[i].up;	

	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), vertex_data.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(UP_LOCATION);
	glVertexAttribPointer(UP_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);	
}

void SceneManager::init_terrain()
{	
	// Load and create the texture	
	load_texture(terrain->heightmap_file.c_str(), terrain->heightmap_texture, terrain->width, terrain->height);
	
	terrain->heightmap_bounds = glm::vec4(-terrain->width / 2, -terrain->height / 2, terrain->width, terrain->height);
	CHECK_GL_ERROR;

	// Set up vertex data	
	unsigned int rez = terrain->rez;
	
	std::vector<float> vertices;
	vertices.reserve(20 * rez * rez);

	float width(terrain->width);
	float height(terrain->height);

	for (int i = 0; i < rez; ++i)
	{
		for (int j = 0; j < rez; ++j)
		{			
			vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
			vertices.push_back(i / (float)rez); // u
			vertices.push_back(j / (float)rez); // v

			vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
			vertices.push_back((i + 1) / (float)rez); // u
			vertices.push_back(j / (float)rez); // v

			vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.z
			vertices.push_back(i / (float)rez); // u
			vertices.push_back((j + 1) / (float)rez); // v

			vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.z
			vertices.push_back((i + 1) / (float)rez); // u
			vertices.push_back((j + 1) / (float)rez); // v
		}
	}

	glGenVertexArrays(1, &terrain->vao);
	glBindVertexArray(terrain->vao);

	glGenBuffers(1, &terrain->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, terrain->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(TERRAIN_POS_LOCATION, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(TERRAIN_POS_LOCATION);
	// texCoord attribute
	glVertexAttribPointer(TERRAIN_TEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(TERRAIN_TEX_LOCATION);

	glPatchParameteri(GL_PATCH_VERTICES, 4);
	CHECK_GL_ERROR;

	// Now set the floor textures.
	// Dummy variable, I don't need to keep the width and height of these textures.
	int tmp_width = 0;
	int tmp_height = 0;	
	load_texture("resources/floor_albedo.jpg", terrain->albedo_texture, tmp_width, tmp_height);
	load_texture("resources/floor_metal.png", terrain->metal_texture, tmp_width, tmp_height);
	load_texture("resources/floor_ao.jpg", terrain->ao_texture, tmp_width, tmp_height);
	load_texture("resources/floor_roughness.jpg", terrain->roughness_texutre, tmp_width, tmp_height);
	load_texture("resources/floor_normal.png", terrain->normal_texture, tmp_width, tmp_height);
}

void SceneManager::app_logic(float delta_time)
{
	// Activate textures
	glActiveTexture(GL_TEXTURE0 + TEXTURE_GRASS_DIFFUSE);
	glBindTexture(GL_TEXTURE_2D, grass_texture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_FLOOR_HEIGHTMAP);
	glBindTexture(GL_TEXTURE_2D, terrain->heightmap_texture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_FLOOR_ALBEDO);
	glBindTexture(GL_TEXTURE_2D, terrain->albedo_texture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_FLOOR_AO);
	glBindTexture(GL_TEXTURE_2D, terrain->ao_texture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_FLOOR_METALLIC);
	glBindTexture(GL_TEXTURE_2D, terrain->metal_texture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_FLOOR_ROUGHNESS);
	glBindTexture(GL_TEXTURE_2D, terrain->roughness_texutre);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_FLOOR_NORMAL);
	glBindTexture(GL_TEXTURE_2D, terrain->normal_texture);

	// Prep data for uniforms	
	glm::mat4 model(1.0f);
	glm::mat4 model_inverse = inverse(model);
	glm::mat3 model_inverse_transpose = glm::inverseTranspose(glm::mat3(model));


	//model = glm::translate(model, glm::vec3(0.5f, 0.0f, -2.0f));
	// model = glm::scale(model, glm::vec3(1.0f));
	glm::mat4 view = camera->GetViewMatrix();
	const float near = 0.1f;
	const float far = 100.0f;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)scr_width / (float)scr_height, near, far);

	const glm::vec3 light_dir(0.7, -1.0f, 0.2f);
	const glm::vec3 light_colour(1.0f, 1.0f, 1.0f);

	grass_shader->use();
	grass_shader->setInt("heightMap", TEXTURE_FLOOR_HEIGHTMAP);

	grass_shader->setVec3("light_dir", light_dir);
	grass_shader->setVec3("light_colour", light_colour);
	grass_shader->setVec3("cam_pos", camera->Position);
	grass_shader->setVec2("near_far", glm::vec2(near, far));

	grass_shader->setMat4("model", model);
	grass_shader->setMat4("view", view);
	grass_shader->setMat4("projection", projection);

	grass_shader->setBool("useHeightMap", terrain ? 1 : 0);
	grass_shader->setInt("diffuse_texture", TEXTURE_GRASS_DIFFUSE);	

	// Floor shader
	if (terrain) {
		grass_shader->setVec4("heightMapBounds", terrain->heightmap_bounds);
		grass_shader->setInt("heightMap", TEXTURE_FLOOR_HEIGHTMAP);
	}	
	
	if (terrain) 
	{
		terrain->shader->use();
		terrain->shader->setVec3("light_dir", light_dir);
		terrain->shader->setVec3("light_colour", light_colour);
		terrain->shader->setVec3("cam_pos", camera->Position);

		terrain->shader->setMat4("projection", projection);
		terrain->shader->setMat4("view", view);
		terrain->shader->setMat4("model", model);

		terrain->shader->setInt("heightMap", TEXTURE_FLOOR_HEIGHTMAP);
		terrain->shader->setInt("albedo_map", TEXTURE_FLOOR_ALBEDO);
		terrain->shader->setInt("metallic_map", TEXTURE_FLOOR_METALLIC);
		terrain->shader->setInt("roughness_map", TEXTURE_FLOOR_ROUGHNESS);
		terrain->shader->setInt("ao_map", TEXTURE_FLOOR_AO);
		terrain->shader->setInt("normal_map", TEXTURE_FLOOR_NORMAL);		
	}

	// Compute shader loop
	compute_shader->use();

	static float acc = 0;
	acc += delta_time;
	if (acc > 100.0f) {
		acc = 0;
	}

	glm::vec4 wind_data(1.0f, 0.0f, -0.8f, acc); // W component is wind strength	

	// Set compute uniforms
	compute_shader->setInt("amount_blades", NUM_BLADES);
	compute_shader->setUVec2("workers", grass_workers);	

	compute_shader->setVec4("wind_data", wind_data);
	compute_shader->setBool("useHeightMap", terrain ? 1 : 0);
	if (terrain) {
		compute_shader->setVec4("heightMapBounds", terrain->heightmap_bounds);
	}
	
	compute_shader->setInt("heightMap", TEXTURE_FLOOR_HEIGHTMAP);

	compute_shader->setMat4("model", model);
	compute_shader->setMat4("model_inverse", model_inverse);
	compute_shader->setMat3("model_inverse_transpose", model_inverse_transpose);
	compute_shader->setInt("amount_blades", NUM_BLADES);
	compute_shader->setFloat("dt", 1.0f /* delta_time */);
	
}

void SceneManager::draw()
{	
	compute_shader->use();
	// Run the workers
	glDispatchCompute(grass_workers.x, grass_workers.y, 1);
	// Wait for workers to finish
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	grass_shader->use();

	glPatchParameteri(GL_PATCH_VERTICES, 1); 
	glBindVertexArray(grass_vao);
	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[0]);	
	
	glDrawArrays(GL_PATCHES, 0, NUM_BLADES);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	
	glBindVertexArray(0);

	terrain->shader->use();
	glPatchParameteri(GL_PATCH_VERTICES, 4);
	glBindVertexArray(terrain->vao);
	glDrawArrays(GL_PATCHES, 0, 4 * terrain->rez * terrain->rez);
	glBindVertexArray(0);
}

unsigned int load_texture(char const* path, unsigned int& textureID, int& width, int& height)
{
	glGenTextures(1, &textureID);

	int nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = 0;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}