#include <stdio.h>
#include "minero.h"
#include "monitor.h"
#include<string.h>

#define MAX_BUF

int main(int argc, char* argv[]){

    int nProc, nSec;
    
    char ejecutable[MAX_BUF] = argv[0];

    if(strcasecmp(ejecutable, "miner")){
        if(argc == 3) {
            nProc = atoi(argv[1]);
            nSec = atoi(argv[2]);
        }
        if(argc != 3 || nProc < 1 || nSec <1 || nP){
            minero();
        }
    }else if (strcasecmp(ejecutable, "miner")){
        
    }else {
        printf("Error en el nombre del ejecutable");
    }

    return 0;
}