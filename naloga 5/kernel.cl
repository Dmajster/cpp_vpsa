__kernel void vector_add(
	__global char *image,
	int width,
	int height
)
{
	const int max_iteration = 800;   //max stevilo iteracij
	const unsigned char max = 255;   //max vrednost barvnega kanala

	int i = get_global_id(1);
	int j = get_global_id(0);

	if( i > height || j > width ){
		return;
	}

	const float x0 = (float)j / width * (float)3.5 - (float)2.5; //zacetna vrednost
	const float y0 = (float)i / height * (float)2.0 - (float)1.0;
	float x = 0;
	float y = 0;
	int iter = 0;
	//ponavljamo, dokler ne izpolnemo enega izmed pogojev
	while ((x * x + y * y <= 4) && (iter < max_iteration))
	{
		const float xtemp = x * x - y * y + x0;
		y = 2 * x * y + y0;
		x = xtemp;
		iter++;
	}
	//izracunamo barvo (magic: http://linas.org/art-gallery/escape/smooth.html)
	int color = 1.0 + iter - log(log(sqrt(x * x + y * y))) / log(2.0);
	color = (8 * max * color) / max_iteration;
	if (color > max)
		color = max;
	//zapisemo barvo RGBA (v resnici little endian BGRA)
	image[4 * i * width + 4 * j + 0] = color; //Blue
	image[4 * i * width + 4 * j + 1] = color; // Green
	image[4 * i * width + 4 * j + 2] = color; // Red
	image[4 * i * width + 4 * j + 3] = 255;   // Alpha
}