#include <iostream>

int main() {
    // avoid 2 dimensional arrays whenever possible
    // abstract it into one dimension for much better performance

    // rows
    // int* array = new int[50];
    int** a2d = new int*[50]; // alloc 200bytes in both cases
    // columns
    for (int i = 0; i < 50; i++) 
        a2d[i] = new int[50];

    a2d[0][0] = 0;
    a2d[0][1] = 0;
    a2d[0][2] = 0; // init first row

    // clean memory
    for (int i = 0; i < 50; i++) 
        delete[] a2d[i];

    delete[] a2d; 

    // allocate 3 dimensions
    int*** a3d = new int**[50];
    for (int i = 0; i < 50; i++) 
    {
        a3d[i] = new int*[50];
        for (int j = 0; j < 50; j++) 
        {
            int** ptr = a3d[i];
            ptr[j] = new int[50];
        }
    }

    // assign
    a3d[0][0][0] = 0;
    a3d[0][0][1] = 0;
    a3d[0][0][2] = 0;

   // clean memory
    for (int i = 0; i < 50; i++) 
    {
        for (int j = 0; j < 50; j++) 
        {
            int** ptr = a3d[i];
            delete[] ptr[j];
        }
        delete[] a3d[i];
    }
    delete[] a3d;


    // flat array abstraction much faster
    int* array = new int[5 * 5];
    for (int y = 0; y < 5; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            array[x+y*5] = 2;
        }

    }

    std::cin.get();
}