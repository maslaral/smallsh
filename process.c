#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"

struct BackArr
{
    int *process;
    int size;
    int capacity;
};

/*************************************************
Function: initBackArr()
Description: initializes the array by allocating
memory for the data and the size and capacity
*************************************************/
void initBackArr(BackArr *v, int capacity)
{
    v->process = (int *)malloc(sizeof(int) * capacity);
    v->size = 0;
    v->capacity = capacity;
};

/*************************************************
Function: newBackArr()
Description: allocates memory for a new array given
the cap size
*************************************************/
BackArr *newBackArr(int cap)
{
    BackArr *v = (BackArr *)malloc(sizeof(BackArr));
    initBackArr(v, cap);
    return v;
};

/*************************************************
Function: _backArrSetCapacity()
Description: sets the capacity of the array, used
to resize the array by copying the values to a new
larger array and destroying the old one
*************************************************/
void _backArrSetCapacity(BackArr *v, int newCap)
{
    BackArr *temp = newBackArr(newCap);

    for (int i = 0; i < v->size; i++)
    {
        temp->process[i] = v->process[i];
    }

    free(v->process);
    v->process = temp->process;
    v->capacity = newCap;

    free(temp);
    temp = 0;
};

/*************************************************
Function: _freeBackArr()
Description: frees each process in the array
*************************************************/
void _freeBackArr(BackArr *v)
{
    if (v->process != 0)
    {
        free(v->process);
        v->process = 0;
    }
    v->size = 0;
    v->capacity = 0;
};

/*************************************************
Function: deleteBackArr()
Description: calls freeBackArr() to free the memory
of the array, and finally free the pointer to the
array
*************************************************/
void deleteBackArr(BackArr *v)
{
    _freeBackArr(v);
    free(v);
};

/*************************************************
Function: addBackArr()
Description: adds a new PID to the background process
array
*************************************************/
void addBackArr(BackArr *v, int pid)
{
    if (v->size == v->capacity)
    {
        int newCapacity = v->capacity * 2;
        _backArrSetCapacity(v, newCapacity);
    }

    v->process[v->size] = pid;
    v->size++;
};

/*************************************************
Function: removeBackArr()
Description: removes a pid from the array once
the process is complete
*************************************************/
void removeBackArr(BackArr *v, int pid)
{
    int i = 0;
    int found = 0;

    // looks through the array for a matching pid
    for (i = 0; i < v->size; i++)
    {
        if (v->process[i] == pid)
        {
            found = 1; // sets found flag to 1
        }
        break;
    }

    if (found == 1) // if found
    {
        // remove value by shifting over each element
        for (int j = i; j < v->size; j++)
        {
            v->process[j] = v->process[j + 1];
        }
        // reduce the size of the array
        v->size -= 1;
    }
}