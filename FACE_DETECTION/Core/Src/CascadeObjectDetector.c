/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: CascadeObjectDetector.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 05-Mar-2022 17:43:35
 */

/* Include Files */
#include "CascadeObjectDetector.h"
#include "my_ViolaJones_internal_types.h"
#include "CascadeClassifierCore_api.hpp"
#include <stdio.h>

/* Function Declarations */
static void b_rtErrorWithMessageID(const char *aFcnName, int aLineNum);

static void rtErrorWithMessageID(const unsigned int u, const unsigned int u1,
                                 const char *aFcnName, int aLineNum);

/* Function Definitions */
/*
 * Arguments    : const char *aFcnName
 *                int aLineNum
 * Return Type  : void
 */
static void b_rtErrorWithMessageID(const char *aFcnName, int aLineNum)
{
  fprintf(stderr, "Expected MaxSize to be greater than MinSize.");
  fprintf(stderr, "\n");
  fprintf(stderr, "Error in %s (line %d)", aFcnName, aLineNum);
  fprintf(stderr, "\n");
  fflush(stderr);
  abort();
}

/*
 * Arguments    : const unsigned int u
 *                const unsigned int u1
 *                const char *aFcnName
 *                int aLineNum
 * Return Type  : void
 */
static void rtErrorWithMessageID(const unsigned int u, const unsigned int u1,
                                 const char *aFcnName, int aLineNum)
{
  fprintf(stderr,
          "Expected MinSize to be greater than or equal to [%d %d], which is "
          "the object size used to train the classification model.",
          u, u1);
  fprintf(stderr, "\n");
  fprintf(stderr, "Error in %s (line %d)", aFcnName, aLineNum);
  fprintf(stderr, "\n");
  fflush(stderr);
  abort();
}

/*
 * Arguments    : const vision_CascadeObjectDetector *obj
 * Return Type  : void
 */
void c_CascadeObjectDetector_validat(const vision_CascadeObjectDetector *obj)
{
  static rtRunTimeErrorInfo b_emlrtRTEI = {
      328,                                            /* lineNo */
      17,                                             /* colNo */
      "CascadeObjectDetector/validatePropertiesImpl", /* fName */
      "C:\\Program "
      "Files\\MATLAB\\R2021b\\toolbox\\vision\\vision\\+"
      "vision\\CascadeObjectDetector.m" /* pName */
  };
  static rtRunTimeErrorInfo emlrtRTEI = {
      312,                                            /* lineNo */
      17,                                             /* colNo */
      "CascadeObjectDetector/validatePropertiesImpl", /* fName */
      "C:\\Program "
      "Files\\MATLAB\\R2021b\\toolbox\\vision\\vision\\+"
      "vision\\CascadeObjectDetector.m" /* pName */
  };
  unsigned int b_originalWindowSize[2];
  unsigned int originalWindowSize[2];
  unsigned int a__1;
  int k;
  bool x[2];
  bool exitg1;
  bool y;
  originalWindowSize[0] = 0U;
  originalWindowSize[1] = 0U;
  a__1 = 0U;
  cascadeClassifier_getClassifierInfo(obj->pCascadeClassifier,
                                      &originalWindowSize[0], &a__1);
  x[0] = (obj->MinSize[0] < originalWindowSize[0]);
  x[1] = (obj->MinSize[1] < originalWindowSize[1]);
  y = false;
  k = 0;
  exitg1 = false;
  while ((!exitg1) && (k < 2)) {
    if (x[k]) {
      y = true;
      exitg1 = true;
    } else {
      k++;
    }
  }
  if (y) {
    originalWindowSize[0] = 0U;
    originalWindowSize[1] = 0U;
    a__1 = 0U;
    cascadeClassifier_getClassifierInfo(obj->pCascadeClassifier,
                                        &originalWindowSize[0], &a__1);
    b_originalWindowSize[0] = 0U;
    b_originalWindowSize[1] = 0U;
    a__1 = 0U;
    cascadeClassifier_getClassifierInfo(obj->pCascadeClassifier,
                                        &b_originalWindowSize[0], &a__1);
    rtErrorWithMessageID(originalWindowSize[0], b_originalWindowSize[1],
                         emlrtRTEI.fName, emlrtRTEI.lineNo);
  }
  x[0] = (obj->MinSize[0] >= obj->MaxSize[0]);
  x[1] = (obj->MinSize[1] >= obj->MaxSize[1]);
  y = false;
  k = 0;
  exitg1 = false;
  while ((!exitg1) && (k < 2)) {
    if (x[k]) {
      y = true;
      exitg1 = true;
    } else {
      k++;
    }
  }
  if (y) {
    b_rtErrorWithMessageID(b_emlrtRTEI.fName, b_emlrtRTEI.lineNo);
  }
}

/*
 * File trailer for CascadeObjectDetector.c
 *
 * [EOF]
 */
