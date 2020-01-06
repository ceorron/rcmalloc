/*----------------------------------------------------------------------------------*\
 |																					|
 | rcnewdelete.hpp	 																|
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
#pragma once

#include <new>
#include "rcmalloc.hpp"

#if !defined(STLIB_NEWTHROW)
#  if defined(_MSC_VER)
#    define _THROW1(x)
#    define STLIB_NEWTHROW _THROW1(_STD bad_alloc)
#  elif defined(__GNUC__)
#    define STLIB_NEWTHROW _GLIBCXX_THROW(std::bad_alloc)
#  endif
#endif

//replace allocator new and delete
void* operator new(std::size_t count) STLIB_NEWTHROW;
void* operator new[](std::size_t count) STLIB_NEWTHROW;
#if __cpp_aligned_new
void* operator new(std::size_t count, std::align_val_t al) STLIB_NEWTHROW;
void* operator new[](std::size_t count, std::align_val_t al) STLIB_NEWTHROW;
#endif
void* operator new(std::size_t count, const std::nothrow_t&) noexcept;
void* operator new[](std::size_t count, const std::nothrow_t&) noexcept;
#if __cpp_aligned_new
void* operator new(std::size_t count,
				   std::align_val_t al, const std::nothrow_t&) noexcept;
void* operator new[](std::size_t count,
					 std::align_val_t al, const std::nothrow_t&) noexcept;
#endif

void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
#if __cpp_aligned_new
void operator delete(void* ptr, std::align_val_t al) noexcept;
void operator delete[](void* ptr, std::align_val_t al) noexcept;
#endif
void operator delete(void* ptr, std::size_t sz) noexcept;
void operator delete[](void* ptr, std::size_t sz) noexcept;
#if __cpp_aligned_new
void operator delete(void* ptr, std::size_t sz,
					 std::align_val_t al) noexcept;
void operator delete[](void* ptr, std::size_t sz,
					   std::align_val_t al) noexcept;
#endif
