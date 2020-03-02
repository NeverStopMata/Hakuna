#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#ifdef _DEBUG
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define new new
#endif
#include <iostream>
#include "HakunaRenderer.h"
#ifdef _DEBUG
static _CrtMemState s1, s2, s3;
#endif // _DEBUG

int main() {
#ifdef _DEBUG
	_CrtMemCheckpoint(&s1);
#endif // _DEBUG
	{
		HakunaRenderer renderer;
		try {
			renderer.Loop();
		}
		catch (const std::exception & e)
		{
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}
#ifdef _DEBUG
	_CrtMemCheckpoint(&s2);
	try {
		if (_CrtMemDifference(&s3, &s1, &s2))
		{
			_CrtMemDumpStatistics(&s3);
			_CrtDumpMemoryLeaks();
			throw "Memory Leak!";
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
#endif

/*multi-thread code example begin*/
	//ThreadPool pool(8);
	//std::vector< std::future<int> > results;
	//std::array<int, 8> test_array{ 0,1,2,3,4,5,6,7 };
	//for (int i = 0; i < 8; ++i) {
	//	results.emplace_back(
	//		pool.enqueue([i, test_array] {
	//		std::cout << "hello " << test_array[i] << std::endl;
	//		std::this_thread::sleep_for(std::chrono::seconds(3));
	//		std::cout << "world " << i << std::endl;
	//		return i * i;
	//	})
	//	);
	//}

	//for (auto&& result : results)
	//	std::cout << result.get() << ' ';
	//std::cout << std::endl;

	//return 0;
/*multi-thread code example end*/
	return EXIT_SUCCESS;
}



