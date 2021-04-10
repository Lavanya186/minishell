#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<string.h>
#include<fcntl.h>

char** tokenize(char* command)                         //function for tokenizing the given command string
{
        char delim[]=" ";
        int no_of_args = 20;
        char **args = malloc(sizeof(char *) *no_of_args) ;
        char *ptr=strtok(command,delim);              //string token function
        int i=0;
        while(ptr!=NULL)
        {
                args[i++]=ptr;
                ptr = strtok(NULL,delim);
        }
        args[i]=0;
        return args;
}


int execute_simple(char **args)             // function for executing simple commands
{
        pid_t pid=fork();                       // child creation
        int status,exit_status;
        if(pid == -1)
                perror("fork failed\n");
        if(pid==0)
        {
               if( execvp(args[0],args) == -1)                   //execvp() for executing commands
                        perror("Error in execvp()\n");
        }
        waitpid(pid,&status,0);
        if(WIFEXITED(status))
        {
                exit_status=WEXITSTATUS(status);                 // getting the exit status of child process
        }
        return exit_status;
}
void execute_background(char **args)          //function for executing commands in backgroud
{
        pid_t pid=fork();                    // child process creation
        if(pid==-1)
        {
                perror("fork failed\n");
        }
        if(pid==0)
        {
                pid=fork();             //child of child creation
                if(pid==0)
                {
                        int i=0;
                        while(args[i]!=0)
                        {
                          i++;
                        }
                        args[i-1]=0;             // removing the '&' from the arguments
                        if(execvp(args[0],args) == -1)            //now, executing the commands
                                perror("Error in execvp()\n");
                }
                else
                {
                        exit(0);
                }
        }
        else
        {
                wait(NULL);                // this indicates the parent wouldn't wait for the child to complete
        }
}
int execute_redirect(char **args)         // function to execute commands with redirection
{
        pid_t pid=fork();                // child creation
        int i,file,file1;
        int status,std_out=dup(1),std_in=dup(0),count=0,exit_status;
        if(pid == -1)
                perror("fork failed\n");
        if(pid==0)
        {
                for(i=0;args[i]!=0;i++)
                {
                        if((strcmp(args[i],"<")==0)||(strcmp(args[i],"<<")==0))             //checking which redirection operation
                        {
                                count=0;
                                break;
                        }
                        else if(strcmp(args[i],">")==0)
                        {
                                count=1;
                                break;
                        }
                        else if(strcmp(args[i],">>")==0)
                        {
                                count=2;
                                break;
                        }
                }
                switch(count)
                {
                        case 0: file=open(args[i+1],O_RDONLY,0777);     //input redirection
                                if(file==-1)
                                {
                                        return 2;
                                }
                                close(0);
                                file1=dup2(file,0);
                                if(file1<0)
                                {
                                        perror("error in dup2");
                                }
                                close(file);
                                args[i]=0;                     // making the redirection operator as null
                                if(execvp(args[0],args) == -1)
                                        perror("Error in execvp()\n");
                                break;
                        case 1:
                                file=open(args[i+1],O_WRONLY|O_TRUNC|O_CREAT,0777);   // output redirection
                                if(file==-1)
                                {
                                        return 2;
                                }
                                close(1);
                                file1=dup2(file,1);
                                if(file1<0)
                                {
                                        perror("error in dup2");
                                }
                                close(file);
                                args[i]=0;                 //making the redirection operator as null
                                if(execvp(args[0],args) == -1)
                                        perror("Error in execvp()\n");
                                break;
                        case 2:
                                file=open(args[i+1],O_RDWR|O_APPEND|O_CREAT,0777); // append redirection
                                if(file==-1)
                                {
                                        return 2;
                                }
                                close(1);
                                file1=dup2(file,1);
                                if(file1<0)
                                {
                                        perror("error in dup2");
                                }
                                close(file);
                                args[i]=0;         // making the redirection operator as null
                                if(execvp(args[0],args) == -1)
                                        perror("Error in execvp()\n");
                                break;
                }
                dup2(std_out,1);
                dup2(std_in,0);
        }
        waitpid(pid,&status,0);
        waitpid(pid,&status,0);
        if(WIFEXITED(status))
        {
                exit_status=WEXITSTATUS(status);        // getting theexit status of the child
        }
        return exit_status;
}

void pipe_sep(char *left[], char *right[],char *args[])        //function for seperating arguments based on pipe operator
{
        int x,y;
        for(x=0;strcmp(args[x],"|") != 0 ; x++)
        {       left[x] = args[x];
        }
        left[x++] = '\0';
        for(y=0; strcmp(args[x],"|") != 0,args[x] != NULL; x++,y++)
                right[y] = args[x];
        right[y] = '\0';

}

void execute_pipe(char **args)           // function for executing the commands with pipe operator
{
        int fpipe[2];
        int fdes;
        pid_t cpid1,cpid2;
        char *left[10],*right[10];
        pipe_sep(left,right,args);
        fflush(stdout);
        if(pipe(fpipe) == -1)
        {
                perror("pipe failed\n");
        }
        cpid1 = fork();          // child 1 creation
        if(cpid1 > 0)
                cpid2 = fork(); // child 2 creation
        if(cpid1 == 0)
        {
                if(close(1) == -1)
                        perror("Error in closing stdout\n");
                fdes = dup(fpipe[1]);
                if( fdes == -1)
                        perror("Error in dup function\n");
                if(fdes != 1)
                        fprintf(stderr, "pipe output is not at 1\n");
                close(fpipe[0]);
                close(fpipe[1]);
                if( execvp(left[0],left) == -1)       //executing the arguments in left with execvp()
                        perror("error in execvp()\n");
        }
        else if(cpid2 ==  0)           // output of the child1 will be the input of child2
        {
                if( close(0) == -1 )
                        perror("error in close()\n");
                fdes = dup(fpipe[0]);
                if( fdes == -1)
                        perror("Error in dup()\n");
                if( fdes != 0)
                        fprintf(stderr,"pipe input is not in 0\n");
                close(fpipe[0]);
                close(fpipe[1]);
                if( execvp(right[0],right) == -1)    //executing the arguments in right with execvp()
                        perror("Error in execvp\n");
        }
        else
        {       int status;
                close(fpipe[0]);
                close(fpipe[1]);
                if( waitpid(cpid1, &status, 0) == -1  )
                        perror("Error in waitpid\n");
                if(WIFEXITED(status) == 0)
                        printf("child 1 exited with code %d\n",WEXITSTATUS(status));     //getting the exit status of child 1
                if(waitpid(cpid2, &status, 0)== -1 )
                        perror("Error in waitpid\n");
                if(WIFEXITED(status) == 0)
                        printf("child 2 exited with code %d\n",WEXITSTATUS(status));     // getting the exit status of child 2
        }
}

int main()                //main function
{
        const char prompt[]="minishell$ ";
        int flag=0,exit_status;
        char command[50];
        char **args;
        system("clear");
        printf("%s",prompt);
        scanf(" %[^\n]",command);        //getting the input

        while(strcmp(command,"exit")!=0)
        {
                flag = 0;
                args = tokenize(command);       // calling the tokenize function

                for(int i=0;args[i]!=0;i++)
                {                                      //checking  which operator is present in the command
                        if(strcmp(args[i],"&")==0)
                        {
                                flag=1;
                                break;
                        }
                        if(strcmp(args[i],"|")==0)
                        {
                                flag=2;
                                break;
                        }
                        if((strcmp(args[i],">")==0)||(strcmp(args[i],"<")==0)||(strcmp(args[i],">>")==0)||(strcmp(args[i],"<<")==0))
                        {
                                flag=3;
                                break;
                        }
                }

                switch(flag)
                {
                        case 0:
                                exit_status=execute_simple(args);
                                break;
                        case 1:
                                execute_background(args);
                                break;
                        case 3:
                                exit_status=execute_redirect(args);
                                break;
                        case 2:
                                execute_pipe(args);
                                break;
                }

                if(exit_status!=0)
                {
                        printf("exit status of child %d\n",exit_status);
                }

                printf("%s",prompt);
                scanf(" %[^\n]",command);
        }
        exit(0);
}
