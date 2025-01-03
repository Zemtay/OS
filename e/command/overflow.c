#include "type.h"
#include "stdio.h"
#include "string.h"
char *PASSWORD = "1234567\0";
int verify1(char *password){
    int check;
    char buf[8];
    check = strcmp(password, PASSWORD);
    printf("|%d|", check);
    strcpy(buf, password);
    printf("*%d*", check);
    return check;
}

int verify2(char *password){
    int check;
    char buf[8];
    int len = strlen(password);
    check = strcmp(password, PASSWORD);
    printf("|%d|", check);
    // strlen没有考虑末尾的'\0' 
    if(len >= 8){
        printf("exceed the buf!\n");
    }
    strcpy(buf, password);
    printf("*%d*", check);
    return check;
}

int verify3(char *password){
    __asm__ __volatile__("xchg %bx, %bx");
    int check = 1;
    u32 canary = 0xffffffff;
    char buf[8];
    check = strcmp(password, PASSWORD);
    printf("|%d|", check);

    strcpy(buf, password);
    if(canary != 0xffffffff){
        printf("canary %d, %x, exceed the buf!\n", canary, canary);
    }
    printf("*%d*", check);
    for(int i = 0; i < 50000; i ++){
        ;
    }
    printf("*2: %x*", canary);
    __asm__ __volatile__("xchg %bx, %bx");
    return check;
}

int main(int argc, char * argv[]){
    char *input = "abcdefgh\0";  //
    int flag = 0;
    
    printf("======> test1: no protection\n");
    flag = verify1(input);
    if(flag){
        printf("Incorrect!\n");
    }
    else{
        printf("You are right!\n");
    }

    printf("======> test2: check the length\n");
    flag = verify2(input);
    if(flag){
        printf("Incorrect!\n");
    }
    else{
        printf("You are right!\n");
    }

    printf("======> test3: put canary\n");
    flag = verify3(input);
    if(flag){
        printf("Incorrect!\n");
    }
    else{
        printf("You are right!\n");
    }
	return 0;
}
