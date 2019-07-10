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
void* operator new(std::size_t count) _GLIBCXX_THROW (std::bad_alloc) {
	rcmalloc::default_allocator<char> alloc;
	void* rtn = alloc.allocate(count + sizeof(unsigned), std::alignment_of<unsigned>(), 1);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new[](std::size_t count) _GLIBCXX_THROW (std::bad_alloc) {
	rcmalloc::default_allocator<char> alloc;
	void* rtn = alloc.allocate(count + sizeof(unsigned), std::alignment_of<unsigned>(), 1);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new(std::size_t count, const std::nothrow_t&) noexcept {
	rcmalloc::default_allocator<char> alloc;
	void* rtn = alloc.allocate(count + sizeof(unsigned), std::alignment_of<unsigned>(), 1);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void* operator new[](std::size_t count, const std::nothrow_t&) noexcept {
	rcmalloc::default_allocator<char> alloc;
	void* rtn = alloc.allocate(count + sizeof(unsigned), std::alignment_of<unsigned>(), 1);
	*((unsigned*)rtn) = count;
	return (char*)rtn + sizeof(unsigned);
}
void operator delete(void* ptr) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	alloc.deallocate(ptr, sz, std::alignment_of<unsigned>(), 1);
}
void operator delete[](void* ptr) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	std::size_t sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	alloc.deallocate(ptr, sz, std::alignment_of<unsigned>(), 1);
}
void operator delete(void* ptr, std::size_t sz) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	alloc.deallocate(ptr, sz, std::alignment_of<unsigned>(), 1);
}
void operator delete[](void* ptr, std::size_t sz) noexcept {
	ptr = (char*)ptr - sizeof(unsigned);
	sz = *((unsigned*)ptr) + sizeof(unsigned);

	rcmalloc::default_allocator<char> alloc;
	alloc.deallocate(ptr, sz, std::alignment_of<unsigned>(), 1);
}
