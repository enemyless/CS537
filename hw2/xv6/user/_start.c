#include "types.h"
#include "user.h"

extern int main (int argc,char* argv[]);

void _start(int argc,char* argv[]) {
    int ret;
    ret = main(argc,argv);
    exit(ret);
    return;
}
