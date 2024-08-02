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
	virtual void print_debug_statements(float delta_time = 0.0f) override;	

private:
	// Actual objects for whatever project or tutorial we are doing. This will change depending on
	// whatever we are doing.
	void init_grass();
	void init_terrain();	

	unsigned int grass_vao = 0;
	unsigned int n_grass_buffers = 4;
	unsigned int grass_vbo[4];
	Shader* grass_shader = nullptr;

	Shader* compute_shader = nullptr;
	unsigned int pressure_map = 0;

	struct Terrain
	{
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int texture = 0;
		unsigned int rez = 20;
		int width;
		int height;
		int n_channels;
		std::string heightmap_file = "iceland_heightmap.png";
		Shader* shader = nullptr;
	};

	Terrain* terrain;

};

#endif // SCENE_MANAGER_H