#include "type.h"
#include "stdio.h"
#include "string.h"

// char payload[] = {

// };
// 注意，在strcpy时遇到\0就会停止，所以不能填充0x00
char payload[] = {0x01,0x01,
    0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,
    0x1d,0x10,0x00,0x00
};

void test(char *str){
    char buf[2];
    strcpy(buf, payload);
}

void shellcode(){
    printf("Bufferoverflow!\n");
}

int main(){
    test(payload);
    printf("Everything is OK.\n");
    return 0;
}