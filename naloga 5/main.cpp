// "s"cepec preberemo iz datoteke
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <cmath>
#include <FreeImage.h>

#define IMAGE_WIDTH	(1024)
#define IMAGE_HEIGHT (1024)
#define WORKGROUP_SIZE	(64)
#define MAX_SOURCE_SIZE	16384

int main()
{
	char ch;
	int i;
	cl_int ret;

	int imagePixelCount = IMAGE_WIDTH * IMAGE_HEIGHT;

	int image_width = IMAGE_WIDTH;
	int image_height = IMAGE_HEIGHT;

	// Branje datoteke
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

	// Rezervacija pomnilnika
	unsigned char* C = (unsigned char*)malloc(imagePixelCount * sizeof(int));

	// Podatki o platformi
	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	char* buf;
	size_t			buf_len;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
	// max. "stevilo platform, kazalec na platforme, dejansko "stevilo platform

	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,
		device_id, &ret_num_devices);
	// izbrana platforma, tip naprave, koliko naprav nas zanima
	// kazalec na naprave, dejansko "stevilo naprav

	// Kontekst
	cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
	// kontekst: vklju"cene platforme - NULL je privzeta, "stevilo naprav, 
	// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
	// dodatni parametri funkcije, "stevilka napake

	// Ukazna vrsta
	cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id[0], 0, &ret);
	// kontekst, naprava, INORDER/OUTOFORDER, napake

	// Delitev dela
	size_t local_item_size[2] = { sqrt(WORKGROUP_SIZE), sqrt(WORKGROUP_SIZE) };
	size_t global_item_size[2] = { IMAGE_WIDTH, IMAGE_HEIGHT };

	printf("local size: %zu, %zu\n", local_item_size[0], local_item_size[1]);
	printf("global size: %zu, %zu\n", global_item_size[0], global_item_size[1]);

	// Alokacija pomnilnika na napravi
	cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, imagePixelCount * sizeof(int), NULL, &ret);

	// Priprava programa
	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source_str, NULL, &ret);
	// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
	// stringi so NULL terminated, napaka													

	// Prevajanje
	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
	printf("%d\n", ret);

	// program, "stevilo naprav, lista naprav, opcije pri prevajanju,
	// kazalec na funkcijo, uporabni"ski argumenti

	// Log
	size_t build_log_len;
	char* build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
	printf("%d\n", ret);
	// program, "naprava, tip izpisa, 
	// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
	build_log = (char*)malloc(sizeof(char) * (build_log_len + 1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, NULL);
	printf("%d\n", ret);
	printf("%s\n", build_log);
	free(build_log);

	// "s"cepec: priprava objekta
	cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);
	// program, ime "s"cepca, napaka

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);
	printf("veckratnik niti = %d", buf_size_t);

	scanf("%c", &ch);

	// "s"cepec: argumenti
	ret |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&c_mem_obj);
	printf("%d\n", ret);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void*)&image_width);
	printf("%d\n", ret);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&image_height);
	printf("%d\n", ret);
	// "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke


	// "s"cepec: zagon
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_item_size, local_item_size, 0, NULL, NULL);
	printf("%d\n", ret);
	// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
	// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
	// dogodki, ki se morajo zgoditi pred klicem

	// Kopiranje rezultatov
	ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, imagePixelCount * sizeof(int), C, 0, NULL, NULL);
	printf("%d\n", ret);
	// branje v pomnilnik iz naparave, 0 = offset
	// zadnji trije - dogodki, ki se morajo zgoditi prej

	// Prikaz rezultatov

	for (auto i = 0; i < imagePixelCount; i++)
	{
		//printf("%d\n", C[i]);
	}
		

	 // "ci"s"cenje
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(c_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	int pitch = ((32 * image_width + 31) / 32) * 4;
	
	FIBITMAP* dst = FreeImage_ConvertFromRawBits(C, image_width, image_height, pitch,
		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Save(FIF_PNG, dst, "mandelbrot_gpu.png", 0);

	
	free(C);



	
	return 0;
}

// kernel

