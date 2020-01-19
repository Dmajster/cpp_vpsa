__kernel void global_histogram(
	__global unsigned char* input_image,
	__global unsigned int* output_bins,
	int width,
	int height
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	if( y > height || x > width ){
		return;
	}

	int value = input_image[y * width + x];

	atomic_add(&output_bins[value], 1);
}

__kernel void local_histogram(
	__global unsigned char* input_image,
	__global unsigned int* output_bins,
	int width,
	int height
)
{
	__local unsigned int localBuffer[256];

	int x = get_global_id(0);
	int y = get_global_id(1);

	int workgroup_x = get_local_id(0);
	int workgroup_y = get_local_id(1);

	if( workgroup_x == 0 && workgroup_y == 0)
	{
		for(int i = 0; i < 256; i++ )
		{
			localBuffer[i] = 0;
		}
	}

	barrier(CLK_GLOBAL_MEM_FENCE);

	if( y > height || x > width ){
		return;
	}

	int value = input_image[y * width + x];

	atomic_add(&localBuffer[value], 1);

	barrier(CLK_GLOBAL_MEM_FENCE);

	if( workgroup_x == 0 && workgroup_y == 0)
	{
		for(int i = 0; i < 256; i++ )
		{
			atomic_add(&output_bins[i], localBuffer[i]);
		}
	}
}