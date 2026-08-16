//#include "include/prompt.h"
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<pwd.h>

void display_prompt(char *home_dir){
    uid_t uid = getuid();
    struct passwd *user_info = getpwuid(uid);
    char *userName = user_info->pw_name;

    char hostName[256];
    gethostname(hostName , sizeof(hostName));

    // the current dir is the home dir
    char display_dir[4096];
    char curr_dir[4096];
    getcwd(curr_dir , sizeof(curr_dir));

    int l = strlen(home_dir);
    if(strncmp(home_dir , curr_dir , l) == 0){
        if(curr_dir[l] == '\0'){
            display_dir[0] = '~';
            display_dir[1] = '\0';
        }else if(curr_dir[l] == '/'){
            snprintf(display_dir , sizeof(display_dir) , "~%s" , curr_dir + l);
        }else{
            snprintf(display_dir , sizeof(display_dir) , "%s" , curr_dir);
        }
    }else{
        snprintf(display_dir , sizeof(display_dir) , "%s" , curr_dir);
    }

    printf("<%s@%s:%s> " , userName , hostName , display_dir);
}

int main(){
    char home_dir[4096];
    getcwd(home_dir , sizeof(home_dir));
    char input[4096];
    while(1){
        display_prompt(home_dir);
        
        if(fgets(input , sizeof(input) , stdin) == NULL){
            printf("\n");
            break;
        }
        
        printf("%s" , input);
        chdir("..");
    }
}