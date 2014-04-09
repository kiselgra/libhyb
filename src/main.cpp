#include <libcgl/libcgl.h>

#include <GL/freeglut.h>
#include <iostream>

#include "cmdline.h"

using namespace std;

mesh_ref cube;
shader_ref cube_shader;

void display() {
	glClearColor(0,0,0.25,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);


	bind_shader(cube_shader);

	uniform_matrix4x4f(cube_shader, "proj", projection_matrix_of_cam(current_camera()));
	uniform_matrix4x4f(cube_shader, "view", gl_view_matrix_of_cam(current_camera()));

	matrix4x4f model;
	make_unit_matrix4x4f(&model);
	uniform_matrix4x4f(cube_shader, "model", &model);

	uniform3f(cube_shader, "light_dir", 0, -1, -0.2);
	uniform3f(cube_shader, "light_col", 1, 0.9, 0.1);
	uniform3f(cube_shader, "color", 0.9, 0.9, 0.9);

	bind_mesh_to_gl(cube);
	draw_mesh(cube, GL_TRIANGLES);
	unbind_mesh_from_gl(cube);


	unbind_shader(cube_shader);



	swap_buffers();
}

void idle() {
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'c') {
		vec3f cam_pos, cam_dir, cam_up;
		matrix4x4f *lookat_matrix = lookat_matrix_of_cam(current_camera());
		extract_pos_vec3f_of_matrix(&cam_pos, lookat_matrix);
		extract_dir_vec3f_of_matrix(&cam_dir, lookat_matrix);
		extract_up_vec3f_of_matrix(&cam_up, lookat_matrix);
		cout << "--pos " << cam_pos.x << "," << cam_pos.y << "," << cam_pos.z << " ";
		cout << "--dir " << cam_dir.x << "," << cam_dir.y << "," << cam_dir.z << " ";
		cout << "--up " << cam_up.x << "," << cam_up.y << "," << cam_up.z << endl;
	}
	else standard_keyboard(key, x, y);
}

void actual_main() 
{
	register_display_function(display);
	register_idle_function(idle);
	register_keyboard_function(keyboard);

    cube = make_cube("test cube", 0);
    cube_shader = find_shader("diffuse-pl");

	enter_glut_main_loop();

}

int main(int argc, char **argv)
{	
	parse_cmdline(argc, argv);
	
	std::string home(getenv("HOME"));
	append_image_path((home+"/render-data/images/").c_str());

	int guile_mode = guile_cfg_only;
	startup_cgl("name", 3, 3, argc, argv, 1366, 768, actual_main, guile_mode, false, "default.scm");

	return 0;
}


