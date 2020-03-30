// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);

    printf("\nAttempting to open file %s\n", filename);
	
    AddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
	//Changes by Alec Hebert
    if(extraInput)
        printf("\nExtra Input enabled.\n");
    printf("Number of Physical Pages: %i\n",NumPhysPages);
    printf("Page Size: %i bytes.\n", PageSize);
    printf("Page replacement algorithm chosen: ");
    if(repChoice == 0)
        printf("Demand Paging.\n");
    else if(repChoice == -1)
        printf("Demand Paging by default.\n");
    else if(repChoice == 1)
        printf("First in, First out.\n");
    else
        printf("Random Replacement.\n");
    //End changes by Alec Hebert
	printf("Memory allocation method chosen: ");
	if(memChoice == 1)
		printf("First-fit.\n\n");
	else if (memChoice == 2)
		printf("Best-fit.\n\n");
	else
		printf("Worst-fit.\n\n");
	
    space = new AddrSpace(executable);    
    currentThread->space = space;
    currentThread->setFN(filename);

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    //space->swap = new Swap(1, 1);
    
    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
