# rcmalloc - robust/easy-to-use/fast/low fragmentation memory allocator, that allows thread-safe/unthreaded memory pools

Features
 - robust, no known bugs or issues
 - fast, allocates large blocks that are divided up for individual allocations preventing slow OS calls to malloc/realloc/free
 - fast, simple design/binary search/pointer arithmetic ensures fast allocation/reallocation/deallocation
 - small, minimal design about 1300 lines of c++ total!!
 - returns aligned memory for all types for faster load and store - alignment can be set on per allocation basis
 - low fragmentation, uses smallest matching size avaliable on allocation
 - smart reallocation function gives lower fragmentation/better locality of reference than default std allocator (using custom vector class) (*)
 - thread-safe, when needed
 - memory pools - v.fast replacement for memory pools that generalises better
 - simple type safe alternatives to new/new[] and delete/delete[]

(*) Fast reallocation for custom written vector/growable list class.
To improve performance and reduce fragmentation the allocator include a custom smart reallocate function that allows byte movement ranges to be specified from a custom vector class (not included). Allowing fast and efficient memmove calls during reallocation as well as preventing multiple memmoves usually needed by C realloc function if used with a "vector" class.

MIT Licence - See Source/License file

# Example use - C++

(examples in main.cpp)

```C++
#include <iostream>
#include <vector>

#include "rcmalloc.hpp"

using namespace std;
using namespace rcmalloc;

struct a_struct {
    unsigned a = 100;
    float b = 3.0;
};

#define POOLB        1
#define POOLC        2

int main() {
    //use new/new[] and delete/delete[] replacements
    cout << "Test 1" << endl;
    {
        //allocate, construct
        a_struct& a1 = *new_T<a_struct>();
        a_struct& a2 = *new_T<a_struct>();
        a_struct& a3 = *new_T(a_struct{300, 10.0f});

        //allocate, construct list
        a_struct* l1 = new_T_array<a_struct>(100);

        //destruct deallocate
        delete_T(&a1);
        delete_T(&a2);
        delete_T(&a3);

        //destruct deallocate list
        delete_T_array(l1, 100);
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
            /*size_t size_of=*/ sizeof(a_struct),
            /*object_move_func move_func=*/ object_move_generator<a_struct>::object_move,
            /*object_move_func intermediary_move_func=*/ object_move_generator<a_struct>::object_intermediary_move
        };
        l2 = (a_struct*)allctr.reallocate(&rdat);

        //init the new memory in l2
        for(unsigned i = 40; i < 140; ++i)
            l2[i] = a_struct{250, 2.5f};

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
        vec.push_back(a_struct{350, 30.0f});
        vec.push_back(a_struct{180, 72.0f});
    }
    cout << "End Test" << endl;
    return 0;
}
```

Please use and let me know what you think.

Thanks

ceorron

aka

Richard Cookman

