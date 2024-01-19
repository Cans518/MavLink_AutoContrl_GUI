#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main(){
    system("./Script/b \"mode guided\"");
    sleep(1);
    system("./Script/b \"arm throttle\"");
    sleep(1);
    system("./Script/b \"takeoff 50\"");
    return 0;
}