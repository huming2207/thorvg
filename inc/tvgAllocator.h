/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TVG_ALLOCATOR_H_
#define _TVG_ALLOCATOR_H_

#ifndef _THORVG_H_
#error "Do not include tvgAllocator.h directly, include thorvg.h instead."
#endif

#include <cstdlib>
#include <cstddef>
#include <new>

namespace tvg
{

/**
 * @brief Heap allocator callbacks used by ThorVG for its dynamic memory.
 *
 * The defaults point to std::malloc, std::calloc, std::realloc and std::free.
 * Pass a customized instance to Initializer::init() to redirect ThorVG's heap,
 * e.g. to an external RAM on embedded systems.
 *
 * @note All four callbacks must be set and must operate on the same heap.
 *
 * @see Initializer::init()
 */
struct HeapAllocator
{
    void* (*alloc)(size_t) = std::malloc;
    void* (*calloc)(size_t, size_t) = std::calloc;
    void* (*realloc)(void*, size_t) = std::realloc;
    void (*free)(void*) = std::free;
};

/**
 * @brief Class-scoped allocation operators of the ThorVG objects.
 *
 * ThorVG objects are allocated from the HeapAllocator given to Initializer::init(),
 * while the global new/delete operators of the host application stay untouched,
 * including in static builds. The operators are defined in the library, so an object
 * is released to the same heap regardless of which binary deletes it.
 */
struct TVG_API Allocator
{
    static void* operator new(std::size_t size);
    static void* operator new[](std::size_t size);
    static void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
    static void* operator new[](std::size_t size, const std::nothrow_t&) noexcept;
    static void operator delete(void* ptr) noexcept;
    static void operator delete[](void* ptr) noexcept;
    static void operator delete(void* ptr, const std::nothrow_t&) noexcept;
    static void operator delete[](void* ptr, const std::nothrow_t&) noexcept;

    // placement construction in existing storage allocates nothing
    static void* operator new(std::size_t, void* ptr) noexcept { return ptr; }
    static void* operator new[](std::size_t, void* ptr) noexcept { return ptr; }
    static void operator delete(void*, void*) noexcept {}
    static void operator delete[](void*, void*) noexcept {}
};
}

#endif //_TVG_ALLOCATOR_H_
