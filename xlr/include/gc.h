#ifndef GC_H
#define GC_H
// ****************************************************************************
//  gc.h                                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//     Garbage collector managing memory for us
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "base.h"
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <stdint.h>
#include <typeinfo>

extern void debuggc(void *);

XL_BEGIN

// ============================================================================
//
//   Class declarations
//
// ============================================================================

struct GarbageCollector;

struct TypeAllocator
// ----------------------------------------------------------------------------
//   Structure allocating data for a single data type
// ----------------------------------------------------------------------------
{
    struct Chunk
    {
        union
        {
            Chunk *         next;           // Next in free list
            TypeAllocator * allocator;      // Allocator for this object
            uintptr_t       bits;           // Allocation bits
        };
        uint                count;          // Ref count
    };

public:
    TypeAllocator(kstring name, uint objectSize);
    virtual ~TypeAllocator();

    void *              Allocate();
    void                Delete(void *);
    virtual void        Finalize(void *);

    void                Sweep();

    static TypeAllocator *ValidPointer(TypeAllocator *ptr);
    static TypeAllocator *AllocatorPointer(TypeAllocator *ptr);

    static void         Acquire(void *ptr);
    static void         Release(void *ptr);
    static bool         IsGarbageCollected(void *ptr);
    static bool         IsAllocated(void *ptr);
    static void         InUse(void *ptr);
    void                DeleteLater(Chunk *);
    bool                DeleteAll();

    void *operator new(size_t size);
    void operator delete(void *ptr);

public:
    enum ChunkBits
    {
        PTR_MASK        = 15,           // Special bits we take out of the ptr
        CHUNKALIGN_MASK = 7,            // Alignment for chunks
        ALLOCATED       = 0,            // Just allocated
        IN_USE          = 1             // Set if already marked this time
    };

public:
    struct Listener
    {
        virtual void BeginCollection()          {}
        virtual bool CanDelete(void *)          { return true; }
        virtual void EndCollection()            {}
    };
    void AddListener(Listener *l) { listeners.insert(l); }
    bool CanDelete(void *object);

protected:
    GarbageCollector *  gc;
    kstring             name;
    std::vector<Chunk*> chunks;
    std::set<Listener *>listeners;
    std::map<void*,uint>roots;
    Chunk *             freeList;
    Chunk *             toDelete;
#ifdef XLR_GC_LIFO
    // Make free list LIFO instead of FIFO. Not good performance-wise,
    // but may help valgrind detect re-use of freed pointers.
    Chunk *             freeListTail;
#endif
    uint                chunkSize;
    uint                objectSize;
    uint                alignedSize;
    uint                available;
    uint                allocatedCount;
    uint                freedCount;
    uint                totalCount;

    friend void ::debuggc(void *ptr);
    friend struct GarbageCollector;

public:
    static void *       lowestAddress;
    static void *       highestAddress;
    static void *       lowestAllocatorAddress;
    static void *       highestAllocatorAddress;
    static uint         finalizing;
} __attribute__((aligned(16)));


template <class Object, typename ValueType=void> struct GCPtr;


template <class Object>
struct Allocator : TypeAllocator
// ----------------------------------------------------------------------------
//    Allocate objects for a given object type
// ----------------------------------------------------------------------------
{
    typedef Object  object_t;
    typedef Object *ptr_t;

public:
    Allocator();

    static Allocator *  Singleton();
    static Object *     Allocate(size_t size);
    static void         Delete(Object *);
    virtual void        Finalize(void *object);
    void                RegisterPointers();
    static bool         IsAllocated(void *ptr);

private:
    static Allocator *  allocator;
};


template<class Object, typename ValueType>
struct GCPtr
// ----------------------------------------------------------------------------
//   A root pointer to an object in a garbage-collected pool
// ----------------------------------------------------------------------------
{
    GCPtr(): pointer(0)                         { }
    GCPtr(Object *ptr): pointer(ptr)            { Acquire(); }
    GCPtr(Object &ptr): pointer(&ptr)           { Acquire(); }
    GCPtr(const GCPtr &ptr)
        : pointer(ptr.Pointer())                { Acquire(); }
    template<class U, typename V>
    GCPtr(const GCPtr<U,V> &p)
        : pointer((U*) p.Pointer())             { Acquire(); }
    ~GCPtr()                                    { Release(); }

    operator Object* () const                   { InUse(); return pointer; }
    const Object *ConstPointer() const          { return pointer; }
    Object *Pointer() const                     { return pointer; }
    Object *operator->() const                  { return pointer; }
    Object& operator*() const                   { return *pointer; }
    operator ValueType() const                  { return pointer; }

    GCPtr &operator= (const GCPtr &o)
    {
        if (o.ConstPointer() != pointer)
        {
            Release();
            pointer = (Object *) o.ConstPointer();
            Acquire();
        }
        return *this;
    }

    template<class U, typename V>
    GCPtr& operator=(const GCPtr<U,V> &o)
    {
        if (o.ConstPointer() != pointer)
        {
            Release();
            pointer = (U *) o.ConstPointer();
            Acquire();
        }
        return *this;
    }

#define DEFINE_CMP(CMP)                                 \
    template<class U, typename V>                       \
    bool operator CMP(const GCPtr<U,V> &o) const        \
    {                                                   \
        return pointer CMP o.ConstPointer();            \
    }                                           

    DEFINE_CMP(==)
    DEFINE_CMP(!=)
    DEFINE_CMP(<)
    DEFINE_CMP(<=)
    DEFINE_CMP(>)
    DEFINE_CMP(>=)

    void        InUse() const   { TypeAllocator::InUse(pointer); }

protected:
    void        Acquire() const { TypeAllocator::Acquire(pointer); }
    void        Release() const { TypeAllocator::Release(pointer); }

    Object *    pointer;
};


struct GarbageCollector
// ----------------------------------------------------------------------------
//    Structure registering all allocators
// ----------------------------------------------------------------------------
{
    GarbageCollector();
    ~GarbageCollector();

    void        Register(TypeAllocator *a);
    void        RunCollection(bool force=false);
    void        MustRun()    { mustRun = true; }
    void        Statistics(uint &tot, uint &alloc, uint &freed);

    static GarbageCollector *   Singleton();
    static void                 Delete();
    static void                 Collect(bool force=false);
    static void                 CollectionNeeded() { Singleton()->MustRun(); }
    static bool                 Running() { return Singleton()->running; }

protected:
    std::vector<TypeAllocator *> allocators;
    bool mustRun, running;
    static GarbageCollector *    gc;

    friend void ::debuggc(void *ptr);
};



// ============================================================================
//
//    Macros used to declare a garbage-collected type
//
// ============================================================================

// Define a garbage collected tree

#define GARBAGE_COLLECT(type)                                           \
    void *operator new(size_t size)                                     \
    {                                                                   \
        return XL::Allocator<type>::Allocate(size);                     \
    }                                                                   \
                                                                        \
    void operator delete(void *ptr)                                     \
    {                                                                   \
        XL::Allocator<type>::Delete((type *) ptr);                      \
    }



// ============================================================================
//
//   Inline functions for base allocator
//
// ============================================================================

inline TypeAllocator *TypeAllocator::ValidPointer(TypeAllocator *ptr)
// ----------------------------------------------------------------------------
//   Return a valid pointer from a possibly marked pointer
// ----------------------------------------------------------------------------
{
    TypeAllocator *result = (TypeAllocator *) (((uintptr_t) ptr) & ~PTR_MASK);
    assert(result && result->gc == GarbageCollector::Singleton());
    return result;
}


inline TypeAllocator *TypeAllocator::AllocatorPointer(TypeAllocator *ptr)
// ----------------------------------------------------------------------------
//   Return a valid pointer from a possibly marked pointer
// ----------------------------------------------------------------------------
{
    TypeAllocator *result = (TypeAllocator *) (((uintptr_t) ptr) & ~PTR_MASK);
    return result;
}


inline bool TypeAllocator::IsGarbageCollected(void *ptr)
// ----------------------------------------------------------------------------
//   Tell if a pointer is managed by the garbage collector
// ----------------------------------------------------------------------------
{
    return ptr >= lowestAddress && ptr <= highestAddress;
}


inline bool TypeAllocator::IsAllocated(void *ptr)
// ----------------------------------------------------------------------------
//   Tell if a pointer is allocated by the garbage collector (not free)
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(ptr))
    {
        if ((uintptr_t) ptr & CHUNKALIGN_MASK)
            return false;

        Chunk *chunk = (Chunk *) ptr - 1;
        TypeAllocator *alloc = AllocatorPointer(chunk->allocator);
        if (alloc >= lowestAllocatorAddress && alloc <= highestAllocatorAddress)
            if (alloc->gc == GarbageCollector::Singleton())
                return true;
    }
    return false;
}


inline void TypeAllocator::Acquire(void *pointer)
// ----------------------------------------------------------------------------
//   Increase reference count for pointer and return it
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(pointer))
    {
        assert (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        assert (IsAllocated(pointer));

        Chunk *chunk = ((Chunk *) pointer) - 1;
        assert (chunk->count || !GarbageCollector::Running());
        ++chunk->count;
    }
}


inline void TypeAllocator::Release(void *pointer)
// ----------------------------------------------------------------------------
//   Decrease reference count for pointer and return it
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(pointer))
    {
        assert (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        assert (IsAllocated(pointer));

        Chunk *chunk = ((Chunk *) pointer) - 1;
        TypeAllocator *allocator = ValidPointer(chunk->allocator);
        assert(chunk->count);
        uint count = --chunk->count;
        if (!count)
        {
            if (finalizing)
                allocator->DeleteLater(chunk);
            else if (~chunk->bits & IN_USE)
                allocator->Finalize(pointer);
        }
    }
}


inline void TypeAllocator::InUse(void *pointer)
// ----------------------------------------------------------------------------
//   Mark the current pointer as in use, to preserve in next GC cycle
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(pointer))
    {
        assert (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        Chunk *chunk = ((Chunk *) pointer) - 1;
        chunk->bits |= IN_USE;
    }
}


inline void TypeAllocator::DeleteLater(TypeAllocator::Chunk *ptr)
// ----------------------------------------------------------------------------
//   Record that we will need to delete this
// ----------------------------------------------------------------------------
{
    ptr->next = toDelete;
    toDelete = ptr;
}


inline bool TypeAllocator::DeleteAll()
// ----------------------------------------------------------------------------
//    Remove all the things that we have pushed on the toDelete list
// ----------------------------------------------------------------------------
{
    bool result = false;
    while (toDelete)
    {
        Chunk *next = toDelete;
        toDelete = toDelete->next;
        next->allocator = this;
        Finalize(next+1);
        freedCount++;
        result = true;
    }
    return result;
}



// ============================================================================
//
//   Definitions for template Allocator
//
// ============================================================================

template<class Object> inline
Allocator<Object>::Allocator()
// ----------------------------------------------------------------------------
//   Create an allocator for the given size
// ----------------------------------------------------------------------------
    : TypeAllocator(typeid(Object).name(), sizeof (Object))
{}


template<class Object> inline
Allocator<Object> * Allocator<Object>::Singleton()
// ----------------------------------------------------------------------------
//    Return a singleton for the allocation class
// ----------------------------------------------------------------------------
{
    if (!allocator)
        // Create the singleton
        allocator = new Allocator;

    return allocator;
}


template<class Object> inline
Object *Allocator<Object>::Allocate(size_t size)
// ----------------------------------------------------------------------------
//   Allocate an object (invoked by operator new)
// ----------------------------------------------------------------------------
{
    assert(size == Singleton()->TypeAllocator::objectSize); (void) size;
    return (Object *) Singleton()->TypeAllocator::Allocate();
}


template<class Object> inline
void Allocator<Object>::Delete(Object *obj)
// ----------------------------------------------------------------------------
//   Allocate an object (invoked by operator delete)
// ----------------------------------------------------------------------------
{
    Singleton()->TypeAllocator::Delete(obj);
}


template<class Object> inline
void Allocator<Object>::Finalize(void *obj)
// ----------------------------------------------------------------------------
//   Make sure that we properly call the destructor for the object
// ----------------------------------------------------------------------------
{
    if (CanDelete(obj))
    {
        Object *object = (Object *) obj;
        delete object;
    }
    else
    {
        Chunk *chunk = ((Chunk *) obj) - 1;
        chunk->bits |= IN_USE;
    }
}


template <class Object> inline
bool Allocator<Object>::IsAllocated(void *ptr)
// ----------------------------------------------------------------------------
//   Tell if a pointer is allocated in this particular pool
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(ptr))
    {
        if ((uintptr_t) ptr & CHUNKALIGN_MASK)
            return false;

        Chunk *chunk = (Chunk *) ptr - 1;
        TypeAllocator *alloc = AllocatorPointer(chunk->allocator);
        if (alloc == Singleton())
            return true;
    }
    return false;
}

XL_END

#endif // GC_H
