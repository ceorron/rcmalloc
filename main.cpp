/*----------------------------------------------------------------------------------*\
 |																					|
 | main.cpp		 																	|
 |																					|
 | Copyright (c) 2019 Richard Cookman												|
 |																					|
 | Permission is hereby granted, free of charge, to any person obtaining a copy		|
 | of this software and associated documentation files (the "Software"), to deal	|
 | in the Software without restriction, including without limitation the rights		|
 | to use, copy, modify, merge, publish, distribute, sublicense, and/or sell		|
 | copies of the Software, and to permit persons to whom the Software is			|
 | furnished to do so, subject to the following conditions:							|
 |																					|
 | The above copyright notice and this permission notice shall be included in all	|
 | copies or substantial portions of the Software.									|
 |																					|
 | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR		|
 | IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,			|
 | FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE		|
 | AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER			|
 | LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,	|
 | OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE	|
 | SOFTWARE.																		|
 |																					|
\*----------------------------------------------------------------------------------*/

#include <iostream>
#include <vector>

#include "rcmalloc.hpp"

using namespace std;
using namespace rcmalloc;

struct a_struct {
	unsigned a = 100;
	float b = 3.0;
};

#define POOLB		1
#define POOLC		2

int main() {
	//use new/new[] and delete/delete[] replacements
	cout << "Test 1" << endl;
	{
		//allocate, construct
		a_struct& a1 = *new_T<a_struct>();
		a_struct& a2 = *new_T<a_struct>();
		a_struct& a3 = *new_T(a_struct{300, 10.0f});

		//allocate, construct list
		a_struct* l1 = allocate_init_count<default_allocator<a_struct>>(100);

		//destruct deallocate
		destruct_deallocate<default_allocator<a_struct>>(&a1);
		destruct_deallocate<default_allocator<a_struct>>(&a2);
		destruct_deallocate<default_allocator<a_struct>>(&a3);

		//destruct deallocate list
		destruct_deallocate_count<default_allocator<a_struct>>(l1, 100);
	}

	//use allocate/reallocate/deallocate directly
	//use new feature of reallocate that allows the efficient movement of memory when reallocating
	cout << "Test 2" << endl;
	{
		default_allocator<a_struct> allctr;

		a_struct* l2 = (a_struct*)allctr.allocate(100 * sizeof(a_struct), std::alignment_of<a_struct>(), sizeof(a_struct));

		//init the memory in l2
		for(unsigned i = 0; i < 100; ++i)
			l2[i] = a_struct{700, 8.5f};

		//insert 100 elements after element 40
		realloc_data rdat = {
			/*void* ptr=*/ l2,
			/*void* hint=*/ 0,
			/*size_t from_byte_size=*/ 100 * sizeof(a_struct),
			/*size_t to_byte_size=*/ 200 * sizeof(a_struct),
			/*size_t keep_byte_size_1=*/ 40 * sizeof(a_struct),
			/*size_t keep_byte_size_2=*/ 60 * sizeof(a_struct),
			/*ssize_t keep_from_byte_offset_1=*/ 0,
			/*ssize_t keep_from_byte_offset_2=*/ 40 * sizeof(a_struct),
			/*ssize_t keep_to_byte_offset_1=*/ 0,
			/*ssize_t keep_to_byte_offset_2=*/ 140 * sizeof(a_struct),
			/*size_t alignment=*/ std::alignment_of<a_struct>(),
			/*size_t size_of=*/ sizeof(a_struct)
		};
		l2 = (a_struct*)allctr.reallocate(&rdat);

		//init the new memory in l2
		for(unsigned i = 40; i < 140; ++i)
			l2[i] = a_struct{300, 2.5f};

		allctr.deallocate(l2, 200 * sizeof(a_struct), std::alignment_of<a_struct>(), sizeof(a_struct));
	}

	//as smaller global memory pools - with or without threading
	//aligns ints to 64 bits
	cout << "Test 3" << endl;
	{
		default_allocator<int, rc_multi_threaded_internal_allocator<std::mutex, ALLOC_PAGE_SIZE, POOLB>> B;
		default_allocator<int, rc_internal_allocator<ALLOC_PAGE_SIZE, POOLC>> C;

		int* l3 = (int*)B.allocate(100 * sizeof(int), std::alignment_of<uint64_t>(), sizeof(int));
		int* l4 = (int*)C.allocate(100 * sizeof(int), std::alignment_of<uint64_t>(), sizeof(int));

		//init memory to 1s
		for(unsigned i = 0; i < 100; ++i)
			l3[i] = 1;
		for(unsigned i = 0; i < 100; ++i)
			l4[i] = 1;

		B.deallocate(l3, 100 * sizeof(int), std::alignment_of<uint64_t>(), sizeof(int));
		C.deallocate(l4, 100 * sizeof(int), std::alignment_of<uint64_t>(), sizeof(int));
	}

	//as replacement for std allocator
	cout << "Test 4" << endl;
	{
		vector<a_struct, default_std_allocator<a_struct>> vec;
		vec.push_back(a_struct{800, 45.0f});
		vec.push_back(a_struct{800, 45.0f});
		vec.push_back(a_struct{800, 45.0f});
	}
	cout << "End Test" << endl;
	return 0;
}
