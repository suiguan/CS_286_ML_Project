#include <stdio.h>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <string>
#include <fstream>
#include "hmm.h"

//#define USE_FIXED_DIGRAPH //enable this define to use a fixed A matrix from a pre-defined digraph
#ifdef USE_FIXED_DIGRAPH
#include "AMat.h"
#endif

#define MAX_INT 9223372036854775807

//#define GRAD_DESCENT //define this to use gradient descent to train
#define TEMP 1.0
#define LEARNING_RATE 1.0


#define USER_AGENT //define this to use UA feature for HMM. Otherwise, it will use path feature for HMM

#ifdef USER_AGENT
static const std::string ObserSet = "012345678"; //we have 9 different UA
std::string feaStr = "UserAgent";
#else
static const std::string ObserSet = "01234567"; //we have 8 different path
std::string feaStr = "Path";
#endif

HMM::HMM(int N, int M, int minIters, float epsilon)
{
   mN = N;
   mM = M;
   mMinIters = minIters;
   mEps = epsilon;
   mOldLogProb = -MAX_INT;

   mA = NULL;
   mB = NULL;
   mPI = NULL;

   mC = NULL;
   mAlpha = NULL;
   mBeta = NULL;
   mGamma = NULL;
   mDiGamma = NULL;

   mW = NULL;
   mV = NULL;
}
HMM::~HMM()
{
   freeTables();
}

void HMM::freeTables()
{
   if (mA) {
      free(mA);
      mA = NULL;
   }
   if (mB) {
      free(mB);
      mB = NULL;
   }
   if (mPI) {
      free(mPI);
      mPI = NULL;
   }
   if (mC) {
      free(mC);
      mC = NULL;
   }
   if (mAlpha) {
      free(mAlpha);
      mAlpha = NULL;
   }
   if (mBeta) {
      free(mBeta);
      mBeta = NULL;
   }
   if (mGamma) {
      free(mGamma);
      mGamma = NULL;
   }
   if (mDiGamma) {
      free(mDiGamma);
      mDiGamma = NULL;
   }
   if (mW) {
      free(mW);
      mW = NULL;
   }
   if (mV) {
      free(mV);
      mV = NULL;
   }
}

void HMM::printModel()
{
   printf("A = \n");
   for (int i = 0; i < mN; i++) {
      std::string msg = std::to_string(i) + ": ";
      for (int j = 0; j < mN; j++) {
         msg += std::to_string(*getA(i, j)) + " ";
      }
      printf("%s\n", msg.c_str());
   }
   printf("B = \n");
   for (int j = 0; j < mM; j++) {
      std::string msg = std::string(1, ObserSet[j]) + ": ";
      for (int i = 0; i < mN; i++) {
         msg += std::to_string(*getB(i, j)) + " ";
      }
      printf("%s\n", msg.c_str());
   }
   printf("PI = \n");
   std::string msg = "";
   for (int i = 0; i < mN; i++) {
      msg += std::to_string(*getPI(i)) + " ";
   }
   printf("%s\n", msg.c_str());
}

void HMM::fit(int* obserArr, int T)
{
   int iters = 0;
   float logProb = -MAX_INT;
   float diff = MAX_INT;
   setupTable(T);
   randomInit();
#ifndef _NO_MAIN
   printModel();
#endif
   while (iters < mMinIters || diff > mEps) {
      logProb = getScore(obserArr, T);
      backwardPass(obserArr, T);
      calcGammaDigamma(obserArr, T);
      reEstimateModel(obserArr, T);

      diff = std::abs(logProb - mOldLogProb); 
      mOldLogProb = logProb;

      //if (iters % 10 == 0) {
#ifndef _NO_MAIN
         printModel();
         printf("%d_score = %.3f\n========================================\n\n", iters, logProb);
#endif
      //}

      iters += 1;
   }

   printf("HMM trained\n");
#ifndef _NO_MAIN
   printModel();
#endif
   printf("%d_score = %.3f\n========================================\n\n", iters, logProb);
}

float HMM::getScore(int* obserArr, int T)
{
   forwardPass(obserArr, T);
   float logProb = 0;
   for (int t = 0; t < T; t++) { 
      float l = std::log(*(getC(t))); 
      if (!std::isnan(l)) logProb += l; 
      else logProb += -MAX_INT;
   }
   return -logProb;
}

void HMM::setupTable(int T)
{
   freeTables();

#ifndef USE_FIXED_DIGRAPH
   mA = (float*)malloc(mN*mN*sizeof(float));
   if (!mA) {
      printf("No memory!\n");
      exit(-1);
   }
#endif

   mB = (float*)malloc(mN*mM*sizeof(float));
   if (!mB) {
      printf("No memory!\n");
      exit(-1);
   }
   mPI = (float*)malloc(mN*sizeof(float));
   if (!mPI) {
      printf("No memory!\n");
      exit(-1);
   }

   mC = (float*)malloc(T*sizeof(float));
   if (!mC) {
      printf("No memory!\n");
      exit(-1);
   }
   mAlpha = (float*)malloc(T*mN*sizeof(float));
   if (!mAlpha) {
      printf("No memory!\n");
      exit(-1);
   }
   mBeta = (float*)malloc(T*mN*sizeof(float));
   if (!mBeta) {
      printf("No memory!\n");
      exit(-1);
   }
   mGamma = (float*)malloc(T*mN*sizeof(float));
   if (!mGamma) {
      printf("No memory!\n");
      exit(-1);
   }
   mDiGamma = (float*)malloc(T*mN*mN*sizeof(float));
   if (!mDiGamma) {
      printf("No memory!\n");
      exit(-1);
   }

   mW = (float*)malloc(mN*mN*sizeof(float));
   if (!mW) {
      printf("No memory!\n");
      exit(-1);
   }

   mV = (float*)malloc(mN*mM*sizeof(float));
   if (!mV) {
      printf("No memory!\n");
      exit(-1);
   }
}
float HMM::getRandVal(int k)
{
   float r = float(rand())/RAND_MAX;
   float v = 1.0/k; 
   float delta = 0.1 * v;
   return v + -delta + (r * 2 * delta);
} 

void HMM::normalizeArr(float* arr, int T)
{
   float sum = 0;
   for (int idx = 0; idx < T; idx++) sum += arr[idx];
   if (sum == 0) {
      printf("error! can't normalize all 0\n");
      exit(-1);
   }
   for (int idx = 0; idx < T; idx++) arr[idx] /= sum;
}
void HMM::randomInit()
{
   srand(time(NULL));

#ifndef USE_FIXED_DIGRAPH
   for (int idx = 0; idx < mN; idx++) {
      for (int jdx = 0; jdx < mN; jdx++) {
         *getA(idx, jdx) = getRandVal(mN);

         //init weights
         float r = float(rand())/RAND_MAX;
         *getW(idx, jdx) = r*2.0 - 1.0;
      }
      normalizeArr(mA+(idx*mN), mN);
   }
#endif

   for (int idx = 0; idx < mN; idx++) {
      for (int jdx = 0; jdx < mM; jdx++) {
         *getB(idx, jdx) = getRandVal(mM);
         //init weights
         float r = float(rand())/RAND_MAX;
         *getV(idx, jdx) = r*2.0 - 1.0;
      }
      normalizeArr(mB+(idx*mM), mM);
   }
   for (int idx = 0; idx < mN; idx++) {
      *getPI(idx) = getRandVal(mN);
   }
   normalizeArr(mPI, mN);
} 

void HMM::forwardPass(int* obserArr, int T)
{
   //compute a0[i]
   *getC(0) = 0;
   for (int idx = 0; idx < mN; idx++) {
      float _a = (*getPI(idx)) * (*getB(idx, obserArr[0])); 
      *getAlpha(0, idx) = _a;
      *getC(0) = (*getC(0)) + _a;
   }

   //scale the a0(i)
   if (*getC(0) != 0) *getC(0) = 1.0 / (*getC(0));
   for (int idx = 0; idx < mN; idx++) {
      *getAlpha(0, idx) = (*getAlpha(0, idx)) * (*getC(0));
   }

   //compute at(i)
   for (int t = 1; t < T; t++) {
      *getC(t) = 0;
      for (int idx = 0; idx < mN; idx++) {
         *getAlpha(t, idx) = 0;
         for (int jdx = 0; jdx < mN; jdx++) {
            *getAlpha(t, idx) = (*getAlpha(t, idx)) +  ((*getAlpha(t-1, jdx)) * (*getA(jdx, idx)));
         }
         *getAlpha(t, idx) = (*getAlpha(t, idx)) * (*getB(idx, obserArr[t]));
         *getC(t) = (*getC(t)) + (*getAlpha(t, idx));
      }

      //scale at(i)
      if (*getC(t) != 0) *getC(t) = 1.0 / (*getC(t));
      for (int idx = 0; idx < mN; idx++) {
         *getAlpha(t, idx) = (*getAlpha(t, idx)) * (*getC(t));
      }
   }
}

void HMM::backwardPass(int* obserArr, int T)
{
   //let beta_t-1(i) = 1 scaled by cT-1
   for (int idx = 0; idx < mN; idx++) {
      *getBeta(T-1, idx) = *getC(T-1);
   }

   //beta-pass
   for (int t = T-2; t > 0; t--) {
      for (int idx = 0; idx < mN; idx++) {
         *getBeta(t, idx) = 0;
         for (int jdx = 0; jdx <mN; jdx++) {
            *getBeta(t, idx) += (*getA(idx, jdx) * (*getB(jdx, obserArr[t+1])) * (*getBeta(t+1, jdx)));
         }

         //scale beta_ti with same scale factor as a_ti
         *getBeta(t, idx) = (*getBeta(t, idx)) * (*getC(t));
      }
   }
}

void HMM::calcGammaDigamma(int* obserArr, int T)
{
   for (int t = 0; t < T-1; t++) {
      for (int idx = 0; idx < mN; idx++) {
         *getGamma(t, idx) = 0;
         for (int jdx = 0; jdx < mN; jdx++) {
            //No need to normalize since using scaled alpha and beta 
            *getDiGamma(t, idx, jdx) = (*getAlpha(t, idx)) * (*getA(idx, jdx)) * (*getB(jdx, obserArr[t+1])) * (*getBeta(t+1, jdx));
            *getGamma(t, idx) = (*getGamma(t, idx)) + (*getDiGamma(t, idx, jdx));
         }
      }
   }

   //special case for gamma_t-1(i)
   //No need to normalize since using scaled alpha and beta 
   for (int idx = 0; idx < mN; idx++) *getGamma(T-1, idx) = *getAlpha(T-1, idx);
  
}

void HMM::reEstimateModel(int* obserArr, int T)
{
   //re-estimate PI
   for (int idx = 0; idx < mN; idx++) *getPI(idx) = *getGamma(0, idx);

#ifndef GRAD_DESCENT

#ifndef USE_FIXED_DIGRAPH
   //re-estimate A
   for (int idx = 0; idx < mN; idx++) {
      for (int jdx = 0; jdx < mN; jdx++) {
         float numer = 0;
         float denom = 0;
         for (int t = 0; t < T-1; t++) {
            numer += *getDiGamma(t, idx, jdx);
            denom += *getGamma(t, idx);
         }
         if (numer == 0) *getA(idx, jdx) = 0;
         else *getA(idx, jdx) = numer / denom;
      }
   }
#endif

   //re-estimate B
   for (int idx = 0; idx < mN; idx++) {
      for (int jdx = 0; jdx < mM; jdx++) {
         float numer = 0;
         float denom = 0;
         for (int t = 0; t < T; t++) {
            if (obserArr[t] == jdx) numer += *getGamma(t, idx);
            denom += *getGamma(t, idx);
         }
         if (numer == 0) *getB(idx, jdx) = 0;
         else *getB(idx, jdx) = numer / denom;
      }
   }

#else

   float _C = 0;
   for (int t = 0; t < T; t++) {
      _C += log(*getC(t));
   }
   float aOverC = LEARNING_RATE / _C; 

   //update weights
   for (int idx = 0; idx < mN; idx++) {
      for (int jdx = 0; jdx < mN; jdx++) {
         float _Aij = 0;
         for (int t = 0; t < T-1; t++) {
            _Aij += *getDiGamma(t,idx,jdx);
         }
         float _Ai = 0;
         for (int t = 0; t < T-1; t++) {
            _Ai += *getGamma(t,idx);
         }
         *getW(idx, jdx) += aOverC*(_Aij - (_Ai*(*getA(idx, jdx))));
      }
   }

   for (int idx = 0; idx < mN; idx++) {
      for (int jdx = 0; jdx < mM; jdx++) {
         float _Bij = 0;
         for (int t = 0; t < T; t++) {
            if (obserArr[t] == jdx) _Bij += *getGamma(t,idx);
         }
         float _Bi = 0;
         for (int t = 0; t < T; t++) {
            _Bi += *getGamma(t,idx);
         }
         *getV(idx, jdx) += aOverC*(_Bij - (_Bi*(*getB(idx, jdx))));
      }
   }

   //re-estimate A
   for (int idx = 0; idx < mN; idx++) {
      float s = 0.0;
      for (int jdx = 0; jdx < mN; jdx++) {
         s += exp(TEMP*(*getW(idx,jdx)));
      }
      for (int jdx = 0; jdx < mN; jdx++) {
         *getA(idx,jdx) = exp(TEMP*(*getW(idx,jdx))) / s;
      }
   }

   //re-estimate B
   for (int idx = 0; idx < mN; idx++) {
      float s = 0.0;
      for (int jdx = 0; jdx < mM; jdx++) {
         s += exp(TEMP*(*getV(idx,jdx)));
      }
      for (int jdx = 0; jdx < mM; jdx++) {
         *getB(idx,jdx) = exp(TEMP*(*getV(idx,jdx))) / s;
      }
   }

#endif
}


float* HMM::getA(int i, int j)
{
#ifndef USE_FIXED_DIGRAPH
   return mA + (i*mN) + j;
#else
   return &DigraphAMat[i][j];
#endif
}
float* HMM::getB(int i, int j)
{
   return mB + (i*mM) + j;
}
float* HMM::getPI(int i)
{
   return mPI + i; 
}

float* HMM::getC(int t)
{
   return mC + t; 
}
float* HMM::getAlpha(int t, int i)
{
   return mAlpha + (t*mN) + i; 
}
float* HMM::getBeta(int t, int i)
{
   return mBeta + (t*mN) + i;
}
float* HMM::getGamma(int t, int i)
{
   return mGamma + (t*mN) + i;
}
float* HMM::getDiGamma(int t, int i, int j)
{
   return mDiGamma + (t*mN*mN) + (i*mN) + j;
}

float* HMM::getW(int i, int j)
{
   return mW + (i*mN) + j;
}
float* HMM::getV(int i, int j)
{
   return mV + (i*mM) + j;
}

//predict the simple shift key assume 
//HHM is trained with: A is fixed digraph matrix, N = M = 26
static int predictMapping(HMM* hmm) {
   //this is the group truth key to be compared
   //e.g., gtKey[0] = 4, then 'a'(0) maps to 'e'(4)
   static const int gtKey[26] = {4, 9, 21, 6, 25, 23, 13, 8, 1, 7, 15, 22, 18, 3, 17, 16, 0, 20, 12, 5, 2, 11, 14, 24, 10, 19};

   int cor = 0;
   for (int i = 0; i < 26; i++) {
      float maxP = 0.0;
      int maxJ = -1;
      for (int j = 0; j < 26; j++) {
         if (*hmm->getB(j, i) >= maxP) {
            maxP = *hmm->getB(j, i);
            maxJ = j;
         }
      }
      if (gtKey[maxJ] == i) cor++;
   }
   printf("cor = %d\n", cor);
   return cor;
}

//This train/test data is generated using "python ../DATA/createCArr.py <dir>"
//make sure top of the ObserSet also changed accordingly.
#ifdef USER_AGENT
static int m1[384] = {0,1,2,3,4,5,4,0,1,1,6,4,4,2,0,7,7,4,3,5,7,6,6,0,3,1,0,4,6,7,7,8,3,4,4,5,7,8,4,7,1,4,3,6,0,2,0,8,7,0,2,7,6,2,8,6,5,0,2,8,6,7,8,7,5,1,0,7,8,7,7,1,0,4,1,5,3,7,1,2,5,6,8,0,6,0,6,2,0,4,5,6,6,2,3,3,7,2,5,4,8,4,0,8,5,3,8,3,4,2,2,1,0,2,6,6,1,2,8,4,7,4,8,3,8,6,5,4,4,0,7,0,3,2,0,2,6,6,4,1,4,8,4,3,3,2,2,2,8,0,5,8,4,8,6,1,6,3,0,4,8,7,5,0,4,5,0,7,4,2,6,8,2,2,7,6,3,6,6,8,7,2,7,4,8,6,2,4,5,0,2,8,2,1,1,6,6,0,2,5,1,5,1,1,7,4,6,5,8,3,5,5,5,3,7,3,7,1,6,3,4,0,0,6,5,3,8,2,2,2,1,8,6,5,4,3,4,6,1,8,0,8,3,2,0,1,5,7,7,8,5,4,3,1,0,4,8,3,6,7,4,1,1,3,5,0,1,4,4,5,1,4,6,7,2,7,3,7,1,2,5,2,0,4,6,5,3,8,1,6,7,5,4,4,2,8,8,8,0,8,0,2,3,8,5,3,6,7,0,1,0,2,4,3,4,1,5,4,1,4,0,2,7,6,7,6,5,4,7,3,0,0,6,6,1,8,3,4,7,7,4,5,3,6,4,7,6,4,1,7,4,3,2,0,8,2,8,0,0,6,2,7,7,2,6,6,8,0,5,0,7,8,8,1,7,7,1,7,7,5,3,1,5,4};
static int m2[384] = {0,7,8,2,1,8,6,6,5,0,0,0,6,4,5,3,3,2,3,2,5,2,5,3,2,8,2,1,8,0,4,8,2,4,3,0,4,4,6,6,1,6,8,8,4,7,2,6,5,8,7,0,3,7,6,0,4,0,4,4,6,2,1,4,4,8,2,3,3,2,0,8,0,2,8,8,4,5,6,8,1,3,4,5,6,6,0,2,7,5,2,7,2,6,3,4,6,2,7,7,4,8,6,8,6,2,4,8,1,0,2,5,6,2,5,6,2,1,1,1,5,0,6,1,7,8,7,0,4,3,8,3,6,0,5,3,6,3,7,5,6,4,5,0,1,7,2,8,3,3,2,5,4,6,2,1,8,8,1,6,0,4,2,8,4,0,1,7,3,3,7,0,4,5,1,5,1,4,4,1,7,3,5,6,8,5,0,8,5,1,4,4,1,4,2,6,7,7,7,2,3,1,2,5,0,4,5,6,3,8,4,1,7,5,2,6,8,4,8,0,8,2,8,0,3,8,3,5,7,6,2,1,8,3,1,8,8,3,5,8,3,0,6,3,1,1,2,3,6,1,7,0,2,0,5,1,5,0,7,6,4,1,7,6,7,0,2,8,6,1,2,4,7,3,3,5,5,1,6,0,1,8,6,8,0,6,3,4,6,3,3,5,3,8,6,2,8,0,3,7,0,3,2,8,1,5,0,6,2,7,2,2,8,1,7,6,7,2,1,7,0,1,6,7,7,5,2,2,0,6,5,7,6,4,6,8,7,0,4,1,2,8,4,0,5,1,6,2,5,1,1,0,8,2,3,6,5,7,6,0,1,4,1,7,6,5,7,4,2,8,1,4,3,2,1,1,2,3,6,2,4,2,5,3};
static int m3[384] = {0,3,2,6,8,1,5,2,5,8,7,6,4,6,6,5,7,8,4,8,3,7,4,2,3,5,3,7,5,5,6,6,5,4,2,4,7,3,3,0,0,7,2,0,8,2,8,0,0,6,5,1,6,0,6,0,6,5,7,1,5,7,6,8,4,6,1,0,0,2,1,1,2,5,3,1,0,1,6,6,8,7,6,2,3,2,2,2,2,3,1,7,4,4,0,7,7,6,1,8,2,0,1,4,8,0,6,8,5,7,8,3,7,4,1,0,8,4,8,4,7,1,4,2,5,6,0,8,1,4,8,7,5,1,0,1,2,8,7,8,6,3,4,1,3,2,0,0,7,8,7,7,1,1,5,8,8,3,1,8,3,8,5,2,0,3,6,1,3,2,1,7,6,2,0,1,7,1,6,7,0,2,2,4,3,6,1,8,4,5,1,2,0,7,1,1,6,6,7,7,2,8,0,6,2,7,1,2,3,4,5,7,6,3,3,1,5,6,0,3,6,8,4,3,6,3,8,3,6,5,2,0,3,2,7,8,3,5,6,7,2,2,2,7,1,6,8,7,2,1,0,7,6,7,5,0,8,5,6,4,1,6,0,1,5,0,6,1,8,2,3,7,5,6,0,1,1,4,7,6,8,1,5,3,1,2,4,2,1,2,3,2,2,3,0,3,6,8,8,7,2,4,4,7,2,6,8,5,4,4,5,7,8,4,8,4,2,3,3,6,5,5,5,5,7,0,5,8,3,2,0,0,6,6,4,4,2,4,7,3,2,8,4,8,1,5,1,6,3,1,6,3,0,6,8,4,8,8,8,4,8,8,6,3,0,7,5,1,8,8,2,4,0,7,0,1,4,8,3,5,8,1,2,0};
static int m4[384] = {0,6,7,1,5,6,8,8,3,2,0,6,0,4,7,0,3,7,5,5,6,1,4,1,0,8,6,2,2,2,0,6,4,6,7,4,5,7,0,4,3,3,6,3,4,0,8,8,7,0,7,1,4,2,5,3,5,8,8,1,5,5,3,5,4,3,1,8,1,7,7,2,5,0,2,6,2,6,5,1,7,3,8,4,1,1,2,6,1,0,0,5,3,2,1,7,5,4,7,8,1,2,2,6,1,6,5,1,7,5,7,1,8,0,1,8,3,1,6,5,0,2,7,4,2,6,2,6,5,5,2,4,0,2,8,4,2,5,5,3,7,4,8,2,8,1,4,1,8,3,4,6,3,8,0,3,4,6,2,5,5,5,4,7,2,8,7,8,6,2,7,8,6,2,5,2,8,5,4,3,2,3,4,6,3,3,2,0,1,3,7,4,8,2,7,1,3,1,4,2,7,3,4,2,1,8,7,2,6,3,0,0,5,6,0,2,5,2,6,1,1,3,8,8,3,7,8,5,2,3,0,6,4,4,4,2,2,3,1,7,4,2,6,5,8,1,3,8,1,6,3,0,8,4,4,3,8,6,4,2,8,6,8,8,8,7,8,8,0,1,2,5,4,7,1,4,8,0,1,3,0,5,7,0,1,5,6,6,8,8,3,2,6,0,7,4,7,3,0,5,0,5,6,1,0,4,1,1,0,2,6,2,2,8,2,6,5,4,7,6,0,4,7,3,7,0,4,3,4,3,8,7,6,0,1,2,3,5,8,5,8,5,8,3,1,5,5,7,2,5,2,0,7,3,1,2,7,5,6,1,1,4,1,8,6,1,1,2,1,4,5,0,5,3,1,0,7,6,4,1,8,2,1,7};
static int m5[384] = {6,7,8,0,5,1,7,1,3,5,8,6,1,0,7,6,2,4,6,2,2,5,5,2,4,2,8,4,0,2,5,2,4,3,4,7,5,5,8,8,1,8,4,1,3,3,0,6,3,8,8,4,2,6,7,5,8,2,5,7,2,6,5,2,7,4,2,5,8,4,5,3,4,3,6,3,3,2,2,8,4,1,7,6,3,2,2,8,4,7,1,1,3,2,7,4,2,4,3,1,0,8,2,6,7,1,6,5,5,4,5,3,7,5,5,8,2,3,3,8,4,4,3,0,7,8,7,8,6,6,7,3,8,6,1,4,8,8,6,1,8,0,0,5,0,5,3,0,3,1,4,4,1,7,2,8,6,8,7,2,4,1,8,2,1,3,8,5,8,2,0,5,6,8,5,1,3,8,7,4,5,6,3,1,5,4,0,6,7,8,4,1,8,7,2,2,1,4,8,5,2,1,3,1,2,5,6,8,5,5,8,1,5,3,4,8,7,1,6,8,0,8,5,0,3,5,3,8,8,1,6,4,1,3,1,8,3,5,3,1,4,1,8,5,0,3,8,6,5,3,7,0,3,1,0,8,0,8,3,5,5,0,3,6,1,8,6,3,1,4,1,3,5,8,3,0,8,4,3,6,5,8,6,1,5,3,7,3,0,4,1,0,0,2,1,5,1,0,6,8,0,5,5,3,5,0,0,3,5,7,0,8,0,3,6,6,6,4,1,3,5,8,2,1,3,4,5,1,1,6,5,2,0,2,8,0,0,5,5,3,0,5,0,3,6,7,0,0,8,6,6,3,7,4,1,3,5,8,4,3,6,2,5,4,6,2,3,5,4,2,8,2,0,1,2,1,5,1,0,6,7,4,7,6};
static int b1[173] = {1,1,1,1,4,4,4,4,4,4,7,7,7,7,1,1,1,1,1,7,7,7,7,1,1,1,1,7,7,7,7,1,1,1,1,1,1,1,1,1,1,7,7,7,7,7,7,1,1,1,1,1,1,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,1,1,7,4,4,1,1,1,1,4,4,4,4,4,7,7,7,4,7,7,7,1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,8,8,8,8,8,8,8,8,8,3,3,3,3,3,3,3,3,3,3,3,3,3,6,3,6,3,6,6,3,6,6,6,6};
static int b2[45] = {5,5,5,3,3,3,2,5,3,2,2,3,3,3,3,3,3,3,3,5,0,2,2,2,0,0,0,3,5,5,5,0,5,2,2,0,0,3,2,2,1,3,3,3,0};
static int b3[106] = {3,3,1,5,3,5,5,1,1,3,5,3,5,1,3,5,5,3,1,1,5,5,5,3,3,5,3,3,1,3,1,1,1,1,3,5,5,5,5,3,3,5,3,5,1,1,5,5,1,3,0,0,5,1,3,3,0,3,1,3,5,1,0,5,3,1,1,5,5,0,0,0,0,1,1,1,1,1,5,3,3,3,3,3,3,3,3,1,1,1,5,0,5,5,1,3,1,3,1,3,3,0,0,0,2,2};
static int b4[125] = {5,1,3,3,1,1,1,3,5,0,0,5,3,3,0,1,1,1,5,5,3,3,0,3,0,3,3,3,3,3,3,5,3,3,3,0,3,3,3,1,3,3,3,3,5,3,3,3,3,3,3,1,3,0,3,3,3,3,3,3,3,0,3,3,1,0,3,0,3,5,3,0,3,3,3,3,1,1,1,5,3,3,3,3,3,3,3,3,3,3,3,1,3,1,1,3,3,5,5,5,5,5,3,1,1,5,5,5,1,0,3,0,0,3,5,1,3,0,1,1,1,5,5,3,3};
static int b5[93] = {1,0,0,5,3,3,3,3,3,5,5,1,5,5,5,3,0,3,3,1,1,1,3,3,0,2,2,2,2,3,3,2,5,2,2,2,0,0,2,2,2,2,5,5,3,1,3,0,0,0,2,2,2,2,0,3,3,1,1,1,0,2,5,3,2,2,2,1,1,5,5,5,3,3,3,1,2,0,0,0,0,0,2,2,2,3,3,3,2,2,2,2,2};
static const int M = 9;

//==========
#else
//This is the path observation sequence ==========
static int m1[384] = {0,1,2,3,0,4,2,1,2,5,6,2,2,0,1,5,6,3,3,5,1,0,6,0,0,2,0,7,7,4,1,7,1,0,0,4,4,6,7,2,4,0,5,0,4,1,4,0,5,6,3,0,7,7,3,5,0,2,5,5,5,4,4,3,7,1,3,3,7,2,3,2,0,3,3,6,6,7,6,0,4,5,7,4,3,5,6,7,7,2,3,7,5,0,0,7,1,4,4,1,6,2,3,7,6,3,5,3,3,2,7,3,2,1,0,3,0,0,0,7,3,0,0,6,0,3,4,0,4,2,3,2,4,1,1,5,2,2,6,3,1,5,2,0,0,1,6,0,7,7,5,0,5,0,1,2,5,7,2,5,4,1,6,6,1,1,0,5,4,2,2,0,1,6,0,0,1,1,0,6,1,5,7,4,3,3,7,4,6,2,2,7,0,0,3,6,6,0,6,2,5,7,6,5,2,3,7,3,6,4,7,5,5,2,2,7,7,1,0,4,5,3,5,7,1,4,3,0,1,2,2,7,6,3,3,6,0,1,3,7,0,4,4,3,1,3,1,0,0,1,0,4,3,3,0,5,5,6,1,5,5,3,5,2,7,3,7,1,3,5,4,3,3,3,1,3,2,0,0,7,6,3,4,2,1,5,1,2,7,5,3,3,5,0,1,2,4,1,0,5,4,7,2,6,3,4,3,6,0,1,1,2,2,3,0,2,4,2,5,2,1,0,1,0,5,6,5,3,6,3,0,0,0,6,2,7,0,7,4,4,0,4,1,7,0,1,0,7,4,2,0,5,1,4,6,3,0,4,6,7,7,0,5,5,5,5,3,2,0,3,4,5,4,1,3,3,2,2,3,7,6,3,6,3};
static int m2[384] = {0,7,7,0,6,7,5,3,4,4,5,7,5,2,3,0,7,7,3,0,4,4,6,3,7,7,2,3,5,3,1,6,1,3,6,2,2,7,7,6,0,3,0,0,0,3,5,3,4,0,3,2,4,1,2,1,4,2,0,6,2,1,3,1,2,5,1,0,0,6,7,7,2,0,0,0,5,5,5,4,2,7,5,6,1,2,6,6,1,1,2,0,1,1,1,4,0,5,7,1,4,6,0,3,3,7,4,7,0,2,0,6,6,2,2,6,6,3,5,6,7,0,7,5,2,0,5,0,1,2,6,7,4,5,5,4,0,4,2,5,7,5,1,3,1,7,1,3,4,6,0,3,3,6,2,2,7,7,3,1,0,0,3,4,3,1,3,0,4,6,5,0,5,3,5,0,3,4,5,3,0,2,7,1,5,1,3,1,5,7,1,3,4,3,7,3,0,3,3,1,2,0,3,6,4,2,5,1,1,2,0,7,3,3,1,5,2,5,4,0,1,7,6,4,2,5,4,3,6,3,0,7,4,2,6,5,5,1,1,3,2,5,3,4,7,3,1,2,6,6,1,1,4,7,7,4,3,1,7,6,6,2,6,3,1,1,7,1,1,4,3,3,7,0,3,4,3,5,2,4,6,2,6,2,3,0,5,6,6,5,7,4,3,2,2,6,7,4,1,7,3,5,5,3,2,1,4,2,1,5,2,3,6,3,0,5,1,1,6,7,6,3,0,2,2,2,4,2,0,5,7,6,0,5,4,0,1,1,7,5,0,6,3,0,6,2,0,3,4,5,3,4,4,5,6,4,6,4,5,2,4,5,3,4,5,0,6,4,2,1,1,5,6,6,0,0,7,6,3,0,6,6,7,2};
static int m3[384] = {0,3,3,2,2,7,0,5,3,2,5,7,3,0,2,2,0,3,6,4,2,5,4,0,0,2,6,0,3,4,2,0,1,4,2,4,2,4,7,1,0,0,0,2,5,7,0,6,4,4,4,4,5,7,0,7,7,1,6,4,5,7,3,3,1,7,7,0,7,3,2,7,0,5,1,6,2,5,7,1,1,3,1,3,1,6,1,4,6,3,1,0,2,7,2,4,1,3,2,3,0,1,3,3,4,1,3,4,7,4,2,5,7,4,3,3,2,6,2,4,2,2,4,1,1,7,1,5,2,0,7,1,2,4,1,2,2,3,6,2,3,3,3,3,7,7,2,0,7,2,7,1,3,7,6,4,5,2,6,3,1,5,1,2,5,2,3,7,4,1,3,5,6,3,0,6,1,6,4,4,1,4,7,6,1,2,4,4,3,3,4,2,1,7,7,2,6,3,6,1,5,1,1,1,7,5,4,3,0,3,4,7,2,7,0,5,3,0,4,5,6,2,6,7,6,5,2,3,2,4,6,3,1,5,7,3,5,1,2,5,3,1,2,0,3,5,6,1,1,6,6,2,0,2,2,0,0,7,0,5,5,4,1,2,6,4,0,3,4,5,6,4,6,5,2,4,3,4,6,5,1,1,0,6,0,2,5,6,0,0,6,6,3,3,0,2,2,2,6,6,0,7,3,1,4,0,2,3,3,6,2,0,4,5,3,4,0,0,6,2,4,2,3,4,5,6,6,1,6,4,0,7,2,0,6,2,1,6,5,2,0,4,3,1,3,7,4,0,4,7,5,0,5,3,1,5,2,0,3,2,4,4,2,2,1,2,6,2,4,0,5,1,3,4,2,0,2,0,5,1,2,1,6,2};
static int m4[384] = {0,3,6,5,5,5,6,7,4,2,3,4,3,0,1,7,1,4,4,0,2,2,7,3,6,2,4,2,6,0,1,3,7,6,2,3,1,7,7,0,2,7,3,5,1,7,3,2,1,3,5,0,4,3,7,5,6,6,7,0,2,7,1,2,4,4,0,4,6,5,6,3,2,5,3,1,2,4,4,2,7,3,0,6,6,6,1,3,7,4,2,0,0,0,0,5,6,2,1,6,6,7,3,4,1,2,6,4,3,1,2,4,7,5,2,4,2,6,7,6,6,4,2,2,6,4,0,7,0,0,0,3,7,3,1,3,6,5,1,1,2,6,2,3,6,1,2,4,2,7,1,2,6,4,0,7,3,6,0,4,4,0,4,2,3,6,4,5,4,1,4,4,3,6,2,3,2,4,1,2,2,7,6,5,3,7,5,5,5,5,6,1,5,2,6,0,5,7,7,3,1,2,7,0,5,6,4,1,6,6,0,7,6,2,6,4,5,2,1,4,0,2,1,7,1,3,0,1,3,2,1,0,6,2,6,6,1,2,3,5,3,0,0,7,4,4,4,1,7,5,0,3,1,4,5,3,2,3,2,1,0,2,4,3,4,2,4,0,3,2,5,6,1,4,0,2,0,2,1,5,2,1,6,0,5,5,5,3,6,7,4,2,4,3,1,0,4,1,7,0,3,4,2,2,6,7,3,0,1,6,4,0,3,2,2,3,1,7,2,6,7,3,7,2,1,7,1,5,0,7,2,5,3,5,0,3,5,2,4,2,7,7,6,1,0,7,6,6,3,2,3,5,5,4,6,2,7,4,4,2,7,6,6,0,3,6,0,0,0,4,0,2,6,2,6,4,1,2,2,1,6,7,4,5};
static int m5[384] = {4,3,7,5,1,4,2,2,2,6,4,7,6,6,2,4,4,2,7,6,0,0,0,3,3,6,1,3,7,3,6,3,3,1,6,2,5,1,6,2,1,2,2,4,7,6,0,2,7,4,6,1,3,6,4,4,5,0,4,2,0,4,0,3,4,4,1,2,2,1,4,3,6,7,5,2,7,6,2,4,3,5,6,3,5,5,2,5,1,6,0,5,2,0,1,7,3,7,5,7,5,6,1,6,4,5,2,3,5,0,1,1,3,5,5,5,2,2,2,0,0,7,2,6,7,3,3,5,3,2,6,4,4,1,1,1,6,7,1,4,3,0,0,3,4,0,0,2,7,5,7,4,5,6,5,2,4,2,1,1,3,5,0,7,0,5,0,6,4,6,6,1,2,6,7,6,3,4,6,0,0,0,7,5,0,7,2,4,6,2,4,5,1,1,1,5,5,3,0,3,7,0,5,7,6,6,2,0,1,7,4,6,0,3,0,4,6,2,0,2,3,6,2,3,1,2,0,1,7,3,1,4,0,4,6,2,1,3,5,7,1,1,6,4,6,6,3,2,2,6,4,5,7,2,1,2,3,6,1,2,2,3,0,5,3,7,1,4,6,4,0,1,0,2,5,6,6,1,6,3,4,3,2,1,2,6,4,7,5,6,4,1,3,1,1,0,3,0,5,3,4,5,1,0,0,5,3,6,0,2,6,4,5,7,0,5,3,5,4,6,7,4,1,3,7,6,0,4,1,1,0,2,3,1,3,4,0,5,0,0,5,1,3,6,6,2,5,6,4,0,5,7,7,5,4,6,7,4,2,7,1,1,0,5,1,2,5,7,5,2,4,2,1,5,6,0,5,4,7,6,4,6,5,1};
static int b1[173] = {1,0,1,1,6,1,1,0,6,0,1,6,0,5,6,5,0,6,1,6,5,1,0,6,1,0,5,5,0,6,1,1,6,1,5,6,1,5,0,6,5,1,6,0,1,6,6,6,5,0,6,1,0,6,1,5,0,5,1,6,1,5,0,5,6,1,0,5,5,1,6,5,1,5,6,1,0,5,1,6,5,1,6,0,1,6,5,1,5,6,1,6,1,1,0,1,5,0,1,6,5,6,1,0,5,6,0,1,5,5,0,5,1,0,1,5,0,5,6,6,5,1,6,0,1,6,5,0,6,5,6,0,5,6,5,0,1,1,6,5,1,0,5,1,5,0,5,6,6,6,5,0,1,1,0,1,6,5,6,0,6,5,6,6,6,6,6,5,1,0,5,1,0};
static int b2[45] = {1,5,0,6,1,5,0,0,5,5,6,1,1,6,6,5,5,0,0,1,1,1,1,0,1,1,1,1,0,0,5,1,1,6,5,1,0,0,1,1,1,1,1,6,0};
static int b3[106] = {1,5,5,0,6,5,0,6,5,1,1,6,5,0,5,0,1,0,1,6,6,5,0,5,6,5,6,6,5,6,1,0,5,6,0,5,0,0,0,1,6,1,5,5,6,5,1,6,6,6,0,6,1,6,6,1,6,0,0,6,1,6,6,0,1,0,5,5,0,1,6,5,0,1,6,5,6,1,1,6,5,0,0,6,1,5,0,1,6,0,5,5,5,0,1,5,5,6,0,6,0,1,6,6,0,0};
static int b4[125] = {1,6,1,0,5,1,5,6,1,6,0,1,5,1,5,5,0,1,5,0,0,6,5,5,0,6,6,6,6,6,6,6,6,6,1,1,0,5,6,1,1,6,0,5,1,6,5,6,1,5,5,0,6,6,1,1,0,6,5,1,6,0,1,1,6,5,1,0,5,1,5,0,6,6,1,6,6,5,0,6,5,0,0,5,5,5,5,5,5,5,5,1,5,6,0,5,6,1,6,5,0,0,1,5,0,6,5,0,5,5,6,6,1,6,0,0,0,1,1,5,6,1,5,5,0};
static int b5[93] = {6,0,5,1,6,5,0,1,6,5,0,0,6,5,0,5,5,6,5,6,5,0,6,0,0,6,5,0,1,1,6,0,1,1,5,1,5,0,0,6,1,0,5,1,5,5,0,0,5,6,1,0,6,5,5,5,0,1,6,6,5,6,1,6,1,6,0,5,6,6,5,0,1,6,5,5,6,0,0,5,6,1,5,6,1,6,0,0,5,6,1,0,5};
static const int M = 8;

//==========
#endif

void dumpScores(HMM* hmm, std::string label, int* obsers, int total, int t, std::string dumpFile) //score consecutive t observations from the array
{
   std::fstream fs;
   fs.open (dumpFile, std::fstream::out | std::fstream::app);
   for (int k = 0; k + t <= total; k++) {
      float logScore = hmm->getScore(obsers+k, t);
      fs << label << "," << logScore << "\n";
   }
   fs.close();
}

#ifndef _NO_MAIN
int main(int argc, const char** argv) {
   if (argc != 7) {
      printf("Usage: %s <N> <minIters> <epsilon> <hmm score file> <train_test_file_prefix> <# observations for scores>\n", argv[0]);
      return -1;
   }
   const int N = std::stoi(argv[1]);
   const int minIters = std::stoi(argv[2]);
   const float epsilon = std::stof(argv[3]);
   std::string scoreFile = argv[4];
   std::string train_test_prefix = argv[5];
   const int numObsersScore = std::stoi(argv[6]);

   //5-fold cross validation
   for (int fold = 0; fold < 5; fold++) {

      int T = sizeof(m1) / 4; //4-bytes per integer 
      int* obsers = m1; 
      if (fold == 1) {
         T = sizeof(m2) / 4;
         obsers = m2; 
      }
      else if (fold == 2) {
         T = sizeof(m3) / 4;
         obsers = m3; 
      }
      else if (fold == 3) {
         T = sizeof(m4) / 4;
         obsers = m4; 
      }
      else if (fold == 4) {
         T = sizeof(m5) / 4;
         obsers = m5; 
      }
      //start training
      HMM* hmm = new HMM(N, M, minIters, epsilon);
      if (!hmm) {
         printf("no memory\n");
         return -3;
      }

#ifdef GRAD_DESCENT
      printf("start training HMM (using feature %s) for (fold %d) seq N = %d, M = %d, minIters = %d, eps = %.6f, T = %d, learning rate = %f, temp = %f\n", 
         feaStr.c_str(), fold, N, M, minIters, epsilon, T, LEARNING_RATE, TEMP);
#else
      printf("start training HMM (using feature %s) for (fold %d) seq N = %d, M = %d, minIters = %d, eps = %.6f, T = %d\n", 
         feaStr.c_str(), fold, N, M, minIters, epsilon, T);
#endif
      hmm->fit(obsers, T);

      //create scores
      printf("creating scores on file %s using consecutive %d observations\n", scoreFile.c_str(), numObsersScore);
      if (fold == 0) {
         dumpScores(hmm, "m", m2, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m3, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m4, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m5, T, numObsersScore, scoreFile); 
      }
      else if (fold == 1) {
         dumpScores(hmm, "m", m1, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m3, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m4, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m5, T, numObsersScore, scoreFile); 
      }
      else if (fold == 2) {
         dumpScores(hmm, "m", m1, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m2, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m4, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m5, T, numObsersScore, scoreFile); 
      }
      else if (fold == 3) {
         dumpScores(hmm, "m", m1, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m2, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m3, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m5, T, numObsersScore, scoreFile); 
      }
      else if (fold == 4) {
         dumpScores(hmm, "m", m1, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m2, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m3, T, numObsersScore, scoreFile); 
         dumpScores(hmm, "m", m4, T, numObsersScore, scoreFile); 
      }

      //score benign
      int total = sizeof(b1) / 4;
      dumpScores(hmm, "b", b1, total, numObsersScore, scoreFile); 
      total = sizeof(b2) / 4;
      dumpScores(hmm, "b", b2, total, numObsersScore, scoreFile); 
      total = sizeof(b3) / 4;
      dumpScores(hmm, "b", b3, total, numObsersScore, scoreFile); 
      total = sizeof(b4) / 4;
      dumpScores(hmm, "b", b4, total, numObsersScore, scoreFile); 
      total = sizeof(b5) / 4;
      dumpScores(hmm, "b", b5, total, numObsersScore, scoreFile); 

#ifdef USE_FIXED_DIGRAPH
      //int cor = predictMapping(hmm);
#endif

      //also save for 5 fold train/test for PCA, SVM, NN, etc...
      std::string fname1 = train_test_prefix + ".train1.txt";
      std::string fname2 = train_test_prefix + ".test1.txt";
      std::string fname3 = train_test_prefix + ".test1.txt";
      std::string fname4 = train_test_prefix + ".test1.txt";
      std::string fname5 = train_test_prefix + ".test1.txt";
      if (fold == 1) {
         fname1 = train_test_prefix + ".test2.txt";
         fname2 = train_test_prefix + ".train2.txt";
         fname3 = train_test_prefix + ".test2.txt";
         fname4 = train_test_prefix + ".test2.txt";
         fname5 = train_test_prefix + ".test2.txt";
      }
      else if (fold == 2) {
         fname1 = train_test_prefix + ".test3.txt";
         fname2 = train_test_prefix + ".test3.txt";
         fname3 = train_test_prefix + ".train3.txt";
         fname4 = train_test_prefix + ".test3.txt";
         fname5 = train_test_prefix + ".test3.txt";
      }
      else if (fold == 3) {
         fname1 = train_test_prefix + ".test4.txt";
         fname2 = train_test_prefix + ".test4.txt";
         fname3 = train_test_prefix + ".test4.txt";
         fname4 = train_test_prefix + ".train4.txt";
         fname5 = train_test_prefix + ".test4.txt";
      }
      else if (fold == 4) {
         fname1 = train_test_prefix + ".test5.txt";
         fname2 = train_test_prefix + ".test5.txt";
         fname3 = train_test_prefix + ".test5.txt";
         fname4 = train_test_prefix + ".test5.txt";
         fname5 = train_test_prefix + ".train5.txt";
      }
      total = sizeof(b1) / 4;
      printf("total b1 = %d\n", total);
      dumpScores(hmm, "m", m1, T, numObsersScore, fname1); 
      dumpScores(hmm, "b", b1, total, numObsersScore, fname1); 

      total = sizeof(b2) / 4;
      printf("total b2 = %d\n", total);
      dumpScores(hmm, "m", m2, T, numObsersScore, fname2); 
      dumpScores(hmm, "b", b2, total, numObsersScore, fname2); 

      total = sizeof(b3) / 4;
      printf("total b3 = %d\n", total);
      dumpScores(hmm, "m", m3, T, numObsersScore, fname3); 
      dumpScores(hmm, "b", b3, total, numObsersScore, fname3); 

      total = sizeof(b4) / 4;
      printf("total b4 = %d\n", total);
      dumpScores(hmm, "m", m4, T, numObsersScore, fname4); 
      dumpScores(hmm, "b", b4, total, numObsersScore, fname4); 

      total = sizeof(b5) / 4;
      printf("total b5 = %d\n", total);
      dumpScores(hmm, "m", m5, T, numObsersScore, fname5); 
      dumpScores(hmm, "b", b5, total, numObsersScore, fname5); 

      //clean up
      delete hmm;
   }

   printf("finished 5 fold cross validation HMM, feature used = %s\n", feaStr.c_str());
   return 0;
}
#endif



