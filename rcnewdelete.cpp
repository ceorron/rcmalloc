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

void* aligned_new_allocate(std::size_t count, size_t al) {
	rcmalloc::default_allocator<char> alloc;

	rcmalloc::alloc_data adat = rcmalloc::init_alloc_data_basic();
	unsigned count_alignment = std::alignment_of<uint32_t>();

	unsigned offset = 0;
	if(count_alignment >= al) {
		adat.size = count + sizeof(uint32_t);
		adat.size_of = 1;
		offset = sizeof(uint32_t);
	} else {
		adat.size = count + al;
		adat.size_of = 1;
		offset = al;
	}
	adat.alignment = (std::alignment_of<uint32_t>() > al ? std::alignment_of<uint32_t>() : al);
	void* rtn = alloc.allocate(&adat);
	*((unsigned*)rtn) = count;
	return (char*)rtn + offset;
}
inline void* general_new_allocate(std::size_t count) {
	return aligned_new_allocate(count, std::alignment_of<void*>());
}

//replace allocator new and delete
void* operator new(std::size_t count) STLIB_NEWTHROW {
	return general_new_allocate(count);
}
void* operator new[](std::size_t count) STLIB_NEWTHROW {
	return general_new_allocate(count);
}
#if __cpp_aligned_new
void* operator new(std::size_t count, std::align_val_t al) STLIB_NEWTHROW {
	return aligned_new_allocate(count, (const size_t&)al);
}
void* operator new[](std::size_t count, std::align_val_t al) STLIB_NEWTHROW {
	return aligned_new_allocate(count, (const size_t&)al);
}
#endif
void* operator new(std::size_t count, const std::nothrow_t&) noexcept {
	return general_new_allocate(count);
}
void* operator new[](std::size_t count, const std::nothrow_t&) noexcept {
	return general_new_allocate(count);
}
#if __cpp_aligned_new
void* operator new(std::size_t count,
				   std::align_val_t al, const std::nothrow_t&) noexcept {
	return aligned_new_allocate(count, (const size_t&)al);
}
void* operator new[](std::size_t count,
					 std::align_val_t al, const std::nothrow_t&) noexcept {
	return aligned_new_allocate(count, (const size_t&)al);
}
#endif


void aligned_delete_deallocate(void* ptr, size_t al) {
	rcmalloc::default_allocator<char> alloc;

	unsigned count_alignment = std::alignment_of<uint32_t>();
	unsigned offset = 0;
	if(count_alignment >= al)
		offset = sizeof(uint32_t);
	else
		offset = al;
	ptr = (char*)ptr - offset;
	size_t sz = *(unsigned*)ptr + offset;

	rcmalloc::dealloc_data ddat = rcmalloc::init_dealloc_data_basic();
	ddat.ptr = ptr;
	ddat.size = sz;
	ddat.alignment = (std::alignment_of<uint32_t>() > al ? std::alignment_of<uint32_t>() : al);
	ddat.size_of = 1;
	alloc.deallocate(&ddat);
}
inline void general_delete_deallocate(void* ptr) {
	aligned_delete_deallocate(ptr, std::alignment_of<void*>());
}

void operator delete(void* ptr) noexcept {
	general_delete_deallocate(ptr);
}
void operator delete[](void* ptr) noexcept {
	general_delete_deallocate(ptr);
}
#if __cpp_aligned_new
void operator delete(void* ptr, std::align_val_t al) noexcept {
	aligned_delete_deallocate(ptr, (const size_t&)al);
}
void operator delete[](void* ptr, std::align_val_t al) noexcept {
	aligned_delete_deallocate(ptr, (const size_t&)al);
}
#endif
void operator delete(void* ptr, std::size_t sz) noexcept {
	general_delete_deallocate(ptr);
}
void operator delete[](void* ptr, std::size_t sz) noexcept {
	general_delete_deallocate(ptr);
}
#if __cpp_aligned_new
void operator delete(void* ptr, std::size_t sz,
					 std::align_val_t al) noexcept {
	aligned_delete_deallocate(ptr, (const size_t&)al);
}
void operator delete[](void* ptr, std::size_t sz,
					   std::align_val_t al) noexcept {
	aligned_delete_deallocate(ptr, (const size_t&)al);
}
#endif

