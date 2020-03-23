#include "syscall.h"

int main()
{
	Join(Exec("../test/sort"));
    Exit(0);
}