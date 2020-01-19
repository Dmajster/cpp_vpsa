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
	for (auto i = 0; i < (height); i++)
		for (auto j = 0; j < (width); j++)
		{
			histogram[imageIn[i * width + j]]++;
		}
}

void printHistogram(unsigned int* histogram) {
	printf("Barva\tPojavitve\n");
	for (auto i = 0; i < BINS; i++) {
		printf("%d\t%d\n", i, histogram[i]);
	}
}

struct ImageData
{
	unsigned int image_width;
	unsigned int image_height;
	unsigned char* pixels;
};

ImageData* load_image(const char* file_name)
{
	const auto image_bitmap = FreeImage_Load(FIF_PNG, file_name, 0);
	const auto image_bitmap_grey = FreeImage_ConvertToGreyscale(image_bitmap);

	const auto image_width = FreeImage_GetWidth(image_bitmap_grey);
	const auto image_height = FreeImage_GetHeight(image_bitmap_grey);

	const auto image_data = new ImageData{
		image_width,
		image_height,
		new unsigned char[image_width * image_height]
	};

	FreeImage_ConvertToRawBits(image_data->pixels, image_bitmap_grey, image_data->image_width, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Unload(image_bitmap_grey);
	FreeImage_Unload(image_bitmap);

	return image_data;
}



int main()
{
	auto image_information = load_image("slika.png");
	

	const auto bins = static_cast<unsigned int*>(calloc(BINS * sizeof(unsigned int), sizeof(unsigned int)));
	
	char* source_str;
	const auto fp = fopen("kernel.cl", "r");
	if (!fp)
	{
		fprintf(stderr, ":-(#\n");
		exit(1);
	}
	source_str = static_cast<char*>(malloc(MAX_SOURCE_SIZE));
	const auto source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);

	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	char* buf;
	auto ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10, device_id, &ret_num_devices);

	const auto context = clCreateContext(nullptr, 1, &device_id[0], nullptr, nullptr, &ret);

	const auto command_queue = clCreateCommandQueueWithProperties(context, device_id[0], nullptr, &ret);

	const size_t WORKGROUP_SIDE_LENGTH = sqrt(WORKGROUP_SIZE);

	size_t local_item_size[2] = { WORKGROUP_SIDE_LENGTH, WORKGROUP_SIDE_LENGTH };
	size_t global_item_size[2] = { ceil(image_information->image_width / static_cast<float>(local_item_size[0])) * local_item_size[0], ceil(image_information->image_height / static_cast<float>(local_item_size[1])) * local_item_size[1] };

	printf("local size: %zu, %zu\n", local_item_size[0], local_item_size[1]);
	printf("global size: %zu, %zu\n", global_item_size[0], global_item_size[1]);

	auto image_input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_information->image_width * image_information->image_height * sizeof(char), image_information->pixels, &ret);
	auto bins_output_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, BINS * sizeof(unsigned int), bins, &ret);

	const auto program = clCreateProgramWithSource(context, 1, const_cast<const char**>(&source_str), nullptr, &ret);

	ret = clBuildProgram(program, 1, &device_id[0], nullptr, nullptr, nullptr);
	printf("%d\n", ret);

	size_t build_log_len;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &build_log_len);
	printf("%d\n", ret);

	const auto build_log = static_cast<char*>(malloc(sizeof(char) * (build_log_len + 1)));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, nullptr);
	printf("%d\n", ret);
	printf("%s\n", build_log);
	free(build_log);

	const auto kernel = clCreateKernel(program, "local_histogram", &ret);

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, nullptr);
	printf("veckratnik niti = %d", buf_size_t);

	ret |= clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void*>(&image_input_mem_obj));
	printf("%d\n", ret);

	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), static_cast<void*>(&bins_output_mem_obj));
	printf("%d\n", ret);

	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), static_cast<void*>(&image_information->image_width));
	printf("%d\n", ret);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), static_cast<void*>(&image_information->image_height));
	printf("%d\n", ret);

	ret = clEnqueueWriteBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, image_information->image_width * image_information->image_height * sizeof(char), image_information->pixels, 0, nullptr, nullptr);

	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, nullptr, global_item_size, local_item_size, 0, nullptr, nullptr);
	printf("%d\n", ret);


	ret = clEnqueueReadBuffer(command_queue, bins_output_mem_obj, CL_TRUE, 0, BINS * sizeof(unsigned int), bins, 0, nullptr, nullptr);
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

	free(image_information->pixels);
	free(image_information);

	return 0;
}

