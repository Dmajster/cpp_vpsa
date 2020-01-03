#include <mpi.h>
#include <cstdio>
#include <string>

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);

	int process_count;
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	
	int process_id;
	MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

	auto buffer = new int[process_count];
	
	if(process_id == 0)
	{
		buffer[0] = process_id;
		
		MPI_Send(buffer, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

		MPI_Recv(buffer, process_count, MPI_INT, process_count-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		for(auto i = 0; i < process_count + 1; i++)
		{
			const auto index = i % process_count;

			printf("%d", buffer[index]);
			
			if( i < process_count )
			{
				printf(" - ");
			}
		}
	}
	else
	{
		MPI_Recv(buffer, process_id, MPI_INT, process_id - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		buffer[process_id] = process_id;

		MPI_Send(buffer, process_id + 1, MPI_INT, (process_id + 1) % process_count, 0, MPI_COMM_WORLD);
	}
	
	MPI_Finalize();
	return 0;
}
