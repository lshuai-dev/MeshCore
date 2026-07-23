/*****************************************************************************
 * matrix operations\vector operations\buffer operations\mean and variance calculations
 * quaternion operations\mean filter\med value filter\general filter
 * Author: Dong Xiaoguang
 * Created on 2015/11/24
 *****************************************************************************/

#ifndef MY_MATH_H_INCLUDED
#define MY_MATH_H_INCLUDED

#include "MemsicCommon.h"

//===========================REAL or float=========================
#define PRECISION_float 0
#if PRECISION_float
	#define REAL float
#else
	#define REAL float
#endif

// buffer
#define Fusion_DATA_BfSIZE               30 //磁\陀螺缓冲区长度

#define MED_BF_SIZE  Fusion_DATA_BfSIZE           // mag and acc samples for med filter
#define MEAN_BF_SIZE Fusion_DATA_BfSIZE         // mag and acc samples for mean filter


//===========================some constants==========================
#if PRECISION_float
#define PI 3.14159265358979323846
#define TWO_PI 6.28318530717959
#define HALF_PI 1.5707963267949
#define R2D 57.2957795130823
#define D2R 0.01745329251994
#else
#define PI 3.14159265358979323846F
#define TWO_PI 6.28318530717959F
#define HALF_PI 1.5707963267949F
#define R2D 57.2957795130823F
#define D2R 0.01745329251994F
#endif
//======================data struct definitions======================
typedef struct _buffer
{
    REAL *d;// data storage, each column represents a set of data
    int m;//row number
    int n;//column number
    int i;//index for data being put into the buffer, -1 means buffer is empty
    int full;//1 means buffer is full, 0 not
    int num;//data number in the buffer
} Buffer;

//===================================================================
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) > (b) ? (b) : (a))
#define myabs(x)	( (x)>=0.0 ? (x) : -(x) ) 

//===========================math operations=========================
//-------------------------------------------------------------------
// print all the elements in a vector
// input:   v--the vector
//          n--vector length
// output:
// return:
void printVec(REAL *v, int n);

//-------------------------------------------------------------------
// print all the elements in a matrix
// input:   a--the matrix
//          m--row number
//          n--column number
// output:
// return:
void printMtx(REAL *a, int m, int n);

//-------------------------------------------------------------------
// vector add
// a, b and c are vectors with n elements
// c = a+b
void vecAdd(REAL* a, REAL* b, REAL* c, int n);

//-------------------------------------------------------------------
// vector subtraction
// a, b and c are vectors with n elements
// c = a-b
void vecSub(REAL* a, REAL* b, REAL* c, int n);

//-------------------------------------------------------------------
// dot product
// a and b are vectors with n elements
// return a.b
REAL dot(REAL* a, REAL* b, int len);

//-------------------------------------------------------------------
// vector 2-norm
// return the 2-norm of a
REAL norm2(REAL* a, int n);

//-------------------------------------------------------------------
// normalize a vector, 1e-15 needs further validation
// input:   a--vector to be normalized
//          len--vector length
// output:  aN--normalized vector
// return:	2-norm of a
REAL vecNormalize(REAL* a, REAL* aN, int len);

//-------------------------------------------------------------------
// mean value of a vector
// input:   a--vector
//          n--length of the vector
// output:
// return: mean value of the first n elements of vector
REAL vecMean(REAL *a, int n);

//-------------------------------------------------------------------
// variance of a vector
// input:   a--vector
//          am--mean value
//          n--length of the vector
// output:
// return: variance of the first n elements of vector
REAL vecVar(REAL *a, REAL am, int n);

//-------------------------------------------------------------------
// sort a vector, ascending
// input:   a--vector
//          n--length of the vector
// output:  a--sorted vector
// return:
void vecSort(REAL *a, int n);

// med value filter
int medFilter_bf(Buffer *u, Buffer *y, int n);

// mean filter
int meanFilter_bf(Buffer *u, Buffer *y, int n);

//-------------------------------------------------------------------
// index of the max element in a vector
// input:   a--vector
//          n--length of the vector
// output:
// return:  index of the max element
int vecMax(REAL *a, int n);

//-------------------------------------------------------------------
// index of the min element in a vector
// input:   a--vector
//          n--length of the vector
// output:
// return:  index of the min element
int vecMin(REAL *a, int n);

//-------------------------------------------------------------------
// absolute values of a vector
// input:   a--vector
//          n--length of the vector
// output:  b--abs(a)
// return:
void vecAbs(REAL *a, int n, REAL *b);

//-------------------------------------------------------------------
// sum of all the elements of a vector
// input:   a--vector
//          n--length of the vector
// output:
// return:  sum of all the elements
REAL vecSum(REAL *a, int n);

//-------------------------------------------------------------------
// vector coross product
// input:   a--vector
//          b--vector
// output:   c--axb
// return:
void cross(REAL* a, REAL* b, REAL* axb);

//-------------------------------------------------------------------
// vector multiplied by a constant
// intput:  a--n vector
//          b--constant scalar
//          n--vector length
// output:  c--b*a,n vector
void vecMultiplyConst(REAL* a,REAL b, REAL* c, int n);

//-------------------------------------------------------------------
// vector duplication
// input:   vSrc--source vector
//          n--source vector length
// output:  vDst--dest vector
// return:
void vecDuplicate(REAL* vSrc, REAL* vDst, int n);

//-------------------------------------------------------------------
// cross product matrix of a vector
// input:   a--3-dim vector
// output:  b--3x3 matrix
// return:
void crossMtx(REAL* a, REAL *b);

//-------------------------------------------------------------------
// max absolute elements in a matrix and its corresponding index
// input:   a--a matrix
//          m--rows
//          n--columns
// output:  mMax--row number of the max absolute element
//          nMax--column number of the max absolute element
// return:  max absolute value
REAL mtxMaxAbsIdx(REAL *a, int m, int n, int *mMax, int *nMax);

//-------------------------------------------------------------------
// max absolute elements in a matrix
// input:   a--a matrix
//          m--rows
//          n--columns
// output:
// return:  max absolute value
REAL mtxMaxAbs(REAL *a, int m, int n);


//-------------------------------------------------------------------
// one row of a matrix
// input:   a--matrix
//          m--row size
//          n--col size
//          r--index of the row to be extracted
// output:  b--the r row of a
void mtxRow(REAL *a, int m, int n, int r, REAL *b);

//-------------------------------------------------------------------
// one column of a matrix
// input:   a--matrix
//          m--row size
//          n--col size
//          r--index of the column to be extracted
// output:  b--the r column of a
void mtxCol(REAL *a, int m, int n, int r, REAL *b);

//-------------------------------------------------------------------
// lower triangular part of a matrix
// input:   a--matrix
//          m--row size
//          n--col size
// output:  l--tril(a)
// return:
void mtxTril(REAL *a, int m, int n, REAL *l);

//-------------------------------------------------------------------
// upper triangular part of a matrix
// input:   a--matrix
//          m--row size
//          n--col size
// output:  u--triu(a)
// return:
void mtxTriu(REAL *a, int m, int n, REAL *u);

//-------------------------------------------------------------------
// matrix duplication
// input:   aSrc--source matrix
//          m--row number of the source matrix
//          n--column number of the source matrix
// output:  aDst--dest matrix
// return:
void mtxDuplicate(REAL* aSrc, REAL* aDst, int m, int n);

//-------------------------------------------------------------------
// matrix multiplication
// input: a--matrix(aRow x aCol), b--matrix(aCol x bCol)
// output: c--matrix(aRow x bCol)
void mtxMultiply(REAL* a, REAL* b, REAL* c, int aRow, int aCol,int bCol);

//-------------------------------------------------------------------
// matrix multiplied by a constant
// input: a--matrix(row x col), b--constant
// output: c--b*a;
void mtxMultiplyConst(REAL* a, REAL b, REAL* c, int row, int col);

//-------------------------------------------------------------------
// matrix add
// input: a and b are matrices(row x col)
// output: c = a+b
void mtxAdd(REAL* a,REAL *b,REAL *c, int row, int col);

//-------------------------------------------------------------------
// matrix substraction
// input: a and b are matrices(row x col)
// output: c = a-b
void mtxSub(REAL* a,REAL* b,REAL* c, int row, int col);

//-------------------------------------------------------------------
// merge two matrices along the row
// input: a--mxn, b--mxq
// output: c--mx(n+q)
void mtxMergeRow(REAL* a, REAL* b, REAL* c, int m, int n, int q);

//-------------------------------------------------------------------
// merge two matrices along col
// input: a--mxn, b--pxn
// output: c--(m+p)xn
void mtxMergeCol(REAL* a, REAL* b, REAL* c, int m, int n, int p);

//-------------------------------------------------------------------
// matrix tranpose
// input: a--matrix(row x col)
// output: aT = a'
void mtxTranspose(REAL* a, REAL* aT, int row, int col);

//-------------------------------------------------------------------
// matrix multiplied by a vector
// input: a--mxn matrix, b--nx1 vector
// return: c--a*b,mx1 vector
void mtxMultiplyVec(REAL* a, REAL* b, REAL* c, int m, int n);

//-------------------------------------------------------------------
// 2x2 matrix inverse
// input: a--matrix(2 x 2)
// output: x--inv(x)
void mtxInverse2(REAL* a, REAL* x);

//-------------------------------------------------------------------
// 3x3 matrix inverse
// input: a--matrix(3 x 3)
// output: x--inv(x)
void mtxInverse3(REAL* a, REAL* x);

//-------------------------------------------------------------------
// 4x4 matrix inverse
// input: a--matrix(4 x 4)
// output: x--inv(x)
void mtxInverse4(REAL* a, REAL* x);

//-------------------------------------------------------------------
// matrix inverse
// input: a--matrix(n x n)
// output: x--inv(x)
void mtxInverse(REAL* a, REAL* x, int n);

//-------------------------------------------------------------------
// matrix inverse, no malloc version
// input:	A--matrix(n x n)
//			iColInd--6x1 vector
//			iRowInd--6x1 vector
//			iPivot--6x1 vector
//			isize--matrix size n
// output:	x--inv(x)
void mtxAeqInvA(float *A[], int8 iColInd[], int8 iRowInd[], int8 iPivot[], int8 isize);

//-------------------------------------------------------------------
// generate diagonal matrix
// input:   a--nx1 vector containing the diagonal elements
//          n--length of the vector
// output:  b = diag(a), nxn
// return:
void diag(REAL* a, REAL* b, int n);

//-------------------------------------------------------------------
// generate identity matrix
// input:   a--matrix
//          n--size of the identity matrix
// output:  a = eye(n)
// return:
void eye(REAL *a, int n);

//-------------------------------------------------------------------
// generate zeros matrix
// input:   a--matrix
//          m--rows
//          n--columns
// output:  a = zeros(n)
// return:
void zeros(REAL *a, int m, int n);

#if 0
//-------------------------------------------------------------------
// generate ones matrix
// input:   a--matrix
//          m--rows
//          n--columns
// output:  a = ones(n)
// return:
void ones(REAL *a, int m, int n);
#endif

//-------------------------------------------------------------------
// exchange two row of a matrix
// input:   a--mxn
//          m--rows
//          n--columns
//          r1,r2--rows to be exchanged, zero based
// output:
// return:  1 means success, 0 error
int mtxExchangeRow(REAL* a, int m, int n, int r1, int r2);

//-------------------------------------------------------------------
// exchange two columns of a matrix
// input:   a--mxn
//          m--rows
//          n--columns
//          c1,c2--columns to be exchanged, zero based
// output:
// return:  1 means success, 0 error
int mtxExchangeCol(REAL* a, int m, int n, int c1, int c2);

//-------------------------------------------------------------------
// new a buffer
// input:   bf--pointer to the buffer
//			d--pointer to the memory for data storage
//          m--row number(length of each set of data)
//          n--column number(number of sets of data)
// output:
// return:
void bfNew(Buffer *bf, REAL*d, int m, int n);

//-------------------------------------------------------------------
// put data into the buffer
// input:   bf--buffer pointer
//          d--pointer to the data being put into the buffer
// output:
// return:
void bfPut(Buffer *bf, REAL* d);

//-------------------------------------------------------------------
// read data from the buffer
// input:   bf--buffer pointer
//          idx--data index, idx=0 means the latest data, idx=1 means data before the latest...
// output:
// return: 1 menas OK, 0 menas idx out of bound
int bfGet(Buffer *bf,REAL *d, int idx);

//-------------------------------------------------------------------
// clear the buffer
// input:   bf--buffer pointer
// output:
// return:
void bfClear(Buffer *bf);

#if  0
//-------------------------------------------------------------------
// apply med value filter to the buffer
// input:   u--input buffer
//          y--filtered value
//          n--filter length
// output:
// return: 1 menas OK, 0 means error
int medFilter(Buffer *u, REAL *y, int n);
#endif

//-------------------------------------------------------------------
// apply mean value filter to the buffer
// input:   u--input buffer
//          y--filtered value
//          n--filter length
// output:
// return: 1 menas OK, 0 means error
int meanFilter(Buffer *u, REAL *y, int n);

//===================================================================
// NOTE: all quaternions are scalar-first
//-------------------------------------------------------------------
// unitary quaternion
// input:   q--input quat
// output:  q--unitary quat
// return:
void unitQuat(REAL* q);

//-------------------------------------------------------------------
// normalize a quaternion
// input:   q--input quat
// output:  qN--normalized quat
// return:
void quatNormalize(REAL* q, REAL* qN);

//-------------------------------------------------------------------
// conver a quat to DCM
// input:   q--input quat
// output:  dcm--DCM
// return:
void quat2DCM(REAL *q, REAL *dcm);

//-------------------------------------------------------------------
// conver a DCM to a quat
// input:   a--input DCM
// output:  q--quat
// return:
void dcm2Quat(REAL *a, REAL *q);

//-------------------------------------------------------------------
// quaternion conjungate
// input:   q--input quat
// output:	qconj--conjungate
// return:
void quatConj(REAL *q, REAL *qconj);

//-------------------------------------------------------------------
// quat multiplication
// input:   q1--input quat
//          q2--input quat
// output:  q--q1xq2
// return:
void quatMultiply(REAL* q1, REAL* q2, REAL* q);

//-------------------------------------------------------------------
// integration of a quaternion based on anguler velocity
// input:   q--input quat
//          w--angular velocity
//          t--integration step, s
// output:  q--quat after integration
// return:
void quatIntegrate(REAL* q, REAL* w, REAL t);
//void quatIntegrate2(REAL* q, REAL* w, REAL t);

//-------------------------------------------------------------------
// get quaternion corresponding to the rotation(w*t, rad)
// input:   w--angular velocity
//          t--integration step, s
// output:  pq--quaternion corresponding to w and t
// return:
void RotationQuat(REAL *w, REAL t, REAL *pq);

#if  0
//-------------------------------------------------------------------
// first order poly fitting
// input:   data--vector containing the data, data[0] is the oldest
//          n--vector length
// output:  a--vector containing two elements, y = a[0]x + a[1]
// return;
void PolyFit1(REAL *data, REAL *a, int n);
#endif

//-------------------------------------------------------------------
// 30\B5\E3\CF\DF\D0\D4\C4\E2\BA\CF
// input:   data--\B4洢\CA\FD\BEݵ\C4\CA\FD\D7飬data[0]\D7\EE\C0\CF
//          n--\CA\FD\D7鳤\B6\C8
// output:  a--2ά\CA\FD\D7飬y = a[0]x + a[1]
// return;
void PolyFit1_30(REAL *data, REAL *a, int n);

//-------------------------------------------------------------------
// modulus
// input:   x--
//          y--
// output:
// return:  x - y*floor(x/y)
REAL mod(REAL x, REAL y);


//-------------------------------------------------------------------
// sine (http://stackoverflow.com/questions/345085/how-do-trigonometric-functions-work/345117#345117)
// input:   x--
// output:
// return:  sin(x)
REAL FastSin(REAL x);

#endif // MY_MATH_H_INCLUDED
