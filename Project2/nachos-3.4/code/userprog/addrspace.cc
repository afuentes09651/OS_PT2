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


//AR
// valid - Set true if page is in physical memory.
void AddrSpace::setValidity(int vPage, bool valid){
	// begin code changes by joseph kokenge
	if (isTwoLevel) {
		int outerIndex = vPage/innerTableSize;

		int innerIndex = vPage%innerTableSize;	

		outerPageTable[outerIndex][innerIndex].valid = valid;
	// end code changes by joseph kokenge
	} else {
		pageTable[vPage].valid = valid;
	}
}
//dirty - Set if page is modified by machine.
void AddrSpace::setDirty(int vPage, bool dirty){
	// begin code changes by joseph kokenge
	if (isTwoLevel) {
		int outerIndex = vPage/innerTableSize;

		int innerIndex = vPage%innerTableSize;	

		outerPageTable[outerIndex][innerIndex].dirty = dirty;
		// end code changes by joseph kokenge
	} else {
		
		pageTable[vPage].dirty = dirty;
	}
}

int AddrSpace::getPageNum(int pPage){
	// begin code changes by joseph kokenge
	if (isTwoLevel) {
		for(int i = 0; i < outerTableSize; i++){
			if (!(outerPageTable[i] == NULL)){
				for (int j = 0; j < innerTableSize; j++){	
					if(outerPageTable[i][j].physicalPage == pPage && outerPageTable[i][j].valid)
						return outerPageTable[i][j].virtualPage;
				}
			}

				
			}
			return -1;		
	// end code changes by joseph kokenge		
	} else {
		
		for(int i = 0; i < numPages; i++){
			if(pageTable[i].physicalPage == pPage && pageTable[i].valid){
					return pageTable[i].virtualPage;
			}
		}
		return -1;

	}

}




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

AddrSpace::AddrSpace(OpenFile *executable, int threadid)
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

	size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize; // we need to increase the size
	numPages = divRoundUp(size, PageSize);
	size = numPages * PageSize;

	// Create swap file
	sprintf(swapFileName, "%i.swap", threadid);
	//Here, we create a swapFileName as ID.swap using unique thread ID
	fileSystem->Create(swapFileName, size);
	printf("\nSWAPFILE CREATION: Swapfile %s has been created.\n",swapFileName);
	// //Then, we must open it up.
	swapFile = fileSystem->Open(swapFileName);

	int exeSize = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
	//This int represents the size of the buffer.
	char *exeBuff = new char[exeSize];
	executable->ReadAt(exeBuff, exeSize, sizeof(noffH));
	swapFile->WriteAt(exeBuff, exeSize, 0);

	// Close to not consume mem
	delete [] exeBuff; //  code change by joseph kokenge
	delete swapFile;

	//If we get past the if statement, then there was sufficient space
	space = true;

	//This safeguards against the loop terminating due to reaching
	//the end of the bitmap but no contiguous space being available

	//begin code by joseph kokenge
	if (isTwoLevel) {
		printf("Initializing Two level Table\n");


	    outerPageTable = new TranslationEntry*[outerTableSize];

		for (i = 0; i < outerTableSize; i++) {
			outerPageTable[i] = NULL;


	    }


		//end code by joseph kokenge
	} else {
		printf("Initializing normal Table \n");
	    pageTable = new TranslationEntry[numPages]; // first, set up the translation
	    for (i = 0; i < numPages; i++) {
			pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
			pageTable[i].physicalPage = -1;
			pageTable[i].valid = FALSE; //edit AF, setting valid bits to false per request of instructions
			pageTable[i].use = FALSE;
			pageTable[i].dirty = FALSE;
			pageTable[i].readOnly = FALSE;  // if the code segment was entirely on

	    }
		//print memMap of singleLevel

		printf("PRINTING MEM MAP\n");
		memMap->Print();	// Useful!
	}


}






void AddrSpace::HandlePageFault(int addr){
	int vPage = addr / PageSize;
	//begin Code changes Joseph Kokenge

	if (isTwoLevel) {
		//printf("address: %d",addr);

		int outerIndex = vPage/innerTableSize;

		int innerIndex = vPage%innerTableSize;

		//printf("OUTER INDEX: %d\n",outerIndex);
		//printf("inner INDEX: %d\n",innerIndex);

		if (outerPageTable[outerIndex] == NULL) {
			outerPageTable[outerIndex] = new TranslationEntry[innerTableSize];

		    for (int i = 0; i < innerTableSize; i++) {
				outerPageTable[outerIndex][i].virtualPage = innerTableSize*outerIndex+i;	// for now, virtual page # = phys page #
				outerPageTable[outerIndex][i].valid = FALSE; //edit AF, setting valid bits to false per request of instructions
				outerPageTable[outerIndex][i].use = FALSE;
				outerPageTable[outerIndex][i].dirty = FALSE;
				outerPageTable[outerIndex][i].readOnly = FALSE;  // if the code segment was entirely on
		    }
			printf("made a new page table\n");

		}

		//end Code changes Joseph Kokenge

	}
	//Begin changes Alec Hebert and Armando Fuentes
		int pPage = memMap -> Find();
		printf("PAGE FAULT #%i\n",faultcount);
		if (extraInput)
			printf("\nPAGE FAULT: Process %i requests virtual page %i.\n", currentThread -> getID(), vPage);

		// Something needs to be swapped out
		// Something needs to be swapped out
		if (pPage == -1)
		{
			// Pick a page to swap out
			if (repChoice == 1)
			{ // FIFO
				pPage = (int)fifo.Remove();
			}
			else if (repChoice == 2)
			{ // RANDOM
				pPage = (int)(Random() % NumPhysPages);

			}
			else
			{ // Demand
				printf("You chose Demand Paging, not enough pages are available. Due to your choice, nothing will be swapped and this process will terminate.\n");
				Cleanup();
			}
			// Do the roar
			printf("Swapping out thread %d page %d\n",ipt[pPage]->getID(),pPage);
			ipt[pPage]->space->SwapOut(pPage);
		}

		// Swap in
		LoadPage(vPage, pPage);
		memMap->Print();
		// Update queue if we fifo
		if (repChoice == 1)
		{
			fifo.Append((void *)pPage);
		}
		//End changes Alec Hebert and Armando Fuentes

}



void AddrSpace::LoadPage(int vPage, int pPage)
{
	printf("pPage: %i, vPage: %i\n", pPage, vPage);
	//begin Code changes Joseph Kokenge

	if (isTwoLevel) {
		if (extraInput)
			printf("Swapping in Physical Page %d and Virtual Page %d\n", pPage, vPage); //guessing we are going to need this output
		int outerIndex = vPage/innerTableSize;

		int innerIndex = vPage%innerTableSize;	

		outerPageTable[outerIndex][innerIndex].physicalPage = pPage;
	//end Code changes Joseph Kokenge

	} else {
		if (extraInput)
			printf("Swapping in Physical Page %d and Virtual Page %d\n", pPage, vPage); //guessing we are going to need this output
		pageTable[vPage].physicalPage = pPage;
	}
	
	ipt[pPage] = currentThread; //AH - put currentThread into ipt slot corresponding to physical page number.

	setValidity(vPage, true);
	setDirty(vPage, false);
	
	printf("Attempting to open swapfile %s...\n",swapFileName);
	swapFile = fileSystem->Open(swapFileName);
	// DONT INCLUDE NOFF SIZE HERE SINCE WE SKIPPED IT WHEN WRITING TO THE SWAPFILE
	//***
	
	swapFile->ReadAt(&(machine->mainMemory[pPage * PageSize]), PageSize, (vPage * PageSize)); //the meat of loadPage

	delete swapFile;
	printf("Closing swapfile %s...\n",swapFileName);



}

bool AddrSpace::SwapOut(int pPage)
{
	printf("swap\n");
	
	int vPage = getPageNum(pPage); //Does the page exist?

	if (vPage == -1)
	{
		printf("ERROR: Could not swap page!\n");
		return false;
	}

	
	//begin Code changes Joseph Kokenge

	if (isTwoLevel) {


		int outerIndex = vPage/innerTableSize;

		int innerIndex = vPage%innerTableSize;	

		if (outerPageTable[outerIndex][innerIndex].dirty)
		{
			if (extraInput)
			{
				printf("Swap out physical page %i from process %i.\n", vPage, ipt[vPage]->getID());
			}
			char *pos = machine->mainMemory + pPage * PageSize;

			swapFile = fileSystem->Open(swapFileName);
			swapFile->WriteAt(pos, PageSize, vPage * PageSize);
			delete swapFile;
		}

		setValidity(vPage, false);
		setDirty(vPage, false);
		outerPageTable[outerIndex][innerIndex].physicalPage = -1;
		
		if (extraInput)
			printf("Virtual page %i removed.\n", vPage);

		return true;
	
	//end Code changes Joseph Kokenge

	} else {

		if (pageTable[vPage].dirty)
		{
			if (extraInput)
			{
				printf("Swap out physical page %i from process %i.\n", vPage, ipt[vPage]->getID());
			}
			char *pos = machine->mainMemory + pPage * PageSize;

			swapFile = fileSystem->Open(swapFileName);
			swapFile->WriteAt(pos, PageSize, vPage * PageSize);
			delete swapFile;
		}

		setValidity(vPage, false);
		setDirty(vPage, false);
		pageTable[vPage].physicalPage = -1;
		if (extraInput)
			printf("Virtual page %i removed.\n", vPage);

		return true;
	}

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
	if(space) {
		//begin Code changes Joseph Kokenge

		if (isTwoLevel) {
			//delete here
			//FROM SHAH it involves deleting your pointers of pagetable and clearing the bitMap used by the process.
			for(int i = 0; i < outerTableSize; i++){
				if (!(outerPageTable[i] == NULL)){
					for (int j = 0; j < innerTableSize; j++){	
						if(outerPageTable[i][j].valid){
							memMap->Clear(outerPageTable[i][j].physicalPage);
							ipt[outerPageTable[i][j].physicalPage] = NULL;
						}
					}
					delete [] outerPageTable[i];
				}
			}
			delete [] outerPageTable;
			//end Code changes Joseph Kokenge

		}
		else {
			for(int i = 0; i < numPages; i++)	// We need an offset of startPage + numPages for clearing.
				if(pageTable[i].valid){
					memMap->Clear(pageTable[i].physicalPage);
					ipt[pageTable[i].physicalPage] = NULL;
				}
				delete [] pageTable;





		}
		//		if (!fileSystem->Remove(swapFileName))
		//			printf("failed to delete swap file\n");
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

void AddrSpace::InitRegisters()
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
{
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{

	//begin Code changes Joseph Kokenge

	if (isTwoLevel) {
		machine->outerPageTable = outerPageTable;
		machine->twoLevelPageTableSize = totalSize;
	//end Code changes Joseph Kokenge

	} else {
		machine->pageTable = pageTable;
		machine->pageTableSize = numPages;
	}

}
