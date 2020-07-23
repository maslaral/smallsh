#ifndef PROCESS_INCLUDED
#define PROCESS_INCLUDED

typedef struct BackArr BackArr;
void initBackArr(BackArr *, int);
BackArr *newBackArr(int);
void addBackArr(BackArr *, int);
void _backArrSetCapacity(BackArr *, int);
void deleteBackArr(BackArr *);
void _freeBackArr(BackArr *);
void removeBackArr(BackArr *, int);

#endif
