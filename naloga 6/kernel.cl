int getPixel(__local unsigned char image[100], int width, int height, int y, int x)
{
	if (x < 0 || x >= width)
		return 0;
	if (y < 0 || y >= height)
		return 0;
	return image[y*width + x];
}

__kernel void vector_add(
	__global unsigned char *input_image,
	__global unsigned char *output_image,
	int width,
	int height
)
{
	__local unsigned char localBuffer[100];

	int x = get_global_id(0);
	int y = get_global_id(1);

	int workgroup_x = get_local_id(0);
	int workgroup_y = get_local_id(1);

	int local_x = workgroup_x + 1;
	int local_y = workgroup_y + 1;

	if( y > height || x > width ){
		return;
	}

	localBuffer[local_y * 10 + local_x] = input_image[y * width + x];
	
	if(workgroup_x == 0)
	{
		localBuffer[local_y * 10 + local_x - 1] = input_image[y * width + x - 1];
	}
	if(workgroup_x == 7)
	{
		localBuffer[local_y * 10 + local_x + 1] = input_image[y * width + x + 1];
	}

	if(workgroup_y == 0)
	{
		localBuffer[(local_y-1) * 10 + local_x] = input_image[(y-1) * width + x];
	}
	if(workgroup_y == 7)
	{
		localBuffer[(local_y+1) * 10 + local_x] = input_image[(y+1) * width + x];
	}

	if(workgroup_x == 0 && workgroup_y == 0)
	{
		localBuffer[(local_y-1) * 10 + local_x - 1] = input_image[(y-1) * width + x - 1];
	}

	if(workgroup_x == 7 && workgroup_y == 0)
	{
		localBuffer[(local_y-1) * 10 + local_x + 1] = input_image[(y-1) * width + x + 1];
	}

	if(workgroup_x == 7 && workgroup_y == 7)
	{
		localBuffer[(local_y+1) * 10 + local_x + 1] = input_image[(y+1) * width + x + 1];
	}

	if(workgroup_x == 0 && workgroup_y == 7)
	{
		localBuffer[(local_y+1) * 10 + local_x - 1] = input_image[(y+1) * width + x - 1];
	}

	barrier(CLK_GLOBAL_MEM_FENCE);

	int Gx = -getPixel(localBuffer, 10, 10, local_y - 1, local_x - 1) 
		 -getPixel(localBuffer, 10, 10, local_y - 1, local_x) * 2
		 -getPixel(localBuffer, 10, 10, local_y - 1, local_x + 1) 
		 +getPixel(localBuffer, 10, 10, local_y + 1, local_x - 1) 
		 +getPixel(localBuffer, 10, 10, local_y + 1, local_x) * 2
		 +getPixel(localBuffer, 10, 10, local_y + 1, local_x + 1);
				
	int Gy = -getPixel(localBuffer, 10, 10, local_y - 1, local_x - 1) 
		 -getPixel(localBuffer, 10, 10, local_y, local_x - 1) * 2
		 -getPixel(localBuffer, 10, 10, local_y + 1, local_x - 1) 
		 +getPixel(localBuffer, 10, 10, local_y - 1, local_x + 1) 
		 +getPixel(localBuffer, 10, 10, local_y, local_x + 1) * 2
		 +getPixel(localBuffer, 10, 10, local_y + 1, local_x + 1);
			
	int tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));

	output_image[y * width + x] = tempPixel;
}
