#include <libcgls/cgls.h>
#include <libcgls/picking.h>
#include <libcgls/interaction.h>
#include <libcgl/wall-time.h>

#include "cmdline.h"

#include <GL/freeglut.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

scene_ref the_scene;
#define samples 128
float times[samples];
int valid_pos = 0, curr_pos = 0;

framebuffer_ref gbuffer;
picking_buffer_ref picking;

void display() {
	glDisable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEPTH_TEST);
	
    glFinish();
	wall_time_t start = wall_time_in_ms();

    bind_framebuffer(gbuffer);

	glClearColor(0,0,0.25,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	render_scene(the_scene);
    unbind_framebuffer(gbuffer);

	render_scene_deferred(the_scene, gbuffer);

	glFinish();
	wall_time_t end = wall_time_in_ms();

	times[curr_pos] = end-start;
	curr_pos = (curr_pos+1) % samples;
	valid_pos = (valid_pos == samples ? samples : valid_pos+1);

    check_for_gl_errors("end of display");

	swap_buffers();
}

void idle() {
	glutPostRedisplay(); 
}

void show_fps(interaction_mode *m, int x, int y) {
	double sum = 0;
	for (int i = 0; i < valid_pos; ++i)
		sum += times[i];
	float avg = sum / (double)valid_pos;
	printf("average render time: %.3f ms, %.1f fps \t(sum %f, n %d)\n", avg, 1000.0f/avg, (float)sum, valid_pos);
}

void advance_anim(interaction_mode *m, int x, int y) {
	skeletal_animation_ref ar = { 0 };
	static float time = 0;
	time += 0.01;

	evaluate_skeletal_animation_at(ar, time);
}

interaction_mode* make_viewer_mode() {
	interaction_mode *m = make_interaction_mode("viewer");
	add_function_key_to_mode(m, 'p', cgls_interaction_no_button, show_fps);
	add_function_key_to_mode(m, ' ', cgls_interaction_no_button, advance_anim);
	return m;
}

void adjust_view(const vec3f *bb_min, const vec3f *bb_max, vec3f *cam_pos, float *distance) {
	vec3f bb_center, tmp;
	sub_components_vec3f(&tmp, bb_max, bb_min);
	div_vec3f_by_scalar(&tmp, &tmp, 2);
	add_components_vec3f(&bb_center, &tmp, bb_min);
	
	sub_components_vec3f(&tmp, bb_max, bb_min);
	*distance = length_of_vec3f(&tmp);
	make_vec3f(&tmp, 0, 0, *distance);
	add_components_vec3f(cam_pos, &bb_center, &tmp);

	cgl_cam_move_factor = *distance / 20.0f;
}

extern "C" {
void register_scheme_functions_for_cmdline();
}
static void register_scheme_functions() {
	register_scheme_functions_for_cmdline();
}

void actual_main() 
{
	dump_gl_info();
	for (int i = 0; i < samples; ++i)
		times[i] = 0.0f;

	register_display_function(display);
	register_idle_function(idle);

	register_cgls_scheme_functions();
	register_scheme_functions();

	initialize_interaction();
	push_interaction_mode(make_default_cgls_interaction_mode());
	push_interaction_mode(make_viewer_mode());

    gbuffer = make_stock_deferred_buffer("gbuffer", cmdline.res.x, cmdline.res.y, GL_RGBA8, GL_RGBA8, GL_RGBA16F, GL_RGBA32F, GL_DEPTH_COMPONENT24);

	for (list<string>::iterator it = cmdline.image_paths.begin(); it != cmdline.image_paths.end(); ++it)
		append_image_path(it->c_str());

	if (cmdline.config != "") {
		char *config = 0;
		int n = asprintf(&config, "%s/%s", cmdline.include_path, cmdline.config);
		load_configfile(config);
		free(config);
		scene_ref scene = { 0 };
		the_scene = scene;
	}
	else
		cerr << "no config file given" << endl;
	
	struct drawelement_array picking_des = make_drawelement_array();
	push_drawelement_list_to_array(scene_drawelements(the_scene), &picking_des);
	picking = make_picking_buffer("pick", &picking_des, cmdline.res.x, cmdline.res.y);
	push_interaction_mode(make_blender_style_interaction_mode(the_scene, picking));
	
	vec3f up = vec3f(0, 1, 0);
	light_ref hemi = make_hemispherical_light("hemi", gbuffer, &up);
	change_light_color3f(hemi, .5, .5, .5);
	add_light_to_scene(the_scene, hemi);

	vec3f pos = vec3f(300, 200, 0);
	vec3f dir = vec3f(1, 0, 0);
	light_ref spot = make_spotlight("spot", gbuffer, &pos, &dir, &up, 30);
	add_light_to_scene(the_scene, spot);
	push_drawelement_to_array(light_representation(spot), &picking_des);

	scene_set_lighting(the_scene, apply_deferred_lights);

	finalize_single_material_passes_for_array(&picking_des);

	enter_glut_main_loop();
}

int main(int argc, char **argv)
{	
	parse_cmdline(argc, argv);
	
// 	int guile_mode = guile_cfg_only;
	int guile_mode = with_guile;
	startup_cgl("name", 4, 2, argc, argv, (int)cmdline.res.x, (int)cmdline.res.y, actual_main, guile_mode, false, 0);

	return 0;
}

