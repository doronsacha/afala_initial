#include <iostream>
#include <unistd.h>

void* smalloc(size_t size);

static bool sizeValid(size_t size)
{
    if(size> 100000000 || size==0)
    {
        return false;
    }
    return true;
}

void* smalloc(size_t size)
{
    if(sizeValid(size) == false)
    {
        return NULL;
    }
    void* new_ptr = sbrk((intptr_t)size);
    if(new_ptr==((void*)-1))
    {
        return NULL;
    }
    return new_ptr;
}
