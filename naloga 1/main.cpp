#include <cstdlib>
#include <iostream>
#include <iomanip>


double* Random(int n)
{
	const auto random_array_of_size_n = new double[n];

	for (auto i = 0; i < n; i++)
	{
		random_array_of_size_n[i] = static_cast<float>(rand()) / RAND_MAX;
	}

	return random_array_of_size_n;
}

double** Matrix(double* A, int n, int r)
{
	const auto column_count = ceil(static_cast<float>(n) / static_cast<float>(r));

	const auto row_of_pointers = new double* [column_count];

	auto element_index = 0;

	for (auto c = 0; c < column_count; c++)
	{
		row_of_pointers[c] = new double[r];
	}

	for (auto row = 0; row < r; row++)
	{
		for (auto c = 0; c < column_count; c++)
		{
			double element_value = 0;
			if (element_index < n)
			{
				element_value = A[element_index];
			}

			row_of_pointers[c][row] = element_value;
			element_index++;
		}
	}
	return row_of_pointers;
}

double* Max(double* A, int n)
{
	auto max_value_index = 0;

	for (auto i = 0; i < n; i++)
	{
		if (A[i] > A[max_value_index])
		{
			max_value_index = i;
		}
	}

	return &A[max_value_index];
}


int main()
{
	srand(time(nullptr));
	std::cout << std::fixed;
	std::cout << std::setprecision(2);

	int element_count;
	std::cout << "Vnesi n: ";
	std::cin >> element_count;

	int row_count;
	std::cout << "Vnesi r: ";
	std::cin >> row_count;

	const auto start_time = clock();
	const auto element_array = Random(element_count);
	const auto end_time = clock();
	const auto execution_time = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;

	std::cout << "1D:" << std::endl;
	for (auto i = 0; i < element_count; i++)
	{
		std::cout << element_array[i] << " ";
	}
	std::cout << std::endl;

	const auto column_count = ceil(static_cast<float>(element_count) / static_cast<float>(row_count));
	const auto element_matrix = Matrix(element_array, element_count, row_count);

	std::cout << "2D:" << std::endl;
	for (auto r = 0; r < row_count; r++)
	{
		for (auto c = 0; c < column_count; c++)
		{
			std::cout << element_matrix[c][r] << " ";
		}
		std::cout << std::endl;
	}

	const auto max_value_address = Max(element_array, element_count);
	std::cout << std::endl;
	std::cout << "Najvecja vrednost: " << *max_value_address << " na naslovu: " << max_value_address << "." << std::endl;
	std::cout << "Cas generiranja nakljucnih stevil: " << execution_time << "s." << std::endl;
}
