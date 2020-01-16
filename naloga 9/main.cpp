#include <mpi.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>

struct Bucket
{
	int start_index;
	int size;
	int offset;
};

int compare(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);

	int process_count = 8;
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);

	int process_id;
	MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

	const auto n = 21;
	const auto group_size = n / process_count;
	const auto receive_buffer = new int[group_size];

	const auto random_numbers = new int[n];
	const auto send_counts = new int[process_count];
	const auto displacements = new int[process_count];
	
	if(process_id == 0)
	{
		printf("numbers: ");
		for (auto i = 0; i < n; i++)
		{
			random_numbers[i] = rand() % 1000;
			printf("%d, ", random_numbers[i]);
		}

		printf("\nnumber counts: ");
		for (auto i = 0; i < process_count; i++)
		{
			send_counts[i] = group_size;
			printf("%d, ", send_counts[i]);
		}

		printf("\nnumber offsets: ");
		for (auto i = 0; i < process_count; i++)
		{
			displacements[i] = group_size * i;
			printf("%d, ", displacements[i]);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Scatterv(random_numbers, send_counts, displacements, MPI_INT, receive_buffer, group_size, MPI_INT, 0, MPI_COMM_WORLD);

	qsort(receive_buffer, group_size, sizeof(int), compare);

	const auto result_buffer = new int[n];
	const auto final_buffer = new int[n];
	
	MPI_Gatherv(receive_buffer, group_size, MPI_INT, result_buffer, send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);

	if( process_id == 0 )
	{
		const auto buckets = new Bucket[process_count];

		for(auto i = 0; i < process_count; i++)
		{
			buckets[i] = {
				displacements[i],
				send_counts[i],
			0
			};
		}
		
		for (auto i = 0; i < n; i++)
		{
			auto min_b = 0;
			
			for(auto b = 0; b < process_count; b++)
			{
				if( buckets[b].offset == buckets[b].size )
				{
					continue;
				}

				const auto bucket_value = result_buffer[buckets[b].start_index + buckets[b].offset];
				const auto min_bucket_value = result_buffer[buckets[min_b].start_index + buckets[min_b].offset];
				
				if (bucket_value < min_bucket_value)
				{
					min_b = b;
				}
			}
			
			const auto min_index = buckets[min_b].start_index + buckets[min_b].offset;
			const auto min_bucket_value = result_buffer[min_index];
		
			final_buffer[i] = min_bucket_value;
			
			buckets[min_b].offset++;
		}

		printf("\nresult buffer: ");
		for (auto i = 0; i < n; i++)
		{
			printf("%d, ", final_buffer[i]);
		}
	}
	
	MPI_Finalize();
	return 0;
}
