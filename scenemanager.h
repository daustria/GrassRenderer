#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H
#include "windowmanager.h"
#include "shader.h"
#include "blade.h"
#include <vector>

class SceneManager : public WindowManager
{
public:
	virtual void initialize() override;
	// stuff i can implement if i want
	//virtual void process_input(float delta_time) override;
	//virtual void mouse_event(double xPosIn, double yPosIn) override;
	//virtual void scroll_callback(double xoffset, double yoffset) override;

	// Drawing stuff
	virtual void app_logic(float delta_time) override;
	virtual void draw() override;

private:

	void init_grass();
	void init_terrain();	

	unsigned int grass_vao = 0;
	unsigned int grass_texture = 0;
	unsigned int n_grass_buffers = 4;
	unsigned int grass_vbo[4];
	Shader* grass_shader = nullptr;

	Shader* compute_shader = nullptr;
	unsigned int pressure_map = 0;

	struct Terrain
	{
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int heightmap_texture = 0;
		unsigned int albedo_texture = 0;
		unsigned int metal_texture = 0;
		unsigned int ao_texture = 0;
		unsigned int roughness_texutre = 0;
		unsigned int normal_texture = 0;
		unsigned int rez = 30;
		int width = 0;
		int height = 0;
		int n_channels = 0;
		std::string heightmap_file;
		Shader* shader = nullptr;

		glm::vec4 heightmap_bounds; // xMin, zMin, xLength, zLength
	};

	Terrain* terrain;

};

#endif // SCENE_MANAGER_H