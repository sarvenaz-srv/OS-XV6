#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char ** argv){
    if(argc<2){
        printf(1, "Enter the policy number to which you want to switch\n");
        exit();
    }
    int pol = atoi(argv[1]);
    int res = change_policy(pol);
    if(res >= 0){
        printf(1, "Success: current policy number is %d\n", res);
    }
    else{
        printf(1, "Change Policy Failed\n");
    }
    exit();
}