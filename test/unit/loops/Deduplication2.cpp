// RUN: true

#include "Deduplication.h"
#include <algorithm>

void unique_for_loops(){
    int arr[] = {1, 2, 3};

    for (auto el : arr){
        // do something
    }
    for (int i = 0; i < 3; i++){
        // do something
    }
    {
        int i = 0;
        while (i < 3){
            // do something
        }
    }
    {
        int i = 0;
        do {
            // do something
            i++;
        } while (i < 3);
    }
}