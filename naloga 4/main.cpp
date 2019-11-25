#include <cstdio>
#include <omp.h>
#include <set>
#include <ctime>

std::set<int>* find_number_divisors(const int number)
{
	auto divisors = new std::set<int>();

	for (auto i = 1; i < sqrt(number); i++)
	{
		if (number % i != 0)
		{
			continue;
		}

		divisors->insert(i);

		if (i == 1)
		{
			continue;
		}

		divisors->insert(number / i);
	}

	return divisors;
}

int sum_divisors(const int n)
{
	const auto divisors = find_number_divisors(n);

	auto sum = 0;

	for (auto divisor : *divisors)
	{
		sum += divisor;
	}

	delete divisors;

	return sum;
}

int main()
{
	const auto n = 1000000;
	auto friendly_numbers_sum = 0;

	auto start_time = clock();
	for (auto i = 1; i < n; i += 1)
	{
		const auto sum = sum_divisors(i);
		const auto sum2 = sum_divisors(sum);

		if (i == sum2 && i != sum && i < sum)
		{
			friendly_numbers_sum += i;
		}
	}

	auto end_time = clock();
	const auto execution_time_baseline = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;
	printf("[Baseline] Sum: %d, Execution time: %f\n", friendly_numbers_sum, execution_time_baseline);


	friendly_numbers_sum = 0;

	start_time = clock();
	#pragma omp parallel for schedule(static)
	for (auto i = 1; i < n; i += 1)
	{
		const auto sum = sum_divisors(i);
		const auto sum2 = sum_divisors(sum);

		if (i == sum2 && i != sum && i < sum)
		{
			#pragma omp atomic
			friendly_numbers_sum += i;
		}
	}

	end_time = clock();
	auto execution_time = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;
	printf("[Parallel Static] Sum: %d, Execution time: %f\n", friendly_numbers_sum, execution_time);

	friendly_numbers_sum = 0;

	start_time = clock();
	#pragma omp parallel for schedule(dynamic)
	for (auto i = 1; i < n; i += 1)
	{
		const auto sum = sum_divisors(i);
		const auto sum2 = sum_divisors(sum);

		if (i == sum2 && i != sum && i < sum)
		{
			#pragma omp atomic
			friendly_numbers_sum += i;
		}
	}

	end_time = clock();
	execution_time = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;
	printf("[Parallel Dynamic] Sum: %d, Execution time: %f\n", friendly_numbers_sum, execution_time);

	friendly_numbers_sum = 0;

	start_time = clock();
	#pragma omp parallel for schedule(guided)
	for (auto i = 1; i < n; i += 1)
	{
		const auto sum = sum_divisors(i);
		const auto sum2 = sum_divisors(sum);

		if (i == sum2 && i != sum && i < sum)
		{
			#pragma omp atomic
			friendly_numbers_sum += i;
		}
	}

	end_time = clock();
	execution_time = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;
	printf("[Parallel Guided] Sum: %d, Execution time: %f\n", friendly_numbers_sum, execution_time);
}