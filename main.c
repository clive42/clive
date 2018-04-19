#include "rt.h"

#ifndef __APPLE__
# define quit_key 113
#else
#endif

void	sum_color(cl_double3 *a, cl_float3 *b, int size)
{
	for (int i = 0; i < size; i++)
	{
		a[i].x += b[i].x;
		a[i].y += b[i].y;
		a[i].z += b[i].z;
	}
}

void add_counts(int *total, int *new, int size)
{
	for (int i = 0; i < size; i++)
		total[i] += new[i];
}

void		path_tracer(t_env *env)
{
	int first = (env->samples == 0) ? 1 : 0;
	env->samples += 1;
	set_camera(&env->cam, DIM_PT);
	cl_int *count = NULL;
	cl_float3 *pix = gpu_render(env->scene, env->cam, DIM_PT, DIM_PT, 1, env->min_bounces, first, &count);
	sum_color(env->pt->total_clr, pix, DIM_PT * DIM_PT);
	free(pix);
	add_counts(env->pt->count, count, DIM_PT * DIM_PT);
	free(count);
	alt_composite(env->pt, DIM_PT * DIM_PT, env->pt->count);
	draw_pixels(env->pt, DIM_PT, DIM_PT);
	mlx_put_image_to_window(env->mlx, env->pt->win, env->pt->img, 0, 0);
	mlx_key_hook(env->pt->win, exit_hook, env);
	if (env->samples >= env->spp)
	{
		env->samples = 0;
		env->render = 0;
		for (int i = 0; i < DIM_PT * DIM_PT; i++)
		{
			env->pt->total_clr[i].x = 0;
			env->pt->total_clr[i].y = 0;
			env->pt->total_clr[i].z = 0;
		}
	}
}

void		init_mlx_data(t_env *env)
{
	env->mlx = mlx_init();

	env->pt = malloc(sizeof(t_mlx_data));
	env->pt->bpp = 0;
	env->pt->size_line = 0;
	env->pt->endian = 0;
	env->pt->win = mlx_new_window(env->mlx, DIM_PT, DIM_PT, "CLIVE - Path Tracer");
	env->pt->img = mlx_new_image(env->mlx, DIM_PT, DIM_PT);
	env->pt->imgbuff = mlx_get_data_addr(env->pt->img, &env->pt->bpp, &env->pt->size_line, &env->pt->endian);
	env->pt->bpp /= 8;
	env->pt->pixels = calloc(DIM_PT * DIM_PT, sizeof(cl_double3));
	env->pt->total_clr = calloc(DIM_PT * DIM_PT, sizeof(cl_double3));
	env->pt->count = calloc(DIM_PT * DIM_PT, sizeof(cl_int));
	
	if (env->mode == IA)
	{
		env->ia = malloc(sizeof(t_mlx_data));
		env->ia->bpp = 0;
		env->ia->size_line = 0;
		env->ia->endian = 0;
		env->ia->win = mlx_new_window(env->mlx, DIM_IA, DIM_IA, "CLIVE - Interactive Mode");
		env->ia->img = mlx_new_image(env->mlx, DIM_IA, DIM_IA);
		env->ia->imgbuff = mlx_get_data_addr(env->ia->img, &env->ia->bpp, &env->ia->size_line, &env->ia->endian);
		env->ia->bpp /= 8;
		env->ia->pixels = calloc(DIM_IA * DIM_IA, sizeof(cl_double3));
		env->ia->total_clr = NULL;
		mlx_hook(env->ia->win, 2, 0, key_press, env);
		mlx_hook(env->ia->win, 3, 0, key_release, env);
		mlx_hook(env->ia->win, 17, 0, exit_hook, env);
	}

	mlx_loop_hook(env->mlx, forever_loop, env);
	mlx_loop(env->mlx);
}

t_env		*init_env(void)
{
	t_env	*env = malloc(sizeof(t_env));
	env->cam = init_camera();
	//load camera settings from config file and import scene
	env->mode = PT;
	env->view = 1;
	env->show_fps = 0;
	env->key.w = 0;
	env->key.a = 0;
	env->key.s = 0;
	env->key.d = 0;
	env->key.uarr = 0;
	env->key.darr = 0;
	env->key.larr = 0;
	env->key.rarr = 0;
	env->key.space = 0;
	env->key.shift = 0;
	env->samples = 0;
	env->render = 1;
	env->eps = 0.00005;
	load_config(env);
	return env;
}

int 		main(int ac, char **av)
{
	srand(time(NULL));

	//Initialize environment with scene and intial configurations
	t_env	*env = init_env();

	//LL is best for this bvh. don't want to rearrange import for now, will do later
	Face *face_list = NULL;
	for (int i = 0; i < env->scene->face_count; i++)
	{
		Face *f = calloc(1, sizeof(Face));
		memcpy(f, &env->scene->faces[i], sizeof(Face));
		f->next = face_list;
		face_list = f;
	}
	free(env->scene->faces);

	//Build BVH
	int box_count, ref_count;
	AABB *tree = sbvh(face_list, &box_count, &ref_count);
	printf("finished with %d boxes\n", box_count);
	study_tree(tree, 100000);

	//Flatten BVH
	env->scene->bins = tree;
	env->scene->bin_count = box_count;
	env->scene->face_count = ref_count;
	flatten_faces(env->scene);

	// for (int i = 0; i < env->scene->face_count; i++)
	// {
	// 	Face *tmp = face_list->next;
	// 	free(face_list);
	// 	face_list = tmp;
	// }

	init_mlx_data(env);
	
	return 0;
}
