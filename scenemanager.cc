#include "scenemanager.h"
#include "stb_image.h"
#include <glm/gtc/matrix_inverse.hpp>

#define NUM_BLADES 50000
#define POSITION_LOCATION 0
#define V1_LOCATION 1
#define V2_LOCATION 2
#define UP_LOCATION 3
#define TERRAIN_POS_LOCATION 5
#define TERRAIN_TEX_LOCATION 6

#define CHECK_GL_ERROR printf("line:%d, error:%d\n", __LINE__, glGetError())

void SceneManager::initialize()
{
	// Im not gonna bother deleting this in the deconstructor, who cares?
	camera = new Camera({ 30.0f, 15.0f, 2.0f }, { 0.0f, 1.0f, 0.0f });

	// Compile shaders
	compute_shader = new Shader("shaders/forces.compute");	
	grass_shader = new Shader("shaders/blade.vs", "shaders/blade.fs", nullptr, "shaders/blade.tcs", "shaders/blade.tes");

	terrain = new Terrain();
	terrain->heightmap_file = "iceland_heightmap.png";
	terrain->rez = 30;
	terrain->shader = new Shader("shaders/terrain.vs", "shaders/terrain.fs", nullptr, "shaders/terrain.tcs", "shaders/terrain.tes");	

	init_grass();
	init_terrain();	
}

void SceneManager::init_grass()
{
	CHECK_GL_ERROR;
	std::vector<Blade> blades;
	blades.reserve(NUM_BLADES);

	const float X = 100.0f;
	const float Z = 100.0f;

	for (int i = 0; i < NUM_BLADES; ++i)
	{
		Blade blade;
		glm::vec3 position;
		glm::vec3 up_dir;		

		const float h = 1.0f;
		const float w = 0.3f;

		float x_pos = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / X));
		float z_pos = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / Z));
		int rand = std::rand();

		int sgn1 = (rand & 2) == 1 ? 1 : -1;
		int sgn2 = (rand & 4) == 1 ? 1 : -1;

		position = glm::vec3(sgn1*x_pos/2, 0.0f, sgn2*z_pos/2);
		position += glm::vec3(30.0f, 0, 0);
		
		// position /= 20.0f; // Put the blades closer together. Maybe I cans put a density sort of factor...
		up_dir = glm::vec3(0, 1.0f, 0);
		blade = Blade(position, up_dir, PI/2 + sgn1*sgn2*RANDOM_OFFSET*PI/2, 0.5f, 0.1f);

				
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
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressure_map);
	
	// Set how texture repeats/scales
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NUM_BLADES, 1, 0, GL_RGBA, GL_FLOAT, nullptr);

	// Bind our texture so that we can access it from the compute shader.
	CHECK_GL_ERROR;
	glBindImageTexture(4, pressure_map, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	CHECK_GL_ERROR;
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POSITION_LOCATION, grass_vbo[POSITION_LOCATION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, V1_LOCATION, grass_vbo[V1_LOCATION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, V2_LOCATION, grass_vbo[V2_LOCATION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, UP_LOCATION, grass_vbo[UP_LOCATION]);	
	
	// position attribute
	std::vector<glm::vec4> v0_arr(NUM_BLADES);
	for (int i = 0; i < NUM_BLADES; ++i)
	{
		v0_arr[i] = blades[i].v0;		
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), v0_arr.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// v1
	std::vector<glm::vec4> v1_arr(NUM_BLADES);
	for (int i = 0; i < NUM_BLADES; ++i)
	{
		v1_arr[i] = blades[i].v1;
	}

	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), v1_arr.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(V1_LOCATION);
	glVertexAttribPointer(V1_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// v2
	std::vector<glm::vec4> v2_arr(NUM_BLADES);
	for (int i = 0; i < NUM_BLADES; ++i)
	{
		v2_arr[i] = blades[i].v2;
	}

	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), v2_arr.data(), GL_STATIC_DRAW);	
	glEnableVertexAttribArray(V2_LOCATION);
	glVertexAttribPointer(V2_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// Up
	std::vector<glm::vec4> up(NUM_BLADES);
	for (int i = 0; i < NUM_BLADES; ++i)
	{
		up[i] = blades[i].up;
	}

	glBindBuffer(GL_ARRAY_BUFFER, grass_vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, NUM_BLADES * sizeof(glm::vec4), up.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(UP_LOCATION);
	glVertexAttribPointer(UP_LOCATION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);	
}

void SceneManager::init_terrain()
{	
	glGenTextures(1, &terrain->texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrain->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load and create the texture
	unsigned char* data = stbi_load(terrain->heightmap_file.c_str(), &terrain->width, &terrain->height, &terrain->n_channels, 0);
	

	if (data) 
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, terrain->width, terrain->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Need to use the shader before setting the variable..
		CHECK_GL_ERROR;
		printf("Loaded heightmap of size %d x %d\n", terrain->height, terrain->width);
		terrain->heightmap_bounds = glm::vec4(-terrain->width / 2, -terrain->height / 2, terrain->width, terrain->height);
	}
	else 
	{
		printf("Failed to load texture");
	}

	stbi_image_free(data);

	// Set up vertex data
	unsigned int rez = terrain->rez;
	
	std::vector<float> vertices;
	vertices.reserve(5 * rez * rez);

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

}

void SceneManager::app_logic(float delta_time)
{
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
	grass_shader->setInt("heightMap", 1);

	grass_shader->setVec3("light_dir", light_dir);
	grass_shader->setVec3("light_colour", light_colour);
	grass_shader->setVec3("cam_pos", camera->Position);
	grass_shader->setVec2("near_far", glm::vec2(near, far));

	grass_shader->setMat4("model", model);
	grass_shader->setMat4("view", view);
	grass_shader->setMat4("projection", projection);

	grass_shader->setBool("useHeightMap", terrain ? 1 : 0);

	// Floor shader
	if (terrain) {
		grass_shader->setVec4("heightMapBounds", terrain->heightmap_bounds);
		grass_shader->setInt("heightMap", 1);
	}	
	
	if (terrain) {
		terrain->shader->use();
		terrain->shader->setVec3("light_dir", light_dir);
		terrain->shader->setVec3("light_colour", light_colour);
		terrain->shader->setVec3("cam_pos", camera->Position);

		terrain->shader->setInt("heightMap", 1);
		terrain->shader->setMat4("projection", projection);
		terrain->shader->setMat4("view", view);
		terrain->shader->setMat4("model", model);
	}

	// Compute shader loop
	compute_shader->use();

	static float acc = 0;
	acc += delta_time;
	if (acc > 100.0f) {
		acc = 0;
	}

	glm::vec4 wind_data(1.0f, 0.0f, -0.8f, (sin(acc) + cos(acc)) * 0.1f); // W component is wind strength	

	// Set compute uniforms
	compute_shader->setVec4("wind_data", wind_data);
	compute_shader->setBool("useHeightMap", terrain ? 1 : 0);
	if (terrain) {
		compute_shader->setVec4("heightMapBounds", terrain->heightmap_bounds);
	}
	compute_shader->setInt("heightMap", 1);

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
	glDispatchCompute(NUM_BLADES, 1, 1);
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