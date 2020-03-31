// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "swap.h"

#define UserStackSize		1024 	// increase this as necessary!


class AddrSpace {
  public:
    

    
    AddrSpace(OpenFile *executable, int threadid);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

    void setValidity(int vPage, bool valid);
    void setDirty(int vPage, bool dirty);

    void LoadPage(int vPage, int pPage);
    void HandlePageFault(int addr);
    bool SwapOut(int pPage);
    bool SwapIn(int vPage, int pPage); 

    int getPageNum(int pPage);

    static const int outerTableSize = 16;
    static const int innerTableSize = 16;
    static const int totalSize = outerTableSize * innerTableSize;


    //Swap *swap;
    unsigned int numPages;		// Number of pages in the virtual 
    //end AR
  private:
    
    OpenFile *exeFile;
    OpenFile *swapFile;
    
    
    TranslationEntry **outerPageTable;
    
    

    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!

					// address space
	unsigned int startPage;		//Page number that the program starts at
  							//in physical memory
	bool space;		//Boolean to remember if there was enough space or not
  char swapFileName[12];
};

#endif // ADDRSPACE_H
