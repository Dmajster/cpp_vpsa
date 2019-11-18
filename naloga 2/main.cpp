#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <set>
#include <memory>
#include <vector>
#include <tuple>
#include <iomanip>
#include <iostream>

constexpr auto n = 100000;
constexpr auto bucket_size = 10000;
constexpr auto repeat_count = 10;

struct thread_data_static
{
	size_t thread_id;
	size_t thread_count;
	size_t n;
};

struct thread_data_dynamic
{
	size_t thread_id;
	size_t bucket_size;
	size_t bucket_count;
};

size_t friendly_numbers_sum = 0;

size_t bucket_current = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


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

void* static_work_allocation_function(void* args)
{
	const auto thread_data = static_cast<thread_data_static*>(args);

	for (auto i = 1 + thread_data->thread_id; i < thread_data->n; i += thread_data->thread_count)
	{
		const auto sum = sum_divisors(i);
		const auto sum2 = sum_divisors(sum);

		if (i == sum2 && i != sum && i < sum)
		{
			pthread_mutex_lock(&mutex);
			friendly_numbers_sum += i;
			pthread_mutex_unlock(&mutex);
			
			//printf("thread: %zu, pair: %llu, %d\n", thread_data->thread_id, i, sum);
		}
	}

	return nullptr;
}

void* dynamic_work_allocation_function(void* args)
{
	const auto thread_data = static_cast<thread_data_dynamic*>(args);

	while (true)
	{
		const auto bucket_id = bucket_current;
		const auto bucket_from = bucket_id * bucket_size;
		const auto bucket_to = (bucket_id + 1) * bucket_size;

		pthread_mutex_lock(&mutex);
		bucket_current++;
		pthread_mutex_unlock(&mutex);

		for (int i = bucket_from; i < bucket_to; i++)
		{
			const auto sum = sum_divisors(i);
			const auto sum2 = sum_divisors(sum);

			if (i == sum2 && i != sum && i < sum)
			{
				pthread_mutex_lock(&mutex);
				friendly_numbers_sum += i;
				pthread_mutex_unlock(&mutex);
				
				//printf("thread: %zu, pair: %llu, %d\n", thread_data->thread_id, i, sum);
			}
		}

		if (bucket_current >= thread_data->bucket_count)
		{
			break;
		}
	}

	return nullptr;
}

std::tuple<size_t, double> static_work(const size_t thread_count)
{
	const auto threads = new std::vector<pthread_t>(thread_count);
	const auto threads_data = new std::vector<thread_data_static*>(thread_count);

	friendly_numbers_sum = 0;
	
	//Static work allocation
	const auto start_time = clock();
	for (size_t thread_id = 0; thread_id < thread_count; thread_id++)
	{
		threads_data->at(thread_id) = new thread_data_static{
			thread_id,
			thread_count,
			n
		};

		pthread_create(&threads->at(thread_id), nullptr, static_work_allocation_function, threads_data->at(thread_id));
	}
	
	for (auto &thread : *threads)
	{
		pthread_join(thread, nullptr);
	}
	const auto end_time = clock();
	const auto execution_time = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;

	//Clean up thread data
	for (auto static_data : *threads_data)
	{
		delete static_data;
	}

	delete threads;
	delete threads_data;

	return std::make_tuple(friendly_numbers_sum, execution_time);
}

std::tuple<size_t, double> dynamic_work(const size_t thread_count)
{
	auto mutex = PTHREAD_MUTEX_INITIALIZER;

	const auto threads = new std::vector<pthread_t>(thread_count);
	const auto threads_data = new std::vector<thread_data_dynamic*>(thread_count);

	friendly_numbers_sum = 0;
	bucket_current = 0;

	const auto start_time = clock();
	for (size_t thread_id = 0; thread_id < thread_count; thread_id++)
	{
		size_t bucket_current = 0;
		const size_t bucket_count = n / bucket_size;

		threads_data->at(thread_id) = new thread_data_dynamic{
			thread_id,
			bucket_size,
			bucket_count
		};

		pthread_create(&threads->at(thread_id), nullptr, dynamic_work_allocation_function, threads_data->at(thread_id));
	}

	for (auto& thread : *threads)
	{
		pthread_join(thread, nullptr);
	}
	const auto end_time = clock();
	const auto execution_time = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;

	//Clean up thread data
	for (auto dynamic_data : *threads_data)
	{
		delete dynamic_data;
	}

	delete threads;
	delete threads_data;

	return std::make_tuple(friendly_numbers_sum, execution_time);
}


int main()
{
	std::cout << std::fixed;
	std::cout << std::setprecision(2);

	auto results = static_work(1);
	size_t result_number;
	double result_time;
	auto time_single_static = 0.0;
	
	for(auto i = 0; i < repeat_count; i++ )
	{
		results = static_work(1);
		result_number = std::get<0>(results);
		result_time = std::get<1>(results);
		time_single_static += result_time;
	}

	time_single_static /= repeat_count;
	

	printf("[1 threads static] sum: %zu, finished on average in: %fs \n", result_number, time_single_static);

	auto time_single_dynamic = 0.0;
	dynamic_work(1);
	for (auto i = 0; i < repeat_count; i++)
	{
		results = dynamic_work(1);
		result_number = std::get<0>(results);
		result_time = std::get<1>(results);
		time_single_dynamic += result_time;
	}
	time_single_dynamic /= repeat_count;

	printf("[1 threads dynamic] sum: %zu, finished on average in: %fs \n", result_number, time_single_dynamic);

	auto time_multi_static = 0.0;
	static_work(8);
	for (auto i = 0; i < repeat_count; i++)
	{
		results = static_work(8);
		result_number = std::get<0>(results);
		result_time = std::get<1>(results);
		time_multi_static += result_time;
	}
	time_multi_static /= repeat_count;
	
	printf("[8 threads static] sum: %zu, finished on average in: %fs \n", result_number, time_multi_static);

	auto time_multi_dynamic = 0.0;
	dynamic_work(8);
	for (auto i = 0; i < repeat_count; i++)
	{
		results = dynamic_work(8);
		result_number = std::get<0>(results);
		result_time = std::get<1>(results);
		time_multi_dynamic += result_time;
	}
	time_multi_dynamic /= repeat_count;
	
	printf("[8 threads dynamic] sum: %zu, finished on average in: %fs \n", result_number, time_multi_dynamic);

	printf("Static improvement %f\n", time_single_static / time_multi_static);
	printf("Dynamic improvement %f\n", time_single_dynamic / time_multi_dynamic);
}
