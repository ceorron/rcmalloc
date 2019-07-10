# rcmalloc - robust/fast/low fragmentation memory allocator, that allows threaded/unthreaded memory pools

Fast reallocation for custom written vector/growable list class.
To improve performance and reduce fragmentation the allocator include a custom reallocate function that allows byte movement ranges to be specified from a custom vector class (not included). Allowing fast and efficient memmove calls during reallocation as well as preventing multiple memmoves usually needed by C realloc function if used with a "vector" class.

MIT Licence - See Source/License file

# Example use - C++

```C++
#include <iostream>

#include "rcmalloc.hpp"

using namespace std;

#define POOLB		1
#define POOLC		1

int main() {
	//TODO add tests
	//use new and delete replacements
	{

	}

	//use allocate/reallocate/deallocate directly
	//use new feature of reallocate that allows the efficient movement of memory when reallocating
	{

	}

	//as smaller global memory pools - with or without threading
	{

	}

	//as replacement for std allocator
	{

	}
	return 0;
}
```

Please use and let me know what you think.

Thanks

ceorron

aka

Richard Cookman