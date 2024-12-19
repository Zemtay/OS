#include "stdio.h"
#include "string.h"
#include "type.h"
#include "unistd.h"

int main(int argc, char *argv[]) {
	char working_path[256];
	getcwd(working_path, 256);

	printf("%s\n", working_path);
	return 0;
}
