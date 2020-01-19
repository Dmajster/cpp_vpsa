__constant unsigned int gray_levels = 256;

__kernel void histogram(
	__global unsigned char* input_image,
	__global unsigned int* output_bins,
	int width,
	int height
)
{
	__local unsigned int localBuffer[gray_levels];

	int x = get_global_id(0);
	int y = get_global_id(1);

	int x_size = get_global_size(0);

	int workgroup_x = get_local_id(0);
	int workgroup_y = get_local_id(1);
	int workgroup_width = get_local_size(0);
	int workgroup_height = get_local_size(1);


	for(int i = workgroup_y * workgroup_width + workgroup_x;i < gray_levels;i+= workgroup_width * workgroup_height )
	{
		localBuffer[i] = 0;
	}

	barrier(CLK_GLOBAL_MEM_FENCE);

	if( y > height || x > width ){
		return;
	}

	int value = input_image[y * width + x];

	atomic_add(&localBuffer[value], 1);

	barrier(CLK_GLOBAL_MEM_FENCE);

	for(int i = workgroup_y * workgroup_width + workgroup_x;i < gray_levels;i+= workgroup_width * workgroup_height )
	{
		atomic_add(&output_bins[i], localBuffer[i]);
	}
}

__kernel void cdf(
	__global unsigned int* histogram,
	__global unsigned int* distribution
)
{
	__local float temp[gray_levels];

	int thid = get_local_id(0);

	int offset = 1;
	temp[2 * thid] = histogram[2 * thid];     
	temp[2 * thid + 1] = histogram[2 * thid + 1];
	for (int d = gray_levels >> 1; d > 0; d >>= 1)
	{
		barrier(CLK_GLOBAL_MEM_FENCE);

		if (thid < d)
		{
			int ai = offset * (2 * thid + 1) - 1; 
			int bi = offset * (2 * thid + 2) - 1;             
			temp[bi] += temp[ai];
		}         
		
		offset *= 2;
	}
	if (thid == 0)
	{
		temp[gray_levels - 1] = 0;
	}

	for (int d = 1; d < gray_levels; d *= 2)    
	{
		offset >>= 1;

		barrier(CLK_GLOBAL_MEM_FENCE);

		if (thid < d)
		{
			int ai = offset * (2 * thid + 1) - 1;
			int bi = offset * (2 * thid + 2) - 1;
			float t = temp[ai];
			temp[ai] = temp[bi];
			temp[bi] += t;
		}
	}

	barrier(CLK_GLOBAL_MEM_FENCE);

	distribution[2 * thid] = temp[2 * thid];
	distribution[2 * thid + 1] = temp[2 * thid + 1];
}



unsigned int findMin(__global unsigned int* histogram)
{
	unsigned int min = 0;

	for (int i = 0; min == 0 && i < gray_levels; i++)
	{
		min = histogram[i];
	}

	return min;
}

unsigned char scale(unsigned int cdf, unsigned int cdfmin, unsigned int imageSize)
{
	float scale = (float)(cdf - cdfmin) / (float)(imageSize - cdfmin);
	scale = round(scale * (float)(gray_levels - 1));
	return (char)scale;
}

__kernel void equalize(
	__global unsigned int* cdf,
	__global unsigned char* image,
	unsigned int width,
	unsigned int heigth
)
{
	unsigned int imageSize = width * heigth;
	unsigned int cdfmin = findMin(cdf);

	int x = get_global_id(0);
	int y = get_global_id(1);

	image[y * width + x] = scale(cdf[image[y*width + x]], cdfmin , imageSize);
}