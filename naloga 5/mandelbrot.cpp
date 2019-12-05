#include <stdio.h>
#include <stdlib.h>
#include "FreeImage.h"
#include <math.h>
#include <cmath>


void mandelbrotCPU(unsigned char* image, int height, int width) {
	const auto max_iteration = 800;   //max stevilo iteracij
	const unsigned char max = 255;   //max vrednost barvnega kanala

	//za vsak piksel v sliki							
	for (auto i = 0; i < height; i++)
		for (auto j = 0; j < width; j++)
		{
			const auto x0 = static_cast<float>(j) / width * static_cast<float>(3.5) - static_cast<float>(2.5); //zacetna vrednost
			const auto y0 = static_cast<float>(i) / height * static_cast<float>(2.0) - static_cast<float>(1.0);
			float x = 0;
			float y = 0;
			auto iter = 0;
			//ponavljamo, dokler ne izpolnemo enega izmed pogojev
			while ((x * x + y * y <= 4) && (iter < max_iteration))
			{
				const auto xtemp = x * x - y * y + x0;
				y = 2 * x * y + y0;
				x = xtemp;
				iter++;
			}
			//izracunamo barvo (magic: http://linas.org/art-gallery/escape/smooth.html)
			int color = 1.0 + iter - log(log(std::sqrt(x * x + y * y))) / log(2.0);
			color = (8 * max * color) / max_iteration;
			if (color > max)
				color = max;
			//zapisemo barvo RGBA (v resnici little endian BGRA)
			image[4 * i * width + 4 * j + 0] = color; //Blue
			image[4 * i * width + 4 * j + 1] = color; // Green
			image[4 * i * width + 4 * j + 2] = color; // Red
			image[4 * i * width + 4 * j + 3] = 255;   // Alpha
		}
}


//int main(void)
//{
//	printf("Start");
//
//	int height = 1024;
//	int width = 1024;
//	int pitch = ((32 * width + 31) / 32) * 4;
//
//	//rezerviramo prostor za sliko (RGBA)
//	unsigned char* image = (unsigned char*)malloc(height * width * sizeof(unsigned char) * 4);
//
//	mandelbrotCPU(image, height, width);
//
//	//shranimo sliko
//	FIBITMAP* dst = FreeImage_ConvertFromRawBits(image, width, height, pitch,
//		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
//	FreeImage_Save(FIF_PNG, dst, "mandelbrot.png", 0);
//	return 0;
//}
