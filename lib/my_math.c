#include <math.h>
//Unfortunate workaround. Including math.h in c++ code resutlts in including definitions
//of malloc.Not a serious problem but can be annoying when you dont want stdlib.h to
//be included in your main code (in memory_manager.c).

double my_log2(double num){
    return log2(num);
}

int my_ceil(double num){
    return ceil(num);
}

