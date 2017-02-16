#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_SIZE 1024

//#define DEBUG
#ifdef DEBUG
#    define DEBUG_PRINT(x) printf x
#else
#    define DEBUG_PRINT(x) do {} while (0)
#endif

typedef struct process {
   pid_t pid;
   //char *cmd;
   char cmd[MAX_SIZE];
} proc;

void checkChildProcess(proc* p,int *cnt) {
    DEBUG_PRINT(("Before cnt : %x\n",*cnt));
    if (!*cnt) return;
    for(int i=0;i<*cnt;i++) {
        int status;
        int st_p = waitpid(p[i].pid,&status,WNOHANG);
        DEBUG_PRINT(("%d ~ %s ~ %d   pid_rcv : %d\n",i,p[i].cmd, p[i].pid,st_p));
//        if ( st_p == -1 )
//            perror("waitpid error!");
//        else if ( st_p > 0 ) {
        if ( st_p != 0 ) {
            fprintf(stderr, "[%s (%d) completed with status %d]\n", p[i].cmd, st_p, WEXITSTATUS(status));
            //free(p[i].cmd);
            if ( i < (*cnt)-1 ) { // not the last one
                p[i] = p[(*cnt)-1];
            }
            (*cnt)--;
            DEBUG_PRINT(("Current cnt : %d\n",*cnt));
        }
    }
    return;
}

int main(int argc, char** argv)
{
	int BatchMode = 1;
    char *PromtMsg = "sqysh$ ";
    FILE *fp;
    char *cmd;
    char **cmdSplit; // splited cmd
    int optCnt; // count number of the options
    int optLen; // the length of the current option
    int cmdPos; // position moving along the received cmd
    int spaceFlag; // indicates whether it is a space for the last char
    int voidLine;
    char* iFileName;
    int iFileDesc;
    char* oFileName;
    int oFileDesc;
    int bgRun=0;
    char *cdPath;
    char pwdPath[MAX_SIZE];
    //char ErrMsg[50];
    int exitProg=0;
    char** args;
    int oldInStream;
    int oldOutStream;
    proc *bgChildProcess;
    int bgChildProcessCnt=0;

    if (argc > 2) {
        char *ErrMsg = "Too many arguements!\n";
        write(STDERR_FILENO,ErrMsg,strlen(ErrMsg));
        exit(1);
    }
    else if (argc == 1) {
        fp = stdin;
        if (isatty(STDIN_FILENO)) {
            BatchMode = 0;
        }
       // else {
       //     perror("Terminal is not attached to STDIN_FILENO!");  
       // }
    }
    else {
        fp = fopen(argv[1], "r");
        if(fp == NULL) {  
            perror("file open error");  
            exit(1);
        }
    }

    bgChildProcess = (proc*)malloc(sizeof(proc));

    while(1) {
        cmd = (char*)malloc(sizeof(char)*MAX_SIZE);
        checkChildProcess(bgChildProcess,&bgChildProcessCnt);
        if (!BatchMode) write(STDOUT_FILENO,PromtMsg,strlen(PromtMsg));
        if (fgets(cmd,MAX_SIZE,fp) == NULL ) {
            free(cmd);
            if (bgChildProcess) free(bgChildProcess);
            exit(0);
        }
        checkChildProcess(bgChildProcess,&bgChildProcessCnt);
        DEBUG_PRINT (("%s\n",cmd));

        optCnt = 0;
        cmdPos = 0;
        voidLine = 1;
        cmdSplit = (char**)malloc(sizeof(char*));
        iFileName = NULL;
        oFileName = NULL;
        bgRun = 0;
        
        DEBUG_PRINT (("start parsing\n"));
        spaceFlag = 1;
        while (1) { // while loop for parsing one cmd of a cmd
            //DEBUG_PRINT (("current char:\"%c\"\n",*(cmd+cmdPos)));
            if ( *(cmd+cmdPos) == '\n' ) { // End of cmd
                //DEBUG_PRINT (("end of cmd\n"));
                if (!voidLine) {
                    optLen++;
                    *(cmdSplit+optCnt-1) = (char*)realloc(*(cmdSplit+optCnt-1),sizeof(char)*(optLen+1));
                    *(*(cmdSplit+optCnt-1)+optLen) = '\0';
                }
                break;
            } 
            else if ( spaceFlag ) {
                if ( *(cmd+cmdPos) == ' ' ) { // still space, keep going
                    //DEBUG_PRINT (("another space\n"));
                    printf ("start parsing\n");
                    spaceFlag = 1;
                    //continue;
                }
                else { // End of space, new options found
                    //DEBUG_PRINT (("end of space, new options found\n"));
                    voidLine = 0;
                    spaceFlag = 0;
                    optLen = 0;
                    optCnt++;
                    cmdSplit = (char**)realloc(cmdSplit,sizeof(char*)*(optCnt));
                    *(cmdSplit+optCnt-1) = (char*)malloc(sizeof(char)*(optLen+1));
                    **(cmdSplit+optCnt-1) = *(cmd+cmdPos);
                }
            }
            else {
                if (*(cmd+cmdPos) == ' ') { // End of current option
                    //DEBUG_PRINT (("End of current options\n"));
                    voidLine = 0;
                    spaceFlag = 1;
                    optLen++;
                    *(cmdSplit+optCnt-1) = (char*)realloc(*(cmdSplit+optCnt-1),sizeof(char)*(optLen+1));
                    *(*(cmdSplit+optCnt-1)+optLen) = '\0';
                }
                else { // current option ingoing
                    //DEBUG_PRINT (("continue current option\n"));
                    voidLine = 0;
                    spaceFlag = 0;
                    optLen++;
                    *(cmdSplit+optCnt-1) = (char*)realloc(*(cmdSplit+optCnt-1),sizeof(char)*(optLen+1));
                    *(*(cmdSplit+optCnt-1)+optLen) = *(cmd+cmdPos);
                }
            }
            cmdPos++;
        }
        
        for (int i=0; i<optCnt; i++) {
            DEBUG_PRINT (("%s",*(cmdSplit+i)));
            DEBUG_PRINT (("++++%d\n",(int)strlen(*(cmdSplit+i))));
        }

        // End of parsing one cmd
        // Start detecting special syntax
        
        int optCnt_ori = optCnt;
        for (int i=0; i<optCnt_ori; i++) {
            if ( !strcmp(*(cmdSplit+i),"<") ) {
                iFileName = *(cmdSplit+i+1);
                i++;
                optCnt -= 2;
            }
            
            else if ( !strcmp(*(cmdSplit+i),">") ) {
                oFileName = *(cmdSplit+i+1);
                i++;
                optCnt -= 2;
            }

            else if ( !strcmp(*(cmdSplit+i),"&") ) {
                bgRun = 1;
                optCnt--;
            }
        }

        for (int i=0; i<optCnt; i++) DEBUG_PRINT (("%s\n",*(cmdSplit+i)));

        DEBUG_PRINT (("\nDEBUG: parameters are:\n"));
        DEBUG_PRINT (("DEBUG: iFileName:%s\n",iFileName ? iFileName : "N/A"));
        DEBUG_PRINT (("DEBUG: oFileName:%s\n",oFileName ? oFileName : "N/A"));
        DEBUG_PRINT (("DEBUG: background : %s\n",bgRun ? "Yes" : "No"));
        DEBUG_PRINT (("DEBUG: optCnt_ori: %d\nDEBUG: optCnt: %d\n",optCnt_ori,optCnt));
        DEBUG_PRINT (("DEBUG: voidLine: %d\n",voidLine));

        if (!voidLine) {

            // Finished special syntax detecting, start running

            cdPath = NULL;
            DEBUG_PRINT (("DEBUG: current cmd: %s\n",*cmdSplit));
            if ( !strcmp(*(cmdSplit),"cd") ) {
                if ( optCnt>2 )
                    fprintf(stderr, "cd: too many arguments\n");
                else if ( optCnt==2 )
                    cdPath = *(cmdSplit+1);
                else
                    cdPath = getenv("HOME");

                if (cdPath)
                    if ( chdir(cdPath) == -1 )
                        fprintf(stderr, "cd: %s: %s\n", cdPath, strerror(errno));
            }

            else if ( !strcmp(*(cmdSplit),"pwd") ) {
                if (getwd(pwdPath) != NULL)
                    fprintf (stdout,"%s\n",pwdPath);
                else
                    perror("getcwd() error");

            }

            else if ( !strcmp(*(cmdSplit),"exit") ) exitProg = 1;

            else {
                //pid_t parent = getpid();
                pid_t pid = fork();


                if (pid == -1) { // error, failed to fork()
                    fprintf(stderr, "%s: %s\n", *cmdSplit, strerror(errno));
                } 
                else if (pid > 0) { // parent process
                    int status;
                    if (bgRun) {
                        bgChildProcessCnt++;
                        bgChildProcess = (proc*)realloc(bgChildProcess,sizeof(proc)*(bgChildProcessCnt+1));
                        bgChildProcess[bgChildProcessCnt-1].pid = pid;
                        //bgChildProcess[bgChildProcessCnt-1].cmd = (char*)malloc(sizeof(char)*(strlen(*cmdSplit)+1));
                        strcpy (bgChildProcess[bgChildProcessCnt-1].cmd,cmdSplit[0]);
                        for (int i=0; i<bgChildProcessCnt; i++) {
                            DEBUG_PRINT (("cnt = %d pid = %d, cmd = %s\n",i,bgChildProcess[i].pid,bgChildProcess[i].cmd));
                        }
                    }
                    else {
                        waitpid(pid, &status, 0);
                        DEBUG_PRINT (("DEBUG: Done pid: %d cmd: %s\n",pid,*cmdSplit));
                    }
                }
                else { // child
                    if (iFileName) {
                        iFileDesc = open(iFileName,O_RDONLY);
                        if ( iFileDesc == -1 )
                            perror("file open error");
                        oldInStream = dup(STDIN_FILENO);
                        if ( dup2(iFileDesc,STDIN_FILENO) == -1 ){
                            perror("I/O Redirection Error!");
                        }
                    }
                    if (oFileName) {
                        oFileDesc = open(oFileName,O_CREAT|O_WRONLY|O_TRUNC,0644);
                        if ( oFileDesc == -1 )
                            perror("file open error");
                        oldOutStream = dup(STDOUT_FILENO);
                        if ( dup2(oFileDesc,STDOUT_FILENO) == -1 ){
                            perror("I/O Redirection Error!");
                        }
                    }

                    args = (char**)malloc(sizeof(char*)*(optCnt+1));
                    for (int i=0;i<optCnt;i++)
                        *(args+i) = *(cmdSplit+i);
                    *(args+optCnt) = '\0';

                    if ( execvp(args[0],args) == -1 ) {
                        fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
                        free(args);
                        
                        for (int i=0; i<optCnt_ori; i++) {
                            free (*(cmdSplit+i));
                        }
                        free(cmdSplit);
                        free(bgChildProcess);
                        free(cmd);
                        exit(1);
                    }
                    free(args);
                    if (iFileName) {
                        if ( dup2(oldInStream,STDIN_FILENO) == -1 ){
                            perror("I/O Redirection Error!");
                        }
                        close(iFileDesc);
                    }
                    if (oFileName) {
                        if ( dup2(oldOutStream,STDOUT_FILENO) == -1 ){
                            perror("I/O Redirection Error!");
                        }
                        close(oFileDesc);
                    }
                }
            }
            
            // End of executiong, free memory now
            for (int i=0; i<optCnt_ori; i++) {
                free (*(cmdSplit+i));
            }

        }
        
        free(cmdSplit);
        free(cmd);        
        if (exitProg) {
            free(bgChildProcess);
            exit(0);
        }
    } // End of while(1)
    
    return 0;
}
