#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "FreeImage.h"


inline int getPixel(unsigned char* image, int width, int height, int y, int x)
{
	if (x < 0 || x >= width)
		return 0;
	if (y < 0 || y >= height)
		return 0;
	return image[y * width + x];
}

void sobelCPU(unsigned char* imageIn, unsigned char* imageOut, int width, int height)
{
	int i, j;
	int Gx, Gy;
	int tempPixel;

	//za vsak piksel v sliki
	for (i = 0; i < (height); i++)
		for (j = 0; j < (width); j++)
		{
			Gx = -getPixel(imageIn, width, height, i - 1, j - 1) - 2 * getPixel(imageIn, width, height, i - 1, j) -
				getPixel(imageIn, width, height, i - 1, j + 1) + getPixel(imageIn, width, height, i + 1, j - 1) +
				2 * getPixel(imageIn, width, height, i + 1, j) + getPixel(imageIn, width, height, i + 1, j + 1);
			Gy = -getPixel(imageIn, width, height, i - 1, j - 1) - 2 * getPixel(imageIn, width, height, i, j - 1) -
				getPixel(imageIn, width, height, i + 1, j - 1) + getPixel(imageIn, width, height, i - 1, j + 1) +
				2 * getPixel(imageIn, width, height, i, j + 1) + getPixel(imageIn, width, height, i + 1, j + 1);
			tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
			if (tempPixel > 255)
				imageOut[i * width + j] = 255;
			else
				imageOut[i * width + j] = tempPixel;
		}
}


//int main()
//{
//	unsigned char* slikaInput;
//	unsigned char* slikaOutput;
//
//	FIBITMAP* imageBitmap = FreeImage_Load(FIF_PNG, "slika2.png", 0);
//	FIBITMAP* imageBitmapGrey = FreeImage_ConvertToGreyscale(imageBitmap);
//	int width = FreeImage_GetWidth(imageBitmapGrey);
//	int height = FreeImage_GetHeight(imageBitmapGrey);
//
//
//	unsigned char* imageIn = (unsigned char*)malloc(height * width * sizeof(unsigned char));
//	unsigned char* imageOut = (unsigned char*)malloc(height * width * sizeof(unsigned char));
//
//	FreeImage_ConvertToRawBits(imageIn, imageBitmapGrey, width, 8, 0xFF, 0xFF, 0xFF, TRUE);
//
//	FreeImage_Unload(imageBitmapGrey);
//	FreeImage_Unload(imageBitmap);
//
//
//	sobelCPU(imageIn, imageOut, width, height);
//	FIBITMAP* imageOutBitmap = FreeImage_ConvertFromRawBits(imageOut, width, height, width, 8, 0xFF, 0xFF, 0xFF, TRUE);
//	FreeImage_Save(FIF_PNG, imageOutBitmap, "sobel2_slika.png", 0);
//	FreeImage_Unload(imageOutBitmap);
//
//	return 0;
//}