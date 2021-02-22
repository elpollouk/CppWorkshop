#pragma once

#include "Allocators/TrackingAllocator.h"

class Vector2
{
public:
    static int InstanceCount;

    Vector2(int x, int y) :
        _x(x),
        _y(y)
    {
        // This is a simple way for us to check the number of instances in tests, but it's
        // not thread safe, so don't do it in production!
        InstanceCount++;
    }

    // Default constructor.
    Vector2() : Vector2(0, 1) {}

    virtual ~Vector2()
    {
        InstanceCount--;
    }

    int getX() const { return _x; }
    int getY() const { return _y; }

    void RotateLeft() {
        int t = _x;
        _x = -_y;
        _y = t;
    }

    void RotateRight() {
        int t = -_x;
        _x = _y;
        _y = t;
    }

private:
    int _x;
    int _y;
};

// A version of Vector2 that uses a custom allocator
class TrackedVector2 : public Vector2
{
public:
    // Custom allocator we're using just for this class
    static TrackingAllocator<> s_Allocator;

    TrackedVector2() : Vector2() {}
    TrackedVector2(int x, int y) : Vector2(x, y) {}

    // Overloads for new/delete that route through to our custom allocator
    void* operator new(size_t size) { return s_Allocator.allocate(size); }
    void operator delete(void* pMem) { s_Allocator.deallocate((uint8_t*)pMem); }
    void* operator new[](size_t size) { return s_Allocator.allocate(size); }
    void operator delete[](void* pMem) { s_Allocator.deallocate((uint8_t*)pMem); }
};


//---------------------------------------------------------------------------------------//
// Example derived class to demo up casting
// Please don't implement a Vector3 using inheritance.
//---------------------------------------------------------------------------------------//
class Vector3 : public Vector2
{
public:
    Vector3(int x, int y, int z) :
        Vector2(x, y),
        _z(z)
    {

    }

    Vector3() :
        _z(0)
    {

    }

    int getZ() const { return _z; }

private:
    int _z;
};
