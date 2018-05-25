#include <iostream>
 
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

extern "C" DLLEXPORT double *DOT(double A[3][3], double B[3][3]) {
    cout << A[0][0] << std::endl;
    
    int i = 0;
    int j = 0;
    int l = 3;
    double C[3];

    // for (i = 0; i < l; i++)
    // {
    //     C[i] = 0;
    //     for (j = 0; j < l; j++)
    //         C[i] +=  A[i][j] * B[i][j];
 
    // }
    // Printing matrix A //
    cout << "Matrix A : \n ";
    for (i = 0; i < l; i++)
    {
        for (j = 0; j < l; j++)
            cout << A[i][j] << " ";
        cout << "\n ";
    }
 
    // Printing matrix B //
    cout << "Matrix B : \n ";
    for (i = 0; i < l; i++)
    {
        for (j = 0; j < l; j++)
            cout << B[i][j] << " ";
        cout << "\n ";     
    }
    cout << "Dot product : \n ";
    for (i = 0; i < l; i++)
       cout << C[i] << " ";

    return C;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X) {
  fprintf(stderr, "%f\n", X);
  return 0;
}

