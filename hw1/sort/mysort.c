#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* See `man 3 getopt()` for how to use the getopt function. */
#include <getopt.h>
#define MAX_SIZE 1024

int compar (const void *a,const void *b) {
    return strcmp(*(char**)a,*(char**)b);
}
int compar_r (const void *a,const void *b) {
    return strcmp(*(char**)b,*(char**)a);
} 

int main(int argc, char** argv)
{
    int ch;
    int line_num;
    int reverse=0;
    int fromFile=0;
    FILE *fp;
    int i;
    int line_limit=-1;
    int line_limit_tmp;
    char **str;
    char *invalid_optarg;
    
    while ( (ch = getopt(argc, argv, "rn:")) != -1 ) {
        switch (ch) {
            case 'r':
                //printf ("r\n");
                reverse = 1;
                break;
            case 'n':
                //printf ("%s %d\n",optarg,(int)strlen(optarg));
                line_limit_tmp = strtol(optarg, &invalid_optarg, 10);
                //printf ("%d %s\n",line_limit_tmp,invalid_optarg);
                if ((*invalid_optarg != '\0') || (line_limit_tmp < 0)) {
                    fprintf(stderr,"invalid number for option \"n\"!\n");
                    exit(1);
                }
                line_limit = line_limit_tmp;
                break;
            case '?':
                fprintf(stderr,"invalid option!\n");
                exit(1);
                break;
            default:
                fprintf(stderr,"invalid option!\n");
                exit(1);
        }
    }

    if (argc-optind>1) {
        fprintf(stderr,"More than 1 file name provided!\n");
        exit(1);
    }
    else if (argc-optind==1) {
        fromFile = 1;
        //printf("%s\n",argv[optind]);
        fp = fopen(argv[optind], "r");
        if(fp == NULL) {  
            perror("file open error");  
            exit(1);
        }
    }
    else
        fromFile = 0;
    //printf ("\n arg:%d %d %s\n",argc,optind,argv[optind]);
    
    str = (char**)malloc(sizeof(char*));
    line_num = 0;
    while (1) {
        *(str+line_num) = (char*)malloc(sizeof(char)*MAX_SIZE);

        if (fromFile) {
            if (fgets(*(str+line_num),MAX_SIZE,fp) == NULL )
                break;
        }
        else {
            if (fgets(*(str+line_num),MAX_SIZE,stdin) == NULL )
                break;
        }
        
        //printf ("%s",*(str+line_num));
        line_num++;
        str = (char**)realloc(str,sizeof(char*)*(line_num+1));
        
    }
    //for (i=0;i<line_num;i++) {
    //    printf ("%s",*(str+i));
    //}
    if (reverse)
        qsort(str,line_num, sizeof(char*), compar_r);
    else
        qsort(str,line_num, sizeof(char*), compar);

    if ((line_limit == -1) ||(line_limit>line_num))
        line_limit = line_num;


    for (i=0;i<line_limit;i++) {
        printf ("%s",*(str+i));
    }
	/* If successful, exit with a zero status. */
	return 0;
}
