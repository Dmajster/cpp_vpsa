#define HAVE_STRUCT_TIMESPEC
#include <vector>
#include <memory>
#include <pthread.h>

constexpr auto array_size = 10;
constexpr auto max_value = 20;
constexpr auto min_value = 0;

pthread_barrier_t barrier1;

bool has_changed = false;

struct thread_data
{
	size_t thread_index{};
	std::shared_ptr<std::vector<int>> numbers;
};

int random(int min, int max)
{
	return min_value + rand() % (max_value - min_value);
}

void print_array(std::vector<int>& vector)
{
	for (auto& value : vector)
	{
		printf("%d ", value);
	}
	printf("\n");
}

void* sort_function(void* args)
{
	const auto data = static_cast<thread_data*>(args);
	const auto index = data->thread_index;
	auto numbers = data->numbers;

	const auto even_index = data->thread_index * 2;
	const auto odd_index = even_index + 1;

	auto odd = false;
	size_t number_index;

	while (true)
	{
		if (odd)
		{
			number_index = even_index;
		}
		else
		{
			number_index = odd_index;
		}

		odd = !odd;

		if( number_index + 1 < array_size )
		{
			auto& number1 = numbers->at(number_index);
			auto& number2 = numbers->at(number_index + 1);
			
			if(number1 > number2)
			{
				std::swap(number1, number2);
				has_changed = true;
			}
		}

		pthread_barrier_wait(&barrier1);

		if (!has_changed)
		{
			return nullptr;
		}

		if (index == 0)
		{
			print_array(*numbers);
		}
		
		pthread_barrier_wait(&barrier1);

		has_changed = false;

		pthread_barrier_wait(&barrier1);
	}
}


int main()
{
	srand(time(nullptr));
	auto numbers = std::make_shared<std::vector<int>>();

	for (size_t i = 0; i < array_size; i++)
	{
		numbers->push_back(random(min_value, max_value));
	}

	printf("Input: ");
	print_array(*numbers);
	printf("\n");
	
	const auto thread_count = (array_size + 1) / 2;

	auto threads = std::make_shared<std::vector<pthread_t>>(thread_count);
	auto threads_data = std::make_shared<std::vector<thread_data>>(thread_count);

	pthread_barrier_init(&barrier1, nullptr, thread_count);
	
	for (size_t i = 0; i < threads->size(); i++)
	{
		threads_data->at(i) = {
			i,
			numbers
		};

		pthread_create(&threads->at(i), nullptr, sort_function, &threads_data->at(i));
	}

	for (auto& thread : *threads)
	{
		pthread_join(thread, nullptr);
	}


	return 0;
}
