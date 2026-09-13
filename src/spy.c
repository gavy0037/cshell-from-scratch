#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<errno.h>
#include<unistd.h>
#include<dirent.h>
#include<signal.h>
#include<linux/limits.h> // for path_max
#include<sys/types.h>
#include<sys/stat.h>
#include"../include/spy.h"
#include"../include/lexer.h"
#include"../include/execute.h"

int validate_spy_syntax(Token *cmd){
    // return 0 if to show to current shell's files , return pid if found pid , return -1 if invalid syntax , return -2 if that process does not exist, return -3 if permission denied
    Token *t = cmd -> next;
    if(t == NULL){
        return getppid() ;
    }

    if(t->next != NULL) return -1;

    if(t->text[0] == '-') return -1;// for negetive numbers
    // check if this current t's text is an actual number
    for(int i = 0 ; i < (int)strlen(t->text) ; i++){
        if(!isdigit(t->text[i])){
            return -1;
        }
    }

    pid_t p = atoi(t->text);

    if(kill(p , 0) == -1){
        if(errno == ESRCH){
            return -2;
        }else if(errno == EPERM){
            return -3;
        }
    }
    return p;
}

char *classify(mode_t mode) {
    if (S_ISREG(mode))  return "REG";
    if (S_ISDIR(mode))  return "DIR";
    if (S_ISCHR(mode))  return "CHR";
    if (S_ISBLK(mode))  return "BLK";
    return "UNKNOWN";
}

void spy_process(Token *cmd){
    int p = validate_spy_syntax(cmd);

    if(p == -1){
        printf("spy: invalid syntax\n");
        return;
    }else if(p == -2){
        printf("spy: no such process\n");
        return;
    }else if(p == -3){
        printf("spy: permission denied\n");
        return;
    }

    printf("PID   FD    TYPE   PATH\n");

    char sys_link_path[64] , target[PATH_MAX]; // this path max is the max length of path that occurs on this system , i get it from linux/limits.h

    snprintf(sys_link_path , sizeof(sys_link_path) , "/proc/%d/cwd" , p);

    int len = readlink(sys_link_path , target , sizeof(target)-1);

    if(len != -1){
        target[len] = '\0';
        printf("%-5d %-5s %-6s %s\n" , p , "cwd" , "DIR" , target);
    }

    snprintf(sys_link_path , sizeof(sys_link_path) , "/proc/%d/exe" , p);
    len = readlink(sys_link_path , target , sizeof(target)-1);
    if(len != -1){
        target[len] = '\0';
        printf("%-5d %-5s %-6s %s\n" , p , "txt" , "REG" , target);
    }


    char mapspath[64];
    snprintf(mapspath, sizeof(mapspath), "/proc/%d/maps", p);
    FILE *f = fopen(mapspath, "r");

    if (f) {
        char line[1024];
        char *seen[4096];// paths that i have explored
        int seen_count = 0;

        while (fgets(line, sizeof(line), f)) {
            // maps line looks like:
            // r-xp 00000000 08:01 131074   /usr/lib/libc.so.6
            char *path = strrchr(line, ' ');
            if (path == NULL) continue;
            path++; // skip the space

            // extract path into a buffer, trim trailing '\n'
            int plen = strlen(path);
            if (plen > 0 && path[plen - 1] == '\n') {
                path[plen - 1] = '\0';
                plen--;
            }
            
            if (plen == 0 || path[0] == '[' ) continue;  // skip anon/pseudo mappings

            // check if this path has already been processed
            int already = 0;
            for (int i = 0; i < seen_count; i++)
                if (strcmp(seen[i], path) == 0) { already = 1; break; }
            if (already) continue;

            seen[seen_count++] = strdup(path);
            printf("%-5d %-5s %-6s %s\n", p, "mem", "REG", path);
        }
        for (int i = 0; i < seen_count; i++) free(seen[i]);
        fclose(f);
    }

    char fddir[64];
    snprintf(fddir, sizeof(fddir), "/proc/%d/fd", p);
    DIR *d = opendir(fddir);

    if (d) {
        struct dirent *entry;
        char *fd_paths[1024] = {NULL};
        char *fd_types[1024] = {NULL};

        // collect entries, then sort numerically before printing (readdir order isn't guaranteed sorted)
        while ((entry = readdir(d)) != NULL) {
            if (entry->d_name[0] == '.') continue;   // skip . and ..
            int fdnum = atoi(entry->d_name);

            if (fdnum >= 0 && fdnum < 1024) {
                char fdpath[512], resolved[PATH_MAX];
                snprintf(fdpath, sizeof(fdpath), "%s/%s", fddir, entry->d_name);
                int rlen = readlink(fdpath, resolved, sizeof(resolved) - 1);
                if (rlen == -1) continue;
                resolved[rlen] = '\0';

                struct stat st;
                stat(fdpath, &st);   // follows the symlink automatically
                const char *type = classify(st.st_mode);  // REG/DIR/CHR/BLK

                fd_paths[fdnum] = strdup(resolved);
                fd_types[fdnum] = strdup(type);
            }
        }
        closedir(d);

        for (int i = 0; i < 1024; i++) {
            if (fd_paths[i] != NULL) {
                printf("%-5d %-5d %-6s %s\n", p, i, fd_types[i], fd_paths[i]);
                free(fd_paths[i]);
                free(fd_types[i]);
            }
        }
    }


}