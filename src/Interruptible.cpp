#include "pch.h"

#include "Interruptible.h"

Interruptible::Interruptible()
{
    Parent = nullptr;
    InterruptRequested = false;
}

Interruptible::Interruptible(Interruptible* parent)
{
    Parent = parent;
    InterruptRequested = false;
}

void Interruptible::RequestInterrupt()
{
    InterruptRequested = true;
}

bool Interruptible::IsInterruptRequested()
{
    return InterruptRequested;
}

void Interruptible::CheckInterruption()
{
    if (IsInterruptRequested() || (Parent && Parent->IsInterruptRequested()))
        throw InterruptedException();
}