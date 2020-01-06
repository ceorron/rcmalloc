/*----------------------------------------------------------------------------------*\
 |																					|
 | rcnewdelete.cpp	 																|
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

#include "rcnewdelete.hpp"

//replace allocator new and delete
void* operator new(std::size_t count) STLIB_NEWTHROW {
	rcmalloc::default_allocator<char> alloc;
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = std::alignment_of<unsigned>();
	adat.size_of = 1;
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new[](std::size_t count) STLIB_NEWTHROW {
	rcmalloc::default_allocator<char> alloc;
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = std::alignment_of<unsigned>();
	adat.size_of = 1;
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
#if __cpp_aligned_new
void* operator new(std::size_t count, std::align_val_t al) STLIB_NEWTHROW {
	rcmalloc::default_allocator<char> alloc;
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = tal;
	adat.size_of = (tal > 0 ? tal : 1);
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new[](std::size_t count, std::align_val_t al) STLIB_NEWTHROW {
	rcmalloc::default_allocator<char> alloc;
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = tal;
	adat.size_of = (tal > 0 ? tal : 1);
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
#endif
void* operator new(std::size_t count, const std::nothrow_t&) noexcept {
	rcmalloc::default_allocator<char> alloc;
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = std::alignment_of<unsigned>();
	adat.size_of = 1;
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new[](std::size_t count, const std::nothrow_t&) noexcept {
	rcmalloc::default_allocator<char> alloc;
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = std::alignment_of<unsigned>();
	adat.size_of = 1;
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
#if __cpp_aligned_new
void* operator new(std::size_t count,
				   std::align_val_t al, const std::nothrow_t&) noexcept {
	rcmalloc::default_allocator<char> alloc;
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = tal;
	adat.size_of = (tal > 0 ? tal : 1);
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new[](std::size_t count,
					 std::align_val_t al, const std::nothrow_t&) noexcept {
	rcmalloc::default_allocator<char> alloc;
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());
	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	adat.size = count + sizeof(unsigned);
	adat.alignment = tal;
	adat.size_of = (tal > 0 ? tal : 1);
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
#endif

void operator delete(void* ptr) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = std::alignment_of<unsigned>();
	ddat.size_of = 1;
	alloc.deallocate(&ddat);
}
void operator delete[](void* ptr) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = std::alignment_of<unsigned>();
	ddat.size_of = 1;
	alloc.deallocate(&ddat);
}
#if __cpp_aligned_new
void operator delete(void* ptr, std::align_val_t al) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sz = *((unsigned*)ptr) + sizeof(unsigned);
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = tal;
	ddat.size_of = (tal > 0 ? tal : 1);
	alloc.deallocate(&ddat);
}
void operator delete[](void* ptr, std::align_val_t al) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sz = *((unsigned*)ptr) + sizeof(unsigned);
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = tal;
	ddat.size_of = (tal > 0 ? tal : 1);
	alloc.deallocate(&ddat);
}
#endif
void operator delete(void* ptr, std::size_t sz) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = std::alignment_of<unsigned>();
	ddat.size_of = 1;
	alloc.deallocate(&ddat);
}
void operator delete[](void* ptr, std::size_t sz) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = std::alignment_of<unsigned>();
	ddat.size_of = 1;
	alloc.deallocate(&ddat);
}
#if __cpp_aligned_new
void operator delete(void* ptr, std::size_t sz,
					 std::align_val_t al) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sze = *((unsigned*)ptr) + sizeof(unsigned);
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sze;
	ddat.alignment = tal;
	ddat.size_of = (tal > 0 ? tal : 1);
	alloc.deallocate(&ddat);
}
void operator delete[](void* ptr, std::size_t sz,
					   std::align_val_t al) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sze = *((unsigned*)ptr) + sizeof(unsigned);
	size_t tal = ((int)al > std::alignment_of<unsigned>() ? (size_t)al : (size_t)std::alignment_of<unsigned>());

	rcmalloc::default_allocator<char> alloc;
	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sze;
	ddat.alignment = tal;
	ddat.size_of = (tal > 0 ? tal : 1);
	alloc.deallocate(&ddat);
}
#endif
