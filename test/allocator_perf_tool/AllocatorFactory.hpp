/*
* Copyright (C) 2015 Intel Corporation.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice(s),
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice(s),
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
* EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "Allocator.hpp"
#include "StandardAllocatorWithTimer.hpp"
#include "VectorIterator.hpp"
#include "Configuration.hpp"
#include "JemallocAllocatorWithTimer.hpp"
#include "MemkindAllocatorWithTimer.hpp"
#include "memkind.h"

#include <vector>
#include <assert.h>
#include <string.h>

class AllocatorFactory
{
public:
	AllocatorFactory() :
		memkind_default(MEMKIND_DEFAULT, AllocatorTypes::MEMKIND_DEFAULT),
		memkind_hbw(MEMKIND_HBW, AllocatorTypes::MEMKIND_HBW),
		memkind_interleave(MEMKIND_INTERLEAVE, AllocatorTypes::MEMKIND_INTERLEAVE)
	{}

	//Get existing allocator without creating new.
	//The owner of existing allocator is AllocatorFactory object.
	Allocator* get_existing(unsigned type)
	{
		switch(type)
		{
			case AllocatorTypes::STANDARD_ALLOCATOR:
				return &standard_allocator;

			case AllocatorTypes::JEMALLOC:
				return &jemalloc;

			case AllocatorTypes::MEMKIND_DEFAULT:
				return &memkind_default;

			case AllocatorTypes::MEMKIND_HBW:
				return &memkind_hbw;

			case AllocatorTypes::MEMKIND_INTERLEAVE:
				return &memkind_interleave;

			default:
				assert(!"'type' out of range!");
		}
	}

	memory_operation initialize_allocator(unsigned type)
	{
		size_t initial_size = 512;
		Allocator* allocator = get_existing(type);
		memory_operation data = allocator->wrapped_malloc(initial_size);
		memset(data.ptr, 0, initial_size);
		allocator->wrapped_free(data.ptr);

		return data;
	}

	VectorIterator<Allocator*> generate_random_allocator_calls(int num, int seed)
	{
		srand(seed);
		std::vector<Allocator*> allocators_calls;

		for (int i=0; i<num; i++)
		{
			int index = (rand() % AllocatorTypes::NUM_OF_ALLOCATOR_TYPES);

			allocators_calls.push_back(get_existing(index));
		}

		return VectorIterator<Allocator*>::create(allocators_calls);
	}

	VectorIterator<Allocator*> generate_const_allocator_calls(unsigned allocator_type, int num, int seed)
	{
		srand(seed);
		std::vector<Allocator*> allocators_calls;

		for (int i=0; i<num; i++)
		{
			allocators_calls.push_back(get_existing(allocator_type));
		}

		return VectorIterator<Allocator*>::create(allocators_calls);
	}

	//Return kind to the corresponding AllocatorTypes enum specified in argument.
	static memkind_t get_kind_by_type(unsigned type)
	{
		assert(AllocatorTypes::is_valid_memkind(type));

		switch(type)
		{
		case AllocatorTypes::MEMKIND_DEFAULT:
			return MEMKIND_DEFAULT;
		case AllocatorTypes::MEMKIND_HBW:
			return MEMKIND_HBW;
		case AllocatorTypes::MEMKIND_INTERLEAVE:
			return MEMKIND_INTERLEAVE;
		}
	}

private:
	StandardAllocatorWithTimer standard_allocator;
	JemallocAllocatorWithTimer jemalloc;
	MemkindAllocatorWithTimer memkind_default;
	MemkindAllocatorWithTimer memkind_hbw;
	MemkindAllocatorWithTimer memkind_interleave;
};