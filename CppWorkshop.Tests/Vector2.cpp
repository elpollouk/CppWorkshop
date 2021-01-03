#include "pch.h"
#include "Vector2.h"

//-------------------------------------------------------------------------------------------//
// Statics
//-------------------------------------------------------------------------------------------//
int Vector2::InstanceCount = 0;
TrackingAllocator<> TrackedVector2::s_Allocator = TrackingAllocator<>();