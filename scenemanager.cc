#include "scenemanager.h"
#include "stb_image.h"

#define NUM_BLADES 100
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
	camera = new Camera();

	// Compile shaders
	compute_shader = new Shader("shaders/forces.compute");	
	grass_shader = new Shader("shaders/blade.vs", "shaders/blade.fs", nullptr, "shaders/blade.tcs", "shaders/blade.tes");

	terrain = new Terrain();
	terrain->shader = new Shader("shaders/terrain.vs", "shaders/terrain.fs", nullptr, "shaders/terrain.tcs", "shaders/terrain.tes");	
	init_grass();
	init_terrain();	
}

void SceneManager::init_grass()
{
	CHECK_GL_ERROR;
	std::vector<Blade> blades;

	for (int i = 0; i < NUM_BLADES; ++i)
	{
		Blade blade;
		glm::vec3 position;
		glm::vec3 up_dir;

		const float h = 0.3f;
		const float w = 0.3f;

		position = glm::vec3((float) i / 10, 0.0f, (float) -1 * (i % 10));
		position /= 20.0f; // Put the blades closer together. Maybe I can put a density sort of factor...
		up_dir = glm::vec3(0, 1.0f, 0);
		blade = Blade(position, up_dir, PI + RANDOM_OFFSET, 0.5f, 0.1f);
		
		
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
	}
	else 
	{
		printf("Failed to load texture");
	}

	stbi_image_free(data);

	// Set up vertex data
	unsigned int rez = terrain->rez;
	
	std::vector<float> vertices;
	vertices.reserve(rez * rez * rez);

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
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(5);
	// texCoord attribute
	glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(6);

	glPatchParameteri(GL_PATCH_VERTICES, 4);
	CHECK_GL_ERROR;

}


void SceneManager::print_debug_statements(float delta_time)
{
	// Get and print values from compute shader.
	// Get values from compute shader

	std::vector<glm::vec4> compute_data(NUM_BLADES);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (void *) compute_data.data());
	
	// When I print this out I don't get anything
	for (glm::vec4 data_point : compute_data)
	{
		printf("%2f, %2f, %2f, %2f\n", data_point.r, data_point.g, data_point.b, data_point.a);
	}	
}

void SceneManager::app_logic(float delta_time)
{
	//static int j = 0;
	//if (j < 1000) print_debug_statements();
	//++j;

	// Prep data for uniforms
	glm::mat4 model(1.0f);

	//model = glm::translate(model, glm::vec3(0.5f, 0.0f, -2.0f));
	// model = glm::scale(model, glm::vec3(1.0f));
	glm::mat4 view = camera->GetViewMatrix();
	const float near = 0.1f;
	const float far = 100.0f;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)scr_width / (float)scr_height, near, far);

	const float ambient = 1.0f;
	const float diffuse = 0.6f;
	const float specular = 1.0f;
	const float specular_hardness = 600.0f;

	const glm::vec3 light_dir(0.7, -1.0f, 0.2f);
	const glm::vec3 light_colour(1.0f, 1.0f, 1.0f);

	// Compute shader loop
	compute_shader->use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressure_map);

	static float acc = 0;

	acc += delta_time;
	if (acc > 100.0f) {
		acc = 0;
	}

	glm::vec4 wind_data(1.0f, 0.0f, -0.8f, cos(acc) * 0.1f); // W component is wind strength	

	// Set compute shader uniforms
	compute_shader->setInt("amount_blades", NUM_BLADES);
	compute_shader->setFloat("dt", 1.0f);
	/*compute_shader->setFloat("dt", delta_time);*/
	compute_shader->setVec4("wind_data", wind_data);

	// Run the workers
	glDispatchCompute(NUM_BLADES, 1, 1);
	// Wait for workers to finish
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	grass_shader->use();
	grass_shader->setFloat("ambient", ambient);
	grass_shader->setFloat("diffuse", diffuse);
	grass_shader->setFloat("specular", specular);
	grass_shader->setFloat("specular_hardness", specular_hardness);

	grass_shader->setVec3("light_dir", light_dir);
	grass_shader->setVec3("light_colour", light_colour);
	grass_shader->setVec3("cam_pos", camera->Position);
	grass_shader->setVec2("near_far", glm::vec2(near, far));

	grass_shader->setMat4("model", model);
	grass_shader->setMat4("view", view);
	grass_shader->setMat4("projection", projection);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrain->texture);

	terrain->shader->use();
	terrain->shader->setInt("heightMap", 1);
	terrain->shader->setMat4("projection", projection);
	terrain->shader->setMat4("view", view);
	terrain->shader->setMat4("model", model);
}

void SceneManager::draw()
{
	grass_shader->use();
	glBindVertexArray(grass_vao);
	
	/*glDrawArraysIndirect(GL_PATCHES, reinterpret_cast<void*>(0));*/
	/*glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, 0);*/
	glDrawArrays(GL_PATCHES, 0, NUM_BLADES);
	glBindVertexArray(0);

	terrain->shader->use();
	glBindVertexArray(terrain->vao);
	glDrawArrays(GL_PATCHES, 0, 4 * terrain->rez * terrain->rez);
	glBindVertexArray(0);
}