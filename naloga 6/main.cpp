// "s"cepec preberemo iz datoteke
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <cmath>
#include <FreeImage.h>

#define WORKGROUP_SIZE	(64)
#define MAX_SOURCE_SIZE	16384

int main()
{

	unsigned char* slikaInput;
	unsigned char* slikaOutput;

	FIBITMAP* imageBitmap = FreeImage_Load(FIF_PNG, "slika2.png", 0);
	FIBITMAP* imageBitmapGrey = FreeImage_ConvertToGreyscale(imageBitmap);

	int image_width = FreeImage_GetWidth(imageBitmapGrey);
	int image_height = FreeImage_GetHeight(imageBitmapGrey);
	int imagePixelCount = image_width * image_height;

	unsigned char* image_input = (unsigned char*)malloc(imagePixelCount * sizeof(char));
	unsigned char* image_output = (unsigned char*)malloc(imagePixelCount * sizeof(char));
	
	FreeImage_ConvertToRawBits(image_input, imageBitmapGrey, image_width, 8, 0xFF, 0xFF, 0xFF, TRUE);

	FreeImage_Unload(imageBitmapGrey);
	FreeImage_Unload(imageBitmap);
	
	char ch;
	int i;
	cl_int ret;

	FILE* fp;
	char* source_str;
	size_t source_size;

	fp = fopen("kernel.cl", "r");
	if (!fp)
	{
		fprintf(stderr, ":-(#\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);
	

	
	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	char* buf;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,device_id, &ret_num_devices);

	cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);

	cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id[0], 0, &ret);

	size_t WORKGROUP_SIDE_LENGTH = sqrt(WORKGROUP_SIZE);

	size_t local_item_size[2] = { WORKGROUP_SIDE_LENGTH, WORKGROUP_SIDE_LENGTH };
	size_t global_item_size[2] = { ceil(image_width / (float)local_item_size[0]) * local_item_size[0], ceil(image_height / (float)local_item_size[1]) * local_item_size[1] };

	printf("local size: %zu, %zu\n", local_item_size[0], local_item_size[1]);
	printf("global size: %zu, %zu\n", global_item_size[0], global_item_size[1]);

	cl_mem image_input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imagePixelCount * sizeof(char), image_input, &ret);
	cl_mem image_output_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, imagePixelCount * sizeof(char), NULL, &ret);

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source_str, NULL, &ret);

	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
	printf("%d\n", ret);

	size_t build_log_len;
	char* build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
	printf("%d\n", ret);
	
	build_log = (char*)malloc(sizeof(char) * (build_log_len + 1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, NULL);
	printf("%d\n", ret);
	printf("%s\n", build_log);
	free(build_log);

	cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);
	printf("veckratnik niti = %d", buf_size_t);

	ret |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&image_input_mem_obj);
	printf("%d\n", ret);
	
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&image_output_mem_obj);
	printf("%d\n", ret);
	
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&image_width);
	printf("%d\n", ret);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&image_height);
	printf("%d\n", ret);

	ret = clEnqueueWriteBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, imagePixelCount * sizeof(char), image_input, 0, NULL, NULL);
	
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_item_size, local_item_size, 0, NULL, NULL);
	printf("%d\n", ret);

	
	ret = clEnqueueReadBuffer(command_queue, image_output_mem_obj, CL_TRUE, 0, imagePixelCount * sizeof(char), image_output, 0, NULL, NULL);
	printf("%d\n", ret);

	
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(image_input_mem_obj);
	ret = clReleaseMemObject(image_output_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	FIBITMAP* imageOutBitmap = FreeImage_ConvertFromRawBits(image_output, image_width, image_height, image_width, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, imageOutBitmap, "sobel2_slika.png", 0);
	FreeImage_Unload(imageOutBitmap);

	free(image_input);

	return 0;
}

