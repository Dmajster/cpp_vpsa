#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <CL/cl.h>
#include <cmath>
#include <FreeImage.h>
#include <ctime>
#include <ostream>
#include <iostream>

constexpr auto workgroup_size = 64;
constexpr auto max_source_size = 16384;

struct image_data
{
	unsigned int width;
	unsigned int height;
	unsigned char* pixels;
};

image_data* load_image(const char* file_name)
{
	const auto image_bitmap = FreeImage_Load(FIF_PNG, file_name, 0);
	const auto image_bitmap_grey = FreeImage_ConvertToGreyscale(image_bitmap);

	const auto image_width = FreeImage_GetWidth(image_bitmap_grey);
	const auto image_height = FreeImage_GetHeight(image_bitmap_grey);

	const auto image = new image_data{
		image_width,
		image_height,
		new unsigned char[image_height * image_width]
	};

	FreeImage_ConvertToRawBits(image->pixels, image_bitmap_grey, image->width, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Unload(image_bitmap_grey);
	FreeImage_Unload(image_bitmap);

	return image;
}

char* load_kernel_source(const char* file_name)
{
	const auto file = fopen(file_name, "r");
	if (!file)
	{
		fprintf(stderr, "Failed to open kernel file: %s \n", file_name);
		exit(1);
	}

	const auto source_string = static_cast<char*>(malloc(max_source_size));
	const auto source_size = fread(source_string, 1, max_source_size, file);
	source_string[source_size] = '\0';
	fclose(file);

	return source_string;
}

int main()
{
	const auto gray_levels = 256;
	
	auto image = load_image("./images/image2.png");
	auto kernel_source = load_kernel_source("kernel.cl");

	const auto image_in_bitmap = FreeImage_ConvertFromRawBits(image->pixels, image->width, image->height, image->width, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, image_in_bitmap, "./images/image_greyscale.png", 0);
	FreeImage_Unload(image_in_bitmap);

	const auto bins = static_cast<unsigned int*>(calloc(gray_levels, sizeof(unsigned int)));

	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	auto ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_CPU, 10, device_id, &ret_num_devices);

	const auto context = clCreateContext(nullptr, 1, &device_id[0], nullptr, nullptr, &ret);
	const auto command_queue = clCreateCommandQueueWithProperties(context, device_id[0], nullptr, &ret);
	
	const size_t workgroup_side_length = sqrt(workgroup_size);
	size_t local_item_size[2] = { workgroup_side_length, workgroup_side_length };
	size_t global_item_size[2] = {
		ceil(image->width / static_cast<float>(local_item_size[0])) * local_item_size[0],
		ceil(image->height / static_cast<float>(local_item_size[1])) * local_item_size[1]
	};

	auto image_input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image->width * image->height * sizeof(unsigned char), image->pixels, &ret);
	auto histogram_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gray_levels * sizeof(unsigned int*), bins, &ret);
	auto cdf_histogram_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gray_levels * sizeof(unsigned int*), bins, &ret);

	const auto program = clCreateProgramWithSource(context, 1, const_cast<const char**>(&kernel_source), nullptr, &ret);

	ret = clBuildProgram(program, 1, &device_id[0], nullptr, nullptr, nullptr);

	size_t build_log_len;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &build_log_len);

	const auto build_log = static_cast<char*>(malloc(sizeof(char)* (build_log_len + 1)));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, nullptr);
	printf("%s\n", build_log);
	free(build_log);

	size_t buf_size_t;


	
	const auto histogram_kernel = clCreateKernel(program, "histogram", &ret);
	clGetKernelWorkGroupInfo(histogram_kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, nullptr);
	ret |= clSetKernelArg(histogram_kernel, 0, sizeof(cl_mem), static_cast<void*>(&image_input_mem_obj));
	ret |= clSetKernelArg(histogram_kernel, 1, sizeof(cl_mem), static_cast<void*>(&histogram_mem_obj));
	ret |= clSetKernelArg(histogram_kernel, 2, sizeof(cl_int), static_cast<void*>(&image->width));
	ret |= clSetKernelArg(histogram_kernel, 3, sizeof(cl_int), static_cast<void*>(&image->height));

	const auto cdf_kernel = clCreateKernel(program, "cdf", &ret);
	clGetKernelWorkGroupInfo(cdf_kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, nullptr);
	ret |= clSetKernelArg(cdf_kernel, 0, sizeof(cl_mem), static_cast<void*>(&histogram_mem_obj));
	ret |= clSetKernelArg(cdf_kernel, 1, sizeof(cl_mem), static_cast<void*>(&cdf_histogram_mem_obj));

	const auto eq_kernel = clCreateKernel(program, "equalize", &ret);
	clGetKernelWorkGroupInfo(eq_kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, nullptr);
	ret |= clSetKernelArg(eq_kernel, 0, sizeof(cl_mem), static_cast<void*>(&cdf_histogram_mem_obj));
	ret |= clSetKernelArg(eq_kernel, 1, sizeof(cl_mem), static_cast<void*>(&image_input_mem_obj));
	ret |= clSetKernelArg(eq_kernel, 2, sizeof(cl_int), static_cast<void*>(&image->width));
	ret |= clSetKernelArg(eq_kernel, 3, sizeof(cl_int), static_cast<void*>(&image->height));



	//WARMUP
	ret = clEnqueueWriteBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, image->width * image->height * sizeof(char), image->pixels, 0, nullptr, nullptr);
	cl_event histogram_event;
	ret = clEnqueueNDRangeKernel(command_queue, histogram_kernel, 2, nullptr, global_item_size, local_item_size, 0, nullptr, &histogram_event);
	size_t cdf_global_size = 128;
	size_t cdf_local_size = 128;
	cl_event cdf_event;
	ret = clEnqueueNDRangeKernel(command_queue, cdf_kernel, 1, nullptr, &cdf_global_size, &cdf_local_size, 1, &histogram_event, &cdf_event);
	ret = clEnqueueNDRangeKernel(command_queue, eq_kernel, 2, nullptr, global_item_size, local_item_size, 1, &cdf_event, nullptr);
	ret = clEnqueueReadBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, image->width * image->height * sizeof(char), image->pixels, 0, nullptr, nullptr);


	//const auto start_time = clock();
	//for(auto i = 0; i < 50; i++ )
	//{
	//	ret = clEnqueueWriteBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, image->width * image->height * sizeof(char), image->pixels, 0, nullptr, nullptr);
	//	cl_event histogram_event;
	//	ret = clEnqueueNDRangeKernel(command_queue, histogram_kernel, 2, nullptr, global_item_size, local_item_size, 0, nullptr, &histogram_event);
	//	size_t cdf_global_size = 128;
	//	size_t cdf_local_size = 128;
	//	cl_event cdf_event;
	//	ret = clEnqueueNDRangeKernel(command_queue, cdf_kernel, 1, nullptr, &cdf_global_size, &cdf_local_size, 1, &histogram_event, &cdf_event);
	//	ret = clEnqueueNDRangeKernel(command_queue, eq_kernel, 2, nullptr, global_item_size, local_item_size, 1, &cdf_event, nullptr);
	//	ret = clEnqueueReadBuffer(command_queue, image_input_mem_obj, CL_TRUE, 0, image->width * image->height * sizeof(char), image->pixels, 0, nullptr, nullptr);
	//}
	//const auto end_time = clock();

	//const auto execution_time = static_cast<double>(end_time - start_time) / 50 / CLOCKS_PER_SEC;
	//
	//std::cout << "Cas racunanja: " << execution_time << "s." << std::endl;
	
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(histogram_kernel);
	ret = clReleaseKernel(cdf_kernel);
	ret = clReleaseKernel(eq_kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(image_input_mem_obj);
	ret = clReleaseMemObject(histogram_mem_obj);
	ret = clReleaseMemObject(cdf_histogram_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);



	const auto image_out_bitmap = FreeImage_ConvertFromRawBits(image->pixels, image->width, image->height, image->width, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, image_out_bitmap, "./images/image_out.png", 0);
	FreeImage_Unload(image_out_bitmap);

	free(image->pixels);
	free(image);

	return 0;
}



