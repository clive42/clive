#include "rt.h"

cl_double3 *composite(cl_float3 **outputs, int numDevices, int resolution, cl_int **counts)
{
	cl_double3 *output_sum = calloc(resolution, sizeof(cl_double3));
	int *count_sum = calloc(resolution, sizeof(int));
	size_t sum = 0;
	for (int i = 0; i < numDevices; i++)
	{
		for (int j = 0; j < resolution; j++)
		{
			output_sum[j].x += (double)outputs[i][j].x;
			output_sum[j].y += (double)outputs[i][j].y;
			output_sum[j].z += (double)outputs[i][j].z;
			count_sum[j] += counts[i][j];
			sum += counts[i][j];
		}
		free(outputs[i]);
		free(counts[i]);
	}

	printf("%.1f avg samples/pixel\n", (float)sum / (float)(resolution));

	free(outputs);
	free(counts);
	// for (int i = 0; i < 10; i++)
	// 	printf("%lf %lf %lf\n", output_sum[i].x, output_sum[i].y, output_sum[i].z);

	//average samples and apply tone mapping
	double Lw = 0.0;
	int nan = 0;
	for (int j = 0;j < resolution; j++)
	{
		double scale = 1.0 / (double)(count_sum[j] == 0 ? 1 : (double)count_sum[j]);
		output_sum[j].x *= scale;
		output_sum[j].y *= scale;
		output_sum[j].z *= scale;

		double this_lw = log(0.1 + 0.2126 * output_sum[j].x + 0.7152 * output_sum[j].y + 0.0722 * output_sum[j].z);
		if (this_lw == this_lw)
			Lw += this_lw;
		else if (nan == 0)
		{
			printf("NaN alert\n");
			nan = 1;
		}
	}
	// printf("Lw is %lf\n", Lw);
	

	Lw /= (double)resolution;
	// printf("Lw is %lf\n", Lw);

	Lw = exp(Lw);
	// printf("Lw is %lf\n", Lw);

	for (int j = 0; j < resolution; j++)
	{
		output_sum[j].x = output_sum[j].x * 0.64 / Lw;
		output_sum[j].y = output_sum[j].y * 0.64 / Lw;
		output_sum[j].z = output_sum[j].z * 0.64 / Lw;

		output_sum[j].x = output_sum[j].x / (output_sum[j].x + 1.0);
		output_sum[j].y = output_sum[j].y / (output_sum[j].y + 1.0);
		output_sum[j].z = output_sum[j].z / (output_sum[j].z + 1.0);
	}
	printf("done composite\n");
	return output_sum;
}

cl_double3 *debug_composite(cl_float3 **outputs, int numDevices, int resolution, int **counts)
{
	cl_double3 *output_sum = calloc(resolution, sizeof(cl_double3));
	int *count_sum = calloc(resolution, sizeof(int));
	for (int i = 0; i < numDevices; i++)
	{
		for (int j = 0; j < resolution; j++)
		{
			count_sum[j] += counts[i][j];
			output_sum[j].x += (double)outputs[i][j].x;
			output_sum[j].y += (double)outputs[i][j].y;
			output_sum[j].z += (double)outputs[i][j].z;
		}
		free(outputs[i]);
	}
	free(outputs);
	free(counts);

	for (int j = 0;j < resolution; j++)
	{
		double scale = 1.0 / (double)(count_sum[j] * numDevices);
		output_sum[j].x *= scale;
		output_sum[j].y *= scale;
		output_sum[j].z *= scale;

		output_sum[j].x = output_sum[j].x / (1.0f + output_sum[j].x);
		output_sum[j].y = output_sum[j].y / (1.0f + output_sum[j].y);
		output_sum[j].z = output_sum[j].z / (1.0f + output_sum[j].z);
	}

	return output_sum;
}
