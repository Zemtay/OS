[section .text]
global _start

_start:
    pushad
    jmp prepare
main:
    pop edx ; 得到msg的地址
    xor eax, eax ; _NR_printx=0
    int 0x90 ; #define INT_VECTOR_SYS_CALL  0x90
loop:
    jmp loop
prepare:
    call main
msg:
    db "overflow", 0xA, 0x0
