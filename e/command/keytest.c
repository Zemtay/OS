// clang-format off
#include "stdio.h"

#include "keywise.h"

// clang-format on

int main(int argc, char *argv[]) {
	while (1) {
		if (_kbhit() == 0) {
			continue;
		}
        printf("ready:");
		printf("0x%x\n",_getch());
	}
    return 0;
}