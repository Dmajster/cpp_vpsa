__kernel void histogram(
	__global unsigned char* input_image,
	__global unsigned int* output_bins,
	int width,
	int height
)
{
	__local unsigned int localBuffer[256];

	int x = get_global_id(0);
	int y = get_global_id(1);

	int x_size = get_global_size(0);

	int workgroup_x = get_local_id(0);
	int workgroup_y = get_local_id(1);

	if( workgroup_x == 0 && workgroup_y == 0)
	{
		for(int i = 0;i < 256;i++ )
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
		for(int i = 0;i < 256;i++ )
		{
			atomic_add(&output_bins[i], localBuffer[i]);
		}
	}
}

__kernel void cdf(
	__global unsigned int* g_idata,
	__global unsigned int* g_odata,
	int n
)
{
	__local float temp[256];

	int thid = get_local_id(0);

	int offset = 1;
	temp[2 * thid] = g_idata[2 * thid];     
	temp[2 * thid + 1] = g_idata[2 * thid + 1];
	for (int d = n >> 1; d > 0; d >>= 1)
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
		temp[n - 1] = 0;
	}

	for (int d = 1; d < n; d *= 2)    
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

	g_odata[2 * thid] = temp[2 * thid];
	g_odata[2 * thid + 1] = temp[2 * thid + 1];
}



unsigned int findMin(__global unsigned int* histogram)
{
	unsigned int min = 0;

	for (int i = 0; min == 0 && i < 256; i++)
	{
		min = histogram[i];
	}

	return min;
}

unsigned char scale(unsigned int cdf, unsigned int cdfmin, unsigned int imageSize)
{
	float scale = (float)(cdf - cdfmin) / (float)(imageSize - cdfmin);
	scale = round(scale * (float)(256.0 - 1));
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