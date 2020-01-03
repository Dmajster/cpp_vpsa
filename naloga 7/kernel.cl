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
