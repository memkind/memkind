
#include "pmem_allocator.h"
#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <utility>
#include <string>
#include <algorithm>
#include <scoped_allocator>
#include <cassert>

#define STL_VECTOR_TEST
#define STL_LIST_TEST
#define STL_MAP_TEST
#define STL_VEC_STRING_TEST
#define STL_MAP_INT_STRING_TEST

void cpp_allocator_test() {

	std::cout << "MAIN SCOPE: HELLO" << std::endl;

	const char* pmem_directory = "/dev/shm";
	size_t pmem_max_size = 1024*1024*1024;

    /*
     * TODO: Because of the issue #57, all allocators must be using the same memkind,
     * so they are built via copy constructor.
     * When the issue is fixed, we should split the program to several functions,
     * instead of all-in-one implementation.
     */

    pmem::allocator<int> alloc_source { pmem_directory, pmem_max_size } ;

	#ifdef STL_VECTOR_TEST

	std::cout << "VECTOR OPEN" << std::endl;

	{
		pmem::allocator<int> alc{ alloc_source  };

		std::vector<int, pmem::allocator<int>> vector{ alc };

		for (int i = 0; i < 20; ++i) {
			vector.push_back(0xDEAD + i);
			assert(vector.back() == 0xDEAD + i);

		}

		std::cout << "VECTOR CLOSE" << std::endl;
	}

	#endif
	
	#ifdef STL_LIST_TEST

	std::cout << "LIST OPEN" << std::endl;
	
	{
		pmem::allocator<int> alc{ alloc_source };

		std::list<int, pmem::allocator<int>> list{ alc };

		const int nx2 = 4;

		for (int i = 0; i < nx2; ++i) {
			list.emplace_back(0xBEAC011 + i);
			assert(list.back() == 0xBEAC011 + i);
		}

		for (int i = 0; i < nx2; ++i) {
			list.pop_back();
		}

		std::cout << "LIST CLOSE" << std::endl;
	}

	#endif

	#ifdef STL_MAP_TEST

	std::cout << "MAP OPEN" << std::endl;

	{
		pmem::allocator<std::pair<const std::string, std::string>> alc{ alloc_source };

		std::map<std::string, std::string, std::less<std::string>, pmem::allocator<std::pair<const std::string, std::string> > > map{ alc };
		
		for (int i = 0; i < 10; ++i) {
			map[std::to_string((i * 997 + 83) % 113)] = std::to_string(0x0CEA11 + i);
			assert(map[std::to_string((i * 997 + 83) % 113)] == std::to_string(0x0CEA11 + i));
		}

		std::cout << "MAP CLOSE" << std::endl;
	}

	#endif

	#ifdef STL_VEC_STRING_TEST

	std::cout << "STRINGED VECTOR OPEN" << std::endl;

	{

		typedef std::basic_string<char, std::char_traits<char>, pmem::allocator<char>> pmem_string;
		typedef pmem::allocator<pmem_string> pmem_alloc;

		pmem_alloc alc{ alloc_source };
		pmem::allocator<char> st_alc{alc};

		std::vector<pmem_string, pmem_alloc> vec{ alc };
		pmem_string arg{ "Very very loooong striiiing", st_alc };

		vec.push_back(arg);
		assert(vec.back() == arg);

		std::cout << "STRINGED VECTOR CLOSE" << std::endl;
	}

	#endif

	#ifdef STL_MAP_INT_STRING_TEST

	std::cout << "INT_STRING MAP OPEN" << std::endl;

	{
		
		typedef std::basic_string<char, std::char_traits<char>, pmem::allocator<char> > pmem_string;
		typedef int key_t;
		typedef pmem_string value_t;
		typedef std::pair<key_t, value_t> target_pair;
		typedef pmem::allocator<target_pair> pmem_alloc;
        typedef pmem::allocator<char> str_allocator_t;
		typedef std::map<key_t, value_t, std::less<key_t>, std::scoped_allocator_adaptor<pmem_alloc> > map_t;

		pmem_alloc map_allocator( alloc_source );

		str_allocator_t str_allocator( map_allocator );

		value_t source_str1( "Lorem ipsum dolor " , str_allocator);
		value_t source_str2( "sit amet consectetuer adipiscing elit", str_allocator );

		map_t target_map{ std::scoped_allocator_adaptor<pmem_alloc>(map_allocator) };

		target_map[key_t(165)] = source_str1;
		assert(target_map[key_t(165)] == source_str1);
		target_map[key_t(165)] = source_str2;
		assert(target_map[key_t(165)] == source_str2);

		std::cout << "INT_STRING MAP CLOSE" << std::endl;
	}

	#endif

	std::cout << "MAIN SCOPE: GOODBYE" << std::endl;
}

void create_destroy_test() {
    memkind_t mkind = 0;
    memkind_t mkind2 = 0;
    
    const int size = 1024 * 1024 * 1024;
    const char* dir = "/dev/shm";
    
    int err = memkind_create_pmem(dir, size, &mkind);
    std::cout << "Caught an error while creating 1: " << err << std::endl;

    err = memkind_create_pmem(dir, size, &mkind2);
    std::cout << "Caught an error while creating 2: " << err << std::endl;
    
    err = memkind_destroy_kind(mkind);
    std::cout << "Caught an error while destroying 1: " << err << std::endl;
    mkind = 0;

    err = memkind_destroy_kind(mkind2);
    std::cout << "Caught an error while destroying 2: " << err << std::endl;
    mkind2 = 0;
    
}

int main() {
    cpp_allocator_test();
    return 0;
}



