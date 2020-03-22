

#include "system.h"
#include "syscall.h"
#include "swap.h"

Swap::Swap(int id, int pageNum){
    printf("doing file shit\n");
	// Create swap file
	sprintf(swapFileName, "%i.swap", currentThread->getID());
	//Here, we create a swapFileName as ID.swap using unique thread ID
	fileSystem->Create(swapFileName, size);

	//Then, we must open it up.
	swapFile = fileSystem->Open(swapFileName);

    MarkSwapped(pageNum, false);
    Swap::id = id;
}
Swap::~Swap(){
    delete swapFile;
    fileSystem->Remove(swapFileName);
}

