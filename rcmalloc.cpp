/*----------------------------------------------------------------------------------*\
 |																					|
 | rcmalloc.cpp	 																	|
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

#include "rcmalloc.hpp"

namespace rcmalloc {

alloc_data init_alloc_data_basic() {
	alloc_data rtn;
	memset((char*)&rtn, 0, sizeof(alloc_data));
	rtn.minalignment = sizeof(uintptr_t);
	rtn.byterounding = sizeof(uintptr_t);
	return rtn;
}
realloc_data init_realloc_data_basic() {
	realloc_data rtn;
	memset((char*)&rtn, 0, sizeof(realloc_data));
	rtn.minalignment = sizeof(uintptr_t);
	rtn.byterounding = sizeof(uintptr_t);
	return rtn;
}
alloc_data to_alloc_data(const realloc_data* dat) {
	//copy across everything needed
	alloc_data rtn;
	memset((char*)&rtn, 0, sizeof(alloc_data));
	rtn.size = dat->to_byte_size;
	rtn.alignment = dat->alignment;
	rtn.size_of = dat->size_of;
	rtn.minalignment = dat->minalignment;
	rtn.byterounding = dat->byterounding;
	return rtn;
}
dealloc_data init_dealloc_data_basic() {
	dealloc_data rtn;
	memset((char*)&rtn, 0, sizeof(dealloc_data));
	rtn.minalignment = sizeof(uintptr_t);
	rtn.byterounding = sizeof(uintptr_t);
	return rtn;
}

vgcsettings::~vgcsettings() {}

vallocator::~vallocator() {}
void vallocator::do_add_stack_variable(void* stkptr, stack_variable_cleanup fptr) {
	//add pointer to stack item
	//NEEDED by garbage collectors only
}
void vallocator::do_remove_stack_variable_range(void* stkptr, uint32_t frame_size) {
	//remove pointers to stack items
	//NEEDED by garbage collectors only
}
void vallocator::do_cleanup(const vgcsettings& settings) {
	//do stop the world cleanup
	//NEEDED by garbage collectors only
}
void vallocator::do_test_cleanup(const vgcsettings& settings) {
	//do stop the world cleanup
	//NEEDED by garbage collectors only
}
void* vallocator::do_dereference(void* ptr) {
	//pointer dereference
	//NEEDED by garbage collectors only
	return ptr;
}
vallocator& vallocator::get_allocator() {
	return *this;
}
const vallocator& vallocator::get_allocator() const {
	return *this;
}

void roundAllocation(uint32_t minalignment, uint32_t byterounding,
					 uint32_t& size, uint32_t& alignment) {
	if(size == 0) size = 1;
	if(alignment < minalignment) alignment = minalignment;
	uint32_t md = size % byterounding;
	if(md > 0) size += byterounding - md;
}
void roundAllocation(realloc_data& ldat) {
	roundAllocation(ldat.minalignment, ldat.byterounding, ldat.from_byte_size, ldat.alignment);
	roundAllocation(ldat.minalignment, ldat.byterounding, ldat.to_byte_size, ldat.alignment);
}
void* align(uint32_t alignment, uint32_t size_of, void*& ptr) {
	//we are not concerned about the size of the resulting memory location just pass in a large value
	size_t size = size_of * 10;
	std::align(alignment,
			   size_of,
			   ptr,
			   size);
	return ptr;
}
char getMemOffset(void* ptr, uint32_t alignment, uint32_t size_of) {
	if(alignment < 2)
		return 0;

	void* rtn = ptr;
	rcmalloc::align(alignment,
					size_of,
					rtn);

	if(rtn == ptr)
		rtn = (char*)rtn + alignment;

	//store the offset to the true block of this
	return dist((char*)ptr, (char*)rtn);
}
void* setAlignment(void* ptr, uint32_t alignment, uint32_t size_of) {
	if(alignment < 2)
		return ptr;

	void* rtn = ptr;
	rcmalloc::align(alignment,
					size_of,
					rtn);

	if(rtn == ptr)
		rtn = (char*)rtn + alignment;

	//store the offset to the true block of this
	char offset = dist((char*)ptr, (char*)rtn);
	*((char*)rtn - 1) = offset;
	return rtn;
}
void* getAlignment(void* ptr, uint32_t alignment, uint32_t& size, uint32_t& offset) {
	if(alignment < 2) {
		offset = 0;
		return ptr;
	}

	size += alignment;
	offset = *((char*)ptr - 1);
	return (char*)ptr - offset;
}

void move_object_list_forward(void* begfrm, void* endfrm, void* begto, uint32_t count,
							  uint32_t size_of, object_move_func move_func) {
	char* lclbegfrm = (char*)begfrm;
	//char* lclendfrm = (char*)endfrm;
	char* lclbegto = (char*)begto;

	for(; count > 0; lclbegfrm+=size_of, lclbegto+=size_of, --count)
		move_func(lclbegfrm, lclbegto);
}
void move_object_list_backward(void* begfrm, void* endfrm, void* endto, uint32_t count,
							   uint32_t size_of, object_move_func move_func) {
	//char* lclbegfrm = (char*)begfrm - size_of;
	char* lclendfrm = (char*)endfrm - size_of;
	char* lclendto = (char*)endto - size_of;

	for(; count > 0; lclendfrm-=size_of, lclendto-=size_of, --count)
		move_func(lclendfrm, lclendto);
}
void memMove(void* begfrm, void* endfrm, void* begto, void* endto,
			 uint32_t count, const realloc_data& dat) {
	//no move if moving to same place
	if(begfrm == begto)
		return;

	if(dat.istrivial) {
		//better performance for trivially copyable types
		memmove((char*)begto, (char*)begfrm, dist((char*)begfrm, (char*)endfrm));
	} else {
		object_move_func mvfunc;
		if((uint32_t)abs(dist((char*)begfrm, (char*)begto)) < dat.size_of)
			//if we need an intermediary (partial object overlap)
			mvfunc = dat.intermediary_move_func;
		else
			mvfunc = dat.move_func;
		if((char*)begto >= (char*)begfrm && (char*)begto < (char*)endfrm) {
			//overlap at the beginning
			move_object_list_backward(begfrm, endfrm, endto, count,
									  dat.size_of, mvfunc);
			return;
		} else if((char*)endto >= (char*)begfrm && (char*)endto < (char*)endfrm) {
			//overlap at the end
			move_object_list_forward(begfrm, endfrm, begto, count,
									 dat.size_of, mvfunc);
			return;
		}
		//just move
		move_object_list_forward(begfrm, endfrm, begto, count,
								 dat.size_of, dat.move_func);
	}
	return;
}
inline bool moveEndFirst(char* ptr1, int32_t keep_from_byte_offset,
						 char* ptr2, int32_t keep_to_byte_offset) {
	return (ptr2 + keep_to_byte_offset) > (ptr1 + keep_from_byte_offset);
}
void* doMemMove(char* frmPtr, char* toPtr,
				const realloc_data& dat) {
	uint32_t keep_byte_size_1 = dat.keep_byte_size_1;
	uint32_t keep_byte_size_2 = dat.keep_byte_size_2;
	int32_t keep_from_byte_offset_1 = dat.keep_from_byte_offset_1;
	int32_t keep_from_byte_offset_2 = dat.keep_from_byte_offset_2;
	int32_t keep_to_byte_offset_1 = dat.keep_to_byte_offset_1;
	int32_t keep_to_byte_offset_2 = dat.keep_to_byte_offset_2;
	uint32_t count_1 = dat.from_count_1;
	uint32_t count_2 = dat.from_count_2;

	//move the memory
	if(moveEndFirst((char*)frmPtr, keep_from_byte_offset_2,
					(char*)toPtr, keep_to_byte_offset_2)) {
		std::swap(keep_from_byte_offset_1, keep_from_byte_offset_2);
		std::swap(keep_to_byte_offset_1, keep_to_byte_offset_2);
		std::swap(keep_byte_size_1, keep_byte_size_2);
		std::swap(count_1, count_2);
	}
	//memmove((char*)toPtr + keep_to_byte_offset_1, (char*)frmPtr + keep_from_byte_offset_1, keep_byte_size_1);
	memMove((char*)frmPtr + keep_from_byte_offset_1, (char*)frmPtr + keep_from_byte_offset_1 + keep_byte_size_1,
			(char*)toPtr + keep_to_byte_offset_1, (char*)toPtr + keep_to_byte_offset_1 + keep_byte_size_1,
			count_1, dat);
	//memmove((char*)toPtr + keep_to_byte_offset_2, (char*)frmPtr + keep_from_byte_offset_2, keep_byte_size_2);
	memMove((char*)frmPtr + keep_from_byte_offset_2, (char*)frmPtr + keep_from_byte_offset_2 + keep_byte_size_2,
			(char*)toPtr + keep_to_byte_offset_2, (char*)toPtr + keep_to_byte_offset_2 + keep_byte_size_2,
			count_2, dat);
	return toPtr;
}

void sortMemUp(basic_list& sizes, bytesizes* itr) {
	//move this about in the sizes list
	while(itr != begin_basic_list<bytesizes>(sizes)) {
		auto tmp = itr;
		--tmp;
		//test less - not less than then break
		if(!(itr->bytecount < tmp->bytecount || (itr->bytecount == tmp->bytecount && itr->ptr < tmp->ptr)))
			break;
		std::swap(*tmp, *itr);
		--itr;
	}
}
void sortMemDown(basic_list& sizes, bytesizes* itr) {
	//move this about in the sizes list
	while(itr != end_basic_list<bytesizes>(sizes) - 1) {
		auto tmp = itr;
		++tmp;
		//test less - not less than then break
		if(!(tmp->bytecount < itr->bytecount || (tmp->bytecount == itr->bytecount && tmp->ptr < itr->ptr)))
			break;
		std::swap(*tmp, *itr);
		++itr;
	}
}
void memblock::init() {
	sizes = init_basic_list<bytesizes>(30);
	freelst = init_basic_list<bytesizes>(30);
}
memblock::~memblock() {
	//NOTE doesn't free ptr here - faster final cleanup!!!
	/*uint32_t bytetotal;
	uint32_t byteremain;
	char* ptr;*/
	dtor_basic_list<bytesizes>(sizes);
	dtor_basic_list<bytesizes>(freelst);
}
void* memblock::internal_malloc_at_hint(uint32_t size, bytesizes* pfrelst, void* hint) {
	//can we allocate here??
	if(pfrelst != end_basic_list<bytesizes>(freelst) &&
	   (char*)hint >= pfrelst->ptr && ((char*)hint + size) <= (pfrelst->ptr + pfrelst->bytecount)) {
		//get the size for this
		bytesizes* sout;
		rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), *pfrelst,
			[](const bytesizes& lhs,
			   const bytesizes& rhs) {
				return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
			}, sout);

		//allocate this here and now
		if(((char*)hint == pfrelst->ptr) & (((char*)hint + size) == (pfrelst->ptr + pfrelst->bytecount))) {
			//takes whole block - remove block and size
			erase_basic_list<bytesizes>(freelst, pfrelst);
			erase_basic_list<bytesizes>(sizes, sout);
		} else if((char*)hint == pfrelst->ptr) {
			//matches front of block
			pfrelst->bytecount -= size;
			pfrelst->ptr += size;
			sout->bytecount -= size;
			sout->ptr += size;

			sortMemUp(sizes, sout);
		} else if(((char*)hint + size) == (pfrelst->ptr + pfrelst->bytecount)) {
			//matches back of block
			pfrelst->bytecount -= size;
			sout->bytecount -= size;

			sortMemUp(sizes, sout);
		} else {
			//split block and size
			bytesizes nszs;
			nszs.bytecount = pfrelst->bytecount - (dist(pfrelst->ptr, (char*)hint) + size);
			nszs.ptr = (char*)hint + size;
			bytesizes tszs = nszs;

			pfrelst->bytecount = dist(pfrelst->ptr, (char*)hint);
			sout->bytecount = dist(pfrelst->ptr, (char*)hint);

			sortMemUp(sizes, sout);

			//insert a new block after this one
			insert_basic_list<bytesizes>(freelst, pfrelst + 1, std::move(nszs));

			bytesizes* iout;
			rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), tszs,
				[](const bytesizes& lhs,
				   const bytesizes& rhs) {
					return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
				}, iout);
			insert_basic_list<bytesizes>(sizes, iout, std::move(tszs));
		}
		return hint;
	}
	return 0;
}
void* memblock::internal_malloc(uint32_t size) {
	//NOTE size always > 0
	if(byteremain < size) return 0;

	bytesizes bszs;
	bszs.bytecount = size;

	//search the sizes
	bytesizes* sout;
	rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), bszs,
		[](const bytesizes& lhs,
		   const bytesizes& rhs) {
			return lhs.bytecount < rhs.bytecount;
		}, sout);

	//couldn't find big enough
	if(sout == end_basic_list<bytesizes>(sizes)) return 0;

	//search the pointers
	bszs.ptr = sout->ptr;
	bytesizes* pout;
	rcmalloc::binary_search(begin_basic_list<bytesizes>(freelst), end_basic_list<bytesizes>(freelst), bszs,
		[](const bytesizes& lhs,
		   const bytesizes& rhs) {
			return lhs.ptr < rhs.ptr;
		}, pout);

	void* rslt = pout->ptr;

	byteremain -= size;
	if(sout->bytecount == size) {
		//just remove both of these
		erase_basic_list<bytesizes>(freelst, pout);
		erase_basic_list<bytesizes>(sizes, sout);
	} else {
		//"split" this
		//modify both of these
		sout->bytecount -= size;
		sout->ptr += size;

		pout->bytecount -= size;
		pout->ptr += size;

		//move this about in the sizes list
		sortMemUp(sizes, sout);
	}
	return rslt;
}
inline bool connectsBefore(basic_list& freelst, const bytesizes& fm,
						   bytesizes* before,
						   bytesizes* after) {
	if(before == begin_basic_list<bytesizes>(freelst) - 1)
		return false;
	return (before->ptr + before->bytecount) == fm.ptr;
}
inline bool connectsAfter(basic_list& freelst, const bytesizes& fm,
						  bytesizes* before,
						  bytesizes* after) {
	if(after == end_basic_list<bytesizes>(freelst))
		return false;
	return (fm.ptr + fm.bytecount) == after->ptr;
}
inline bool connectsBeforeAndAfter(basic_list& freelst, const bytesizes& fm,
								   bytesizes* before,
								   bytesizes* after) {
	return connectsBefore(freelst, fm, before, after) &&
		   connectsAfter(freelst, fm, before, after);
}
void* memblock::internal_realloc(
		const realloc_data* dat,
		char offset,
		bytesizes*& freeOut
) {
	/*hints - user hint*/
	bytesizes* bsize0 = 0;
	void* hint0 = dat->hint;
	/*keep before in place - minimise move - just expand*/
	bytesizes* bsize1 = 0;
	void* hint1 = 0;
	/*keep after in place - minimise move - just expand*/
	bytesizes* bsize2 = 0;
	void* hint2 = 0;
	/*give this space at the beginning - minimise relocation*/
	bytesizes* bsize3 = 0;
	void* hint3 = 0;
	/*into the current memory block - minimise fragmentation*/
	bytesizes* bsize4 = 0;
	void* hint4 = 0;

	if(!dat->ptr) {
		void* rslt = 0;
		if(rslt == 0 && hint0) {
			bytesizes bszs;
			bszs.bytecount = dat->to_byte_size;
			bszs.ptr = (char*)hint0;
			rcmalloc::binary_search(begin_basic_list<bytesizes>(freelst), end_basic_list<bytesizes>(freelst), bszs,
				[](const bytesizes& lhs,
				   const bytesizes& rhs) {
					return lhs.ptr < rhs.ptr;
				}, bsize0);
			//try to allocate at the hint
			rslt = internal_malloc_at_hint(dat->to_byte_size, bsize0, hint0);
		}
		if(rslt == 0)
			rslt = internal_malloc(dat->to_byte_size);
		if(rslt == 0)
			return 0;
		//do alignment
		if(dat->alignment < 2)
			return rslt;

		void* rtn = rslt;
		rcmalloc::align(dat->alignment,
						dat->size_of,
						rtn);

		if(rtn == rslt)
			rtn = (char*)rtn + dat->alignment;

		//store the offset to the true block of this
		char offset = dist((char*)rslt, (char*)rtn);
		*((char*)rtn - 1) = offset;
		return rtn;
	}

	//do free before allocation!
	internal_free(dat->ptr, dat->from_byte_size, freeOut);

	void* rslt = 0;
	{
		//calculate the hints
		if(rslt == 0 && hint0) {
			bytesizes bszs;
			bszs.bytecount = dat->to_byte_size;
			bszs.ptr = (char*)hint0;
			rcmalloc::binary_search(begin_basic_list<bytesizes>(freelst), end_basic_list<bytesizes>(freelst), bszs,
				[](const bytesizes& lhs,
				   const bytesizes& rhs) {
					return lhs.ptr < rhs.ptr;
				}, bsize0);
			//try to allocate at the hint
			rslt = internal_malloc_at_hint(dat->to_byte_size, bsize0, hint0);
		}

		if(rslt == 0) {
			//keep the front the same
			bsize1 = freeOut;
			hint1 = ((char*)dat->ptr + offset + dat->keep_from_byte_offset_1) - dat->keep_to_byte_offset_1 - offset;
			//keep the back the same
			bsize2 = freeOut;
			hint2 = ((char*)dat->ptr + offset + dat->keep_from_byte_offset_2) - dat->keep_to_byte_offset_2 - offset;

			//try to allocate the largest of the two first
			if(dat->keep_byte_size_2 > dat->keep_byte_size_1) {
				std::swap(hint1, hint2);
				std::swap(bsize1, bsize2);
			}

			if(rslt == 0)
				rslt = internal_malloc_at_hint(dat->to_byte_size, bsize1, hint1);
			if(rslt == 0)
				rslt = internal_malloc_at_hint(dat->to_byte_size, bsize2, hint2);
		}

		//move this into the largest of the free blocks - give this space at the beginning
		if(rslt == 0) {
			bytesizes bszs = *(end_basic_list<bytesizes>(sizes) - 1);
			rcmalloc::binary_search(begin_basic_list<bytesizes>(freelst), end_basic_list<bytesizes>(freelst), bszs,
				[](const bytesizes& lhs,
				   const bytesizes& rhs) {
					return lhs.ptr < rhs.ptr;
				}, bsize3);
			hint3 = bsize3->ptr + dat->to_byte_size;

			rslt = internal_malloc_at_hint(dat->to_byte_size, bsize3, hint3);
		}

		if(rslt == 0) {
			bsize4 = freeOut;
			hint4 = freeOut->ptr;
			rslt = internal_malloc_at_hint(dat->to_byte_size, bsize4, hint4);
		}

		//tried malloc at all of the hint locations - just malloc
		if(rslt == 0)
			rslt = internal_malloc(dat->to_byte_size);
		if(rslt == 0)
			return 0;
	}


	//do alignment
	if(dat->alignment >= 2) {
		void* rtn = rslt;
		rcmalloc::align(dat->alignment,
						dat->size_of,
						rtn);

		if(rtn == rslt)
			rtn = (char*)rtn + dat->alignment;

		//store the offset to the true block of this
		char offset = dist((char*)rslt, (char*)rtn);
		*((char*)rtn - 1) = offset;
		rslt = rtn;
	}

	//do memove
	return doMemMove((char*)dat->ptr + offset, (char*)rslt, *dat);
}
void memblock::internal_free(void* p, uint32_t size, bytesizes*& freeOut) {
	/*uint32_t bytetotal;
	uint32_t byteremain;
	char* ptr;
	vector<bytesizes, basic_aligned_allocator<bytesizes>> sizes;
	vector<bytecount, basic_aligned_allocator<bytecount>> freelst;*/

	//if this isn't within this!
	if((char*)p < ptr || (char*)p >= (ptr + bytetotal))
		return;

	if(byteremain == 0) {
		//simply return this, everything allocated
		byteremain += size;

		bytesizes fm;
		fm.bytecount = size;
		fm.ptr = (char*)p;
		bytesizes tm = fm;

		push_back_basic_list<bytesizes>(sizes, std::move(fm));
		push_back_basic_list<bytesizes>(freelst, std::move(tm));
		freeOut = begin_basic_list<bytesizes>(freelst);
		return;
	}

	//find the before and after on the free list
	bytesizes fm;
	fm.bytecount = size;
	fm.ptr = (char*)p;
	bytesizes* before;
	bytesizes* after;
	rcmalloc::binary_search(begin_basic_list<bytesizes>(freelst), end_basic_list<bytesizes>(freelst), fm,
		[](const bytesizes& lhs,
		   const bytesizes& rhs) {
			return lhs.ptr < rhs.ptr;
		}, after);

	before = after;
	--before;

	if(connectsBeforeAndAfter(freelst, fm, before, after)) {
		//increase the size of before
		//search the sizes to update
		bytesizes* sout;
		rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), *before,
			[](const bytesizes& lhs,
			   const bytesizes& rhs) {
				return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
			}, sout);
		bytesizes* aout;
		rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), *after,
			[](const bytesizes& lhs,
			   const bytesizes& rhs) {
				return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
			}, aout);

		before->bytecount += fm.bytecount;
		before->bytecount += after->bytecount;
		*sout = *before;

		//remove after
		uint32_t beforePos = dist(begin_basic_list<bytesizes>(sizes), sout);
		uint32_t afterPos = dist(begin_basic_list<bytesizes>(sizes), aout);

		freeOut = erase_basic_list<bytesizes>(freelst, after);
		erase_basic_list<bytesizes>(sizes, aout);
		--freeOut;

		if(afterPos <= beforePos)
			--beforePos;

		sortMemDown(sizes, begin_basic_list<bytesizes>(sizes) + beforePos);
	} else if(connectsBefore(freelst, fm, before, after)) {
		//increase the size of before
		//search the sizes to update
		bytesizes* sout;
		rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), *before,
			[](const bytesizes& lhs,
			   const bytesizes& rhs) {
				return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
			}, sout);

		before->bytecount += fm.bytecount;
		*sout = *before;
		sortMemDown(sizes, sout);
		freeOut = before;
	} else if(connectsAfter(freelst, fm, before, after)) {
		//increate the size of after
		//search the sizes to update
		bytesizes* sout;
		rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), *after,
			[](const bytesizes& lhs,
			   const bytesizes& rhs) {
				return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
			}, sout);

		after->ptr = fm.ptr;
		after->bytecount += fm.bytecount;
		*sout = *after;
		sortMemDown(sizes, sout);
		freeOut = after;
	} else {
		bytesizes tm = fm;
		//connects neither - reinsert
		freeOut = insert_basic_list<bytesizes>(freelst, after, std::move(fm));

		//search the sizes to insert
		bytesizes* sout;
		rcmalloc::binary_search(begin_basic_list<bytesizes>(sizes), end_basic_list<bytesizes>(sizes), tm,
			[](const bytesizes& lhs,
			   const bytesizes& rhs) {
				return (lhs.bytecount < rhs.bytecount || (lhs.bytecount == rhs.bytecount && lhs.ptr < rhs.ptr));
			}, sout);
		insert_basic_list<bytesizes>(sizes, sout, std::move(tm));
	}

	byteremain += size;
	return;
}

void addMemBlock(basic_list& blocklst, memblock* nMmBlck) {
	memblock** out;
	rcmalloc::binary_search(begin_basic_list<memblock*>(blocklst), end_basic_list<memblock*>(blocklst), nMmBlck,
		[](memblock* lhs, memblock* rhs) {
			return lhs->ptr < rhs->ptr;
		}, out);
	insert_basic_list<memblock*>(blocklst, out, std::move(nMmBlck));
}
void findBlockForPointer(basic_list& blocklst, void* ptr,
						 memblock**& out) {
	memblock stkBlck;
	stkBlck.ptr = (char*)ptr;
	memblock* sMmBlck = &stkBlck;

	bool rtn = rcmalloc::binary_search(begin_basic_list<memblock*>(blocklst), end_basic_list<memblock*>(blocklst), sMmBlck,
					[=](memblock* lhs, memblock* rhs) {
						return lhs->ptr < rhs->ptr;
					}, out);
	//if we don't find it then, go one back this is what we are searching for
	if(!rtn)
		--out;
}
void sortMemBlockDown(basic_list& blockfreespace,
					  memblock** blk) {
	//search blockfreespace for this block
	auto itr = end_basic_list<memblock*>(blockfreespace) - 1;
	for(; itr != begin_basic_list<memblock*>(blockfreespace) - 1; --itr)
		if(*itr == *blk)
			break;

	//move this about in the sizes list
	while(itr != end_basic_list<memblock*>(blockfreespace) - 1) {
		auto tmp = itr;
		++tmp;
		//test less - not less than then break
		if(!((*itr)->byteremain > (*tmp)->byteremain))
			break;
		std::swap(*tmp, *itr);
		++itr;
	}
}


}

