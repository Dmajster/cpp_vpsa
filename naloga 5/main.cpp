// "s"cepec preberemo iz datoteke
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <CL/cl.h>
#include <memory>

#define SIZE			(1024)
#define WORKGROUP_SIZE	(128)
#define MAX_SOURCE_SIZE	16384

int main()
{
	char ch;
	int i;

	auto vector_size = SIZE;

	char* source_str;

	const auto fp = fopen("kernel.cl", "r");
	if (!fp)
	{
		fprintf(stderr, ":-(#\n");
		exit(1);
	}
	source_str = new char[MAX_SOURCE_SIZE];
	const auto source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);

	const auto a = new int[vector_size];
	const auto b = new int[vector_size];
	const auto c = new int[vector_size];

	for (i = 0; i < vector_size; i++)
	{
		a[i] = i;
		b[i] = vector_size - i;
	}

	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	char* buf;
	size_t			buf_len;
	auto ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

	printf("platform id: %d\n", ret);
	
	char* profile = nullptr;
	size_t size;
	clGetPlatformInfo(platform_id[0], CL_PLATFORM_PROFILE, NULL, profile, &size);
	profile = static_cast<char*>(malloc(size));
	clGetPlatformInfo(platform_id[0], CL_PLATFORM_PROFILE, size, profile, nullptr);
	printf("%s\n", profile);
	
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10, device_id, &ret_num_devices);
	printf("get 0 device id: %d\n", ret);

	
	const auto context = clCreateContext(nullptr, 1, &device_id[0], nullptr, nullptr, &ret);
	const auto command_queue = clCreateCommandQueueWithProperties(context, device_id[0], nullptr, &ret);

	size_t local_item_size = WORKGROUP_SIZE;
	const auto num_groups = (vector_size - 1) / local_item_size + 1;
	auto global_item_size = num_groups * local_item_size;

	auto a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, vector_size * sizeof(int), a, &ret);
	auto b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, vector_size * sizeof(int), b, &ret);
	auto c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, vector_size * sizeof(int), nullptr, &ret);

	const auto program = clCreateProgramWithSource(context, 1, const_cast<const char**>(&source_str), nullptr, &ret);

	ret = clBuildProgram(program, 1, &device_id[0], nullptr, nullptr, nullptr);
	printf("get build info: %d\n", ret);
	
	size_t build_log_len;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &build_log_len);
	printf("get program build info: %d\n", ret);
	
	const auto build_log = static_cast<char*>(malloc(sizeof(char) * (build_log_len + 1)));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, nullptr);
	printf("get program build info 2: %d\n", ret);
	printf("%s\n", build_log);
	free(build_log);

	const auto kernel = clCreateKernel(program, "vector_add", &ret);

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, nullptr);
	printf("veckratnik niti = %zu", buf_size_t);

	scanf("%c", &ch);

	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void*>(&a_mem_obj));
	printf("set kernel arg 0: %d\n", ret);
	
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), static_cast<void*>(&b_mem_obj));
	printf("set kernel arg 1: %d\n", ret);
	
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), static_cast<void*>(&c_mem_obj));
	printf("set kernel arg 2: %d\n", ret);
	
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), static_cast<void*>(&vector_size));
	printf("set kernel arg 3: %d\n", ret);
	
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, &global_item_size, &local_item_size, 0, nullptr, nullptr);
	printf("enqueue nd range kernel %d\n", ret);
	
	ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, vector_size * sizeof(int), c, 0, nullptr, nullptr);
	printf("enqueue read buffer %d\n", ret);
	

	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(a_mem_obj);
	ret = clReleaseMemObject(b_mem_obj);
	ret = clReleaseMemObject(c_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	return 0;
}

