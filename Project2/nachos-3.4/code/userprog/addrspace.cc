// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
	exeFile = executable;
    NoffHeader noffH;
    unsigned int i, size, pAddr, counter;
	space = false;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;	// we need to increase the size
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

	// Create swap file
	sprintf(swapFileName, "%i.swap", currentThread->getID());
	//Here, we create a swapFileName as ID.swap using unique thread ID
	fileSystem->Create(swapFileName, size);

	// //Then, we must open it up.
	swapFile = fileSystem->Open(swapFileName);

	int exeSize = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
	//This int represents the size of the buffer.
	char *exeBuff = new char[exeSize];
	executable->ReadAt(exeBuff, exeSize, sizeof(noffH));
	swapFile->WriteAt(exeBuff, exeSize, 0);

	// Close to not consume mem
	delete exeBuff;
	delete swapFile;


	//If we get past the if statement, then there was sufficient space
	space = true;

	//This safeguards against the loop terminating due to reaching
	//the end of the bitmap but no contiguous space being available


// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		//pageTable[i].physicalPage = i;
		pageTable[i].valid = FALSE; //edit AF, setting valid bits to false per request of instructions
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
    }
	
	memMap->Print();	// Useful!
    
}

void AddrSpace::HandlePageFault(int addr){
	int vPage = addr / PageSize;
	if(extraInput)
		printf("\nPAGE FAULT: Process %i requests virtual page %i.\n",currentThread->getID(),vPage);

	TranslationEntry entry = pageTable[vPage];

	if(!entry.valid){
		
		LoadPage(vPage);
	}
	//Start changes Alec Hebert
	else{
		// Something needs to be swapped out according to selected algo
		// Pick page to swap out
		// store using IPT (the way to do this is the same for all options)
		// call SwapOut
		if(repChoice==1){
			//FIFO
			//Look at the List, find first page
			//find PHYSICAL PAGE NUMBER used by any thread to be replaced
			//get that page number to swap
			void * swapPagePtr = fifo.Remove(); 
			int swapPage = *(int *)swapPagePtr;
			//find thread in ipt using that page
			Thread * t = ipt[swapPage];
			//is it dirty?
			if(pageTable[swapPage].dirty){
				//if so, open swapfile
				//write from physical page to swap at vpage location
				sprintf(swapFileName, "%i.swap", t->getID());
				SwapOut(swapPage);//write content of this thread's physical page to thread's vpage location
				ipt[swapPage] = t;//idfk how to do this
				//close swap
				//delete pointer
				//set valid bit to false
			}
			//open current thread's swapfile
			sprintf(swapFileName, "%i.swap", currentThread->getID());
			//copy content into reserved phys page
			LoadPage(swapPage);//???
			//update ipt[ppn] to current thread
			ipt[swapPage] = currentThread;
			//if fifo, add to list
			void * swapPtr = &swapPage;
			fifo.Append(swapPtr);//idek if i appended the right shit honestly
		}
		else if (repChoice==2){
			//Random
			//find physical page number generated by randint
			int randint = (int)(Random() % NumPhysPages);
			TranslationEntry randPage = pageTable[randint];
			//get that boi to swap
			//find thread in ipt using that page
			Thread * t = ipt[randPage.physicalPage];
			//is it dirty?
			if(randPage.dirty){
				//if so, open swapfile
				//write from physical page to swap at vpage location
				sprintf(swapFileName, "%i.swap", t->getID());
				SwapOut(randPage.physicalPage);
				ipt[randPage.physicalPage] = t;
				//close swap
				//delete pointer
				//set valid bit to false
			}
			//open current thread's swapfile
			sprintf(swapFileName, "%i.swap", currentThread->getID());
			//copy content into reserved phys page
			LoadPage(randPage.physicalPage);//???
			//update ipt[ppn] to current thread
			ipt[randint] = currentThread;
		}
		else{
			//Demand
			printf("You chose Demand Paging, not enough pages are available. Due to your choice, nothing will be swapped and this process will terminate.\n");
		}
		//End changes Alec Hebert
	}
}
void AddrSpace::LoadPage(int vPage){

	int pPage = memMap->Find();
	printf("pPage: %i, vPage: %i\n", pPage, vPage);
	if(pPage < 0){
		printf("ERROR: No free page!\n");

		for(int i = 0; i < NumPhysPages; i++){
			if(pageTable[i].valid)
				printf("ayooo %i", i);
		}
		Cleanup();
		return;
		// No free page, so we need to pick a page, and swap it out, then swap ours in.
		//SwapOut(); // swap out should really only be called here when there are no pages available
	}

	if(extraInput)
		printf("Assigning physical page %i\n",pPage); //guessing we are going to need this output
	pageTable[vPage].physicalPage = pPage;
	ipt[pPage] = currentThread;//AH - put currentThread into ipt slot corresponding to physical page number.

	printf("Swapping in page\n");
	//SwapIn(vPage, pPage);
	setValidity(vPage, true);
	setDirty(vPage, false);
	
	swapFile = fileSystem->Open(swapFileName);
	// DONT INCLUDE NOFF SIZE HERE SINCE WE SKIPPED IT WHEN WRITING TO THE SWAPFILE
	swapFile->ReadAt(&(machine->mainMemory[pPage * PageSize]), PageSize,  (vPage * PageSize));
	if(repChoice==1){
		void * pPagePtr = &pPage;
		fifo.Append(pPagePtr);//AH - put page in fifo list after its in memory
	}
	delete swapFile;
}


bool AddrSpace::SwapOut(int pPage){

	int page = getPageNum(pPage);//Does the page exist?

	if(page == -1){
		printf("ERROR: Could not swap page!\n");
		return false;
	}

	if(pageTable[page].dirty){
		if(extraInput){
			printf("Swap out physical page %i from process %i.\n",page,ipt[page]->getID());
		}
		int charsWrote;
		char *pos = machine->mainMemory + pPage * PageSize;

		swapFile = fileSystem->Open(swapFileName);
		swapFile->WriteAt(pos, PageSize, page * PageSize);
		delete swapFile;

		if(charsWrote != PageSize){
			//some shit fucked up
			printf("some shit fucked up\n");
			return false;
		}
	} 

	setValidity(page, false);
	setDirty(page, false);
	pageTable[page].physicalPage = -1;
	if(extraInput)
		printf("Virtual page %i removed.",page);

	return true;
}
//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

//Because the initialization already zeroes out the memory to be used,
//is it even necessary to clear out any garbage data during deallocation?

AddrSpace::~AddrSpace()
{
	
	// Only clear the memory if it was set to begin with
	// which in turn only happens after space is set to true
	if(space)
	{
		for(int i = 0; i < numPages; i++)	// We need an offset of startPage + numPages for clearing.
			if(pageTable[i].valid){
				memMap->Clear(pageTable[i].physicalPage);
			}

		delete [] pageTable;

		if(!fileSystem->Remove(swapFileName))
			printf("failed to delete swap file\n");
		
		memMap->Print();
	}
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
