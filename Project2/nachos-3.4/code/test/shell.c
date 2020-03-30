#include "syscall.h"

int main(){
    SpaceId newProc;
    char *prompt = "student@NachOS > ", buffer[60];
    while( 1 ){
	    Write(prompt, 32, ConsoleOutput);   // Write our prompt string to console 
        Read(buffer, 60, ConsoleInput);     // Get user input
        // quit the shell if user enters "exit"
        if(buffer[0]=='e'&&buffer[1]=='x'&&buffer[2]=='i'&&buffer[3]=='t'&&buffer[4]=='\0'){
            Exit(0);
        }
        newProc = Exec(buffer);             // Exec examples: "../test/shell" "../test/sort" etc...
        if(newProc >= 0){                   // This assumes that SpaceId < 0 represents an error 
            Join(newProc);
        }
    }
}

