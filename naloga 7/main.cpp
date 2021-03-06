// "s"cepec preberemo iz datoteke
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <cmath>
#include <FreeImage.h>

#define WORKGROUP_SIZE	(64)
#define MAX_SOURCE_SIZE	16384

#define BINS 256

void HistogramCPU(unsigned char* imageIn, unsigned int* histogram, int width, int height)
{
	memset(histogram, 0, BINS * sizeof(unsigned int));

	//za vsak piksel v sliki
	for (int i = 0; i < (height); i++)
		for (int j = 0; j < (width); j++)
		{
			histogram[imageIn[i * width + j]]++;
		}
}

void printHistogram(unsigned int* histogram) {
	printf("Barva\tPojavitve\n");
	for (int i = 0; i < BINS; i++) {
		printf("%d\t%d\n", i, histogram[i]);
	}
}

int main()
{
	FIBITMAP* imageBitmap = FreeImage_Load(FIF_PNG, "slika.png", 0);
	FIBITMAP* imageBitmapGrey = FreeImage_ConvertToGreyscale(imageBitmap);

	int image_width = FreeImage_GetWidth(imageBitmapGrey);
	int image_height = FreeImage_GetHeight(imageBitmapGrey);
	int imagePixelCount = image_width * image_height;

	unsigned char* image_input = (unsigned char*)malloc(imagePixelCount * sizeof(char));
	unsigned int* bins = (unsigned int*)calloc(BINS * sizeof(unsigned int), sizeof(unsigned int));

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
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10, device_id, &ret_num_devices);

	cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);

	cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id[0], 0, &ret);

	size_t WORKGROUP_SIDE_LENGTH = sqrt(WORKGROUP_SIZE);

	size_t local_item_size[2] = { WORKGROUP_SIDE_LENGTH, WORKGROUP_SIDE_LENGTH };
	size_t global_item_size[2] = { ceil(image_width / (float)local_item_size[0]) * local_item_size[0], ceil(image_height / (float)local_item_size[1]) * local_item_size[1] };

	printf("local size: %zu, %zu\n", local_item_size[0], local_item_size[1]);
	printf("global size: %zu, %zu\n", global_item_size[0], global_item_size[1]);

	cl_mem image_input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imagePixelCount * sizeof(char), image_input, &ret);
	cl_mem bins_output_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, BINS * sizeof(unsigned int), bins, &ret);

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

	cl_kernel kernel = clCreateKernel(program, "local_histogram", &ret);

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);
	printf("veckratnik niti = %d", buf_size_t);

	ret |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&image_input_mem_obj);
	printf("%d\n", ret);

	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bins_output_mem_obj);
	printf("%d\n", ret);

	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&image_width);
	printf("%d\n", ret);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&image_height);
	printf("%d\n", ret);

	ret = clEnqueueWriteBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, imagePixelCount * sizeof(char), image_input, 0, NULL, NULL);

	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_item_size, local_item_size, 0, NULL, NULL);
	printf("%d\n", ret);


	ret = clEnqueueReadBuffer(command_queue, bins_output_mem_obj, CL_TRUE, 0, BINS * sizeof(unsigned int), bins, 0, NULL, NULL);
	printf("%d\n", ret);

	//HistogramCPU(image_input, bins, image_width, image_height);
	
	printHistogram(bins);

	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(image_input_mem_obj);
	ret = clReleaseMemObject(bins_output_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	free(image_input);

	return 0;
}

