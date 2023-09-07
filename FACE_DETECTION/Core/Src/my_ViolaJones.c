/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: my_ViolaJones.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 05-Mar-2022 17:43:35
 */

/* Include Files */
#include "my_ViolaJones.h"
#include "CascadeObjectDetector.h"
#include "my_ViolaJones_data.h"
#include "my_ViolaJones_emxutil.h"
#include "my_ViolaJones_initialize.h"
#include "my_ViolaJones_internal_types.h"
#include "my_ViolaJones_types.h"
#include "CascadeClassifierCore_api.hpp"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Type Definitions */
#ifndef typedef_rtEqualityCheckInfo
#define typedef_rtEqualityCheckInfo
typedef struct {
  int nDims;
  int lineNo;
  int colNo;
  const char *fName;
  const char *pName;
} rtEqualityCheckInfo;
#endif /* typedef_rtEqualityCheckInfo */

#ifndef typedef_rtDoubleCheckInfo
#define typedef_rtDoubleCheckInfo
typedef struct {
  int lineNo;
  int colNo;
  const char *fName;
  const char *pName;
  int checkKind;
} rtDoubleCheckInfo;
#endif /* typedef_rtDoubleCheckInfo */

/* Variable Definitions */
static vision_CascadeObjectDetector faceDetector;

static bool faceDetector_not_empty;

/* Function Declarations */
static void d_rtErrorWithMessageID(const char *r, const char *aFcnName,
                                   int aLineNum);

static void e_rtErrorWithMessageID(const char *r, const char *aFcnName,
                                   int aLineNum);

static void rtAddSizeString(char *aBuf, const int aDim);

static void rtGenSizeString(const int *aDims, char *aBuf);

static void rtNonNegativeError(const double aPositive,
                               const rtDoubleCheckInfo *aInfo);

static void rtReportErrorLocation(const char *aFcnName, int aLineNo);

static void rtSubAssignSizeCheck(const int *aDims1, const int *aDims2,
                                 const rtEqualityCheckInfo *aInfo);

static double rt_roundd_snf(double u);

/* Function Definitions */
/*
 * Arguments    : const char *r
 *                const char *aFcnName
 *                int aLineNum
 * Return Type  : void
 */
static void d_rtErrorWithMessageID(const char *r, const char *aFcnName,
                                   int aLineNum)
{
  fprintf(stderr,
          "The %.*s method cannot be called after calling the release method.",
          4, r);
  fprintf(stderr, "\n");
  fprintf(stderr, "Error in %s (line %d)", aFcnName, aLineNum);
  fprintf(stderr, "\n");
  fflush(stderr);
  abort();
}

/*
 * Arguments    : const char *r
 *                const char *aFcnName
 *                int aLineNum
 * Return Type  : void
 */
static void e_rtErrorWithMessageID(const char *r, const char *aFcnName,
                                   int aLineNum)
{
  fprintf(stderr,
          "The %.*s method cannot be called on an initialized System object or "
          "after calling the release method.",
          5, r);
  fprintf(stderr, "\n");
  fprintf(stderr, "Error in %s (line %d)", aFcnName, aLineNum);
  fprintf(stderr, "\n");
  fflush(stderr);
  abort();
}

/*
 * Arguments    : char *aBuf
 *                const int aDim
 * Return Type  : void
 */
static void rtAddSizeString(char *aBuf, const int aDim)
{
  char b_dimStr[1024];
  char dimStr[1024];
  sprintf(&dimStr[0], "[%d]", aDim);
  if (strlen(&aBuf[0]) + strlen(&dimStr[0]) < 1024) {
    memcpy(&b_dimStr[0], &dimStr[0], 1024U * sizeof(char));
    strcat(aBuf, b_dimStr);
  }
}

/*
 * Arguments    : const int *aDims
 *                char *aBuf
 * Return Type  : void
 */
static void rtGenSizeString(const int *aDims, char *aBuf)
{
  int i;
  aBuf[0] = '\x00';
  for (i = 0; i < 2; i++) {
    rtAddSizeString(aBuf, aDims[i]);
  }
}

/*
 * Arguments    : const double aPositive
 *                const rtDoubleCheckInfo *aInfo
 * Return Type  : void
 */
static void rtNonNegativeError(const double aPositive,
                               const rtDoubleCheckInfo *aInfo)
{
  fprintf(stderr,
          "Value %g is not greater than or equal to zero.\nExiting to prevent "
          "memory corruption.",
          aPositive);
  fprintf(stderr, "\n");
  rtReportErrorLocation(aInfo->fName, aInfo->lineNo);
  fflush(stderr);
  abort();
}

/*
 * Arguments    : const char *aFcnName
 *                int aLineNo
 * Return Type  : void
 */
static void rtReportErrorLocation(const char *aFcnName, int aLineNo)
{
  fprintf(stderr, "Error in %s (line %d)", aFcnName, aLineNo);
  fprintf(stderr, "\n");
}

/*
 * Arguments    : const int *aDims1
 *                const int *aDims2
 *                const rtEqualityCheckInfo *aInfo
 * Return Type  : void
 */
static void rtSubAssignSizeCheck(const int *aDims1, const int *aDims2,
                                 const rtEqualityCheckInfo *aInfo)
{
  int i;
  int j;
  char b_dims1Str[1024];
  char b_dims2Str[1024];
  char dims1Str[1024];
  char dims2Str[1024];
  i = 0;
  j = 0;
  while ((i < 2) && (j < 2)) {
    while ((i < 2) && (aDims1[i] == 1)) {
      i++;
    }
    while ((j < 2) && (aDims2[j] == 1)) {
      j++;
    }
    if (((i < 2) || (j < 2)) &&
        ((i == 2) ||
         ((j == 2) || ((aDims1[i] != -1) &&
                       ((aDims2[j] != -1) && (aDims1[i] != aDims2[j])))))) {
      rtGenSizeString(aDims1, dims1Str);
      rtGenSizeString(aDims2, dims2Str);
      memcpy(&b_dims1Str[0], &dims1Str[0], 1024U * sizeof(char));
      memcpy(&b_dims2Str[0], &dims2Str[0], 1024U * sizeof(char));
      fprintf(stderr, "Subscripted assignment dimension mismatch: %s ~= %s.",
              b_dims1Str, b_dims2Str);
      fprintf(stderr, "\n");
      rtReportErrorLocation(aInfo->fName, aInfo->lineNo);
      fflush(stderr);
      abort();
    }
    i++;
    j++;
  }
}

/*
 * Arguments    : double u
 * Return Type  : double
 */
static double rt_roundd_snf(double u)
{
  double y;
  if (fabs(u) < 4.503599627370496E+15) {
    if (u >= 0.5) {
      y = floor(u + 0.5);
    } else if (u > -0.5) {
      y = u * 0.0;
    } else {
      y = ceil(u - 0.5);
    }
  } else {
    y = u;
  }
  return y;
}

/*
 * Arguments    : const unsigned char inGray[76800]
 * Return Type  : double
 */
double my_ViolaJones(const unsigned char inGray[76800])
{
  static rtDoubleCheckInfo emlrtDCI = {
      116, /* lineNo */
      48,  /* colNo */
      "cascadeClassifierBuildable/cascadeClassifier_detectMultiScale", /* fName
                                                                        */
      "C:\\Program "
      "Files\\MATLAB\\R2021b\\toolbox\\vision\\vision\\+vision\\+internal\\+"
      "buildable\\cascadeClassifierBuildable.m", /* pName */
      4                                          /* checkKind */
  };
  static rtEqualityCheckInfo emlrtECI = {
      -1,                               /* nDims */
      397,                              /* lineNo */
      13,                               /* colNo */
      "CascadeObjectDetector/stepImpl", /* fName */
      "C:\\Program "
      "Files\\MATLAB\\R2021b\\toolbox\\vision\\vision\\+"
      "vision\\CascadeObjectDetector.m" /* pName */
  };
  static rtRunTimeErrorInfo emlrtRTEI = {
      1,                           /* lineNo */
      1,                           /* colNo */
      "SystemCore/releaseWrapper", /* fName */
      "C:\\Program "
      "Files\\MATLAB\\R2021b\\toolbox\\shared\\system\\coder\\+matlab\\+"
      "system\\+coder\\SystemCore.p" /* pName */
  };
  static const short inSize[8] = {240, 320, 1, 1, 1, 1, 1, 1};
  static const short iv[8] = {240, 320, 1, 1, 1, 1, 1, 1};
  static const short iv1[8] = {240, 320, 1, 1, 1, 1, 1, 1};
  static const char ClassificationModel[123] =
      "C:\\Program "
      "Files\\MATLAB\\R2021b\\toolbox\\vision\\visionutilities\\classifierdata"
      "\\cascade\\haar\\haarcascade_frontalface_alt2.xml";
  void *ptrDetectedObj;
  emxArray_int32_T *bboxes_;
  emxArray_int32_T *varargout_1;
  double d;
  double rows;
  int MaxSize_[2];
  int MinSize_[2];
  int i;
  int num_bboxes;
  unsigned int u;
  int *bboxes__data;
  int *varargout_1_data;
  unsigned char uv[76800];
  bool exitg1;
  if (!isInitialized_my_ViolaJones) {
    my_ViolaJones_initialize();
  }
  /*  Kernel function for 'Face Detection on ARM Target using Code Generation'
   * example */
  /*  Instantiate system object */
  if (!faceDetector_not_empty) {
    faceDetector.ScaleFactor = 1.1;
    faceDetector.MergeThreshold = 4.0;
    faceDetector.isInitialized = 0;
    faceDetector.pCascadeClassifier = NULL;
    cascadeClassifier_construct(&faceDetector.pCascadeClassifier);
    if (faceDetector.isInitialized == 1) {
      faceDetector.TunablePropsChanged = true;
    }
    faceDetector.MinSize[0] = 20.0;
    faceDetector.MinSize[1] = 20.0;
    if (faceDetector.isInitialized == 1) {
      faceDetector.TunablePropsChanged = true;
    }
    faceDetector.MaxSize[0] = 240.0;
    faceDetector.MaxSize[1] = 320.0;
    cascadeClassifier_load(faceDetector.pCascadeClassifier,
                           &ClassificationModel[0]);
    c_CascadeObjectDetector_validat(&faceDetector);
    faceDetector.matlabCodegenIsDeleted = false;
    faceDetector_not_empty = true;
  }
  /*  Create uninitialized memory in generated code */
  /* outRGB = coder.nullcopy(inGray); */
  /*  Detect faces and create boundiong boxes around detected faces */
  if (faceDetector.isInitialized == 2) {
    d_rtErrorWithMessageID("step", emlrtRTEI.fName, emlrtRTEI.lineNo);
  }
  if (faceDetector.isInitialized != 1) {
    faceDetector.isSetupComplete = false;
    if (faceDetector.isInitialized != 0) {
      e_rtErrorWithMessageID("setup", emlrtRTEI.fName, emlrtRTEI.lineNo);
    }
    faceDetector.isInitialized = 1;
    for (i = 0; i < 8; i++) {
      faceDetector.inputVarSize[0].f1[i] = (unsigned int)iv[i];
    }
    c_CascadeObjectDetector_validat(&faceDetector);
    faceDetector.isSetupComplete = true;
    faceDetector.TunablePropsChanged = false;
  }
  if (faceDetector.TunablePropsChanged) {
    c_CascadeObjectDetector_validat(&faceDetector);
    faceDetector.TunablePropsChanged = false;
  }
  num_bboxes = 0;
  exitg1 = false;
  while ((!exitg1) && (num_bboxes < 8)) {
    if (faceDetector.inputVarSize[0].f1[num_bboxes] !=
        (unsigned int)iv1[num_bboxes]) {
      for (i = 0; i < 8; i++) {
        faceDetector.inputVarSize[0].f1[i] = (unsigned int)inSize[i];
      }
      exitg1 = true;
    } else {
      num_bboxes++;
    }
  }
  d = rt_roundd_snf(faceDetector.MinSize[0]);
  if (d < 2.147483648E+9) {
    if (d >= -2.147483648E+9) {
      i = (int)d;
    } else {
      i = MIN_int32_T;
    }
  } else if (d >= 2.147483648E+9) {
    i = MAX_int32_T;
  } else {
    i = 0;
  }
  MinSize_[0] = i;
  d = rt_roundd_snf(faceDetector.MaxSize[0]);
  if (d < 2.147483648E+9) {
    if (d >= -2.147483648E+9) {
      i = (int)d;
    } else {
      i = MIN_int32_T;
    }
  } else if (d >= 2.147483648E+9) {
    i = MAX_int32_T;
  } else {
    i = 0;
  }
  MaxSize_[0] = i;
  d = rt_roundd_snf(faceDetector.MinSize[1]);
  if (d < 2.147483648E+9) {
    if (d >= -2.147483648E+9) {
      i = (int)d;
    } else {
      i = MIN_int32_T;
    }
  } else if (d >= 2.147483648E+9) {
    i = MAX_int32_T;
  } else {
    i = 0;
  }
  MinSize_[1] = i;
  d = rt_roundd_snf(faceDetector.MaxSize[1]);
  if (d < 2.147483648E+9) {
    if (d >= -2.147483648E+9) {
      i = (int)d;
    } else {
      i = MIN_int32_T;
    }
  } else if (d >= 2.147483648E+9) {
    i = MAX_int32_T;
  } else {
    i = 0;
  }
  MaxSize_[1] = i;
  ptrDetectedObj = NULL;
  for (i = 0; i < 240; i++) {
    for (num_bboxes = 0; num_bboxes < 320; num_bboxes++) {
      uv[num_bboxes + 320 * i] = inGray[i + 240 * num_bboxes];
    }
  }
  d = rt_roundd_snf(faceDetector.MergeThreshold);
  if (d < 4.294967296E+9) {
    if (d >= 0.0) {
      u = (unsigned int)d;
    } else {
      u = 0U;
    }
  } else if (d >= 4.294967296E+9) {
    u = MAX_uint32_T;
  } else {
    u = 0U;
  }
  emxInit_int32_T(&bboxes_);
  num_bboxes = cascadeClassifier_detectMultiScale(
      faceDetector.pCascadeClassifier, &ptrDetectedObj, &uv[0], 240, 320,
      faceDetector.ScaleFactor, u, &MinSize_[0], &MaxSize_[0]);
  if (num_bboxes < 0) {
    rtNonNegativeError(num_bboxes, &emlrtDCI);
  }
  emxInit_int32_T(&varargout_1);
  i = bboxes_->size[0] * bboxes_->size[1];
  bboxes_->size[0] = num_bboxes;
  bboxes_->size[1] = 4;
  emxEnsureCapacity_int32_T(bboxes_, i);
  bboxes__data = bboxes_->data;
  cascadeClassifier_assignOutputDeleteBbox(ptrDetectedObj, &bboxes__data[0]);
  i = varargout_1->size[0] * varargout_1->size[1];
  varargout_1->size[0] = bboxes_->size[0];
  varargout_1->size[1] = 4;
  emxEnsureCapacity_int32_T(varargout_1, i);
  varargout_1_data = varargout_1->data;
  num_bboxes = bboxes_->size[0] * 4;
  for (i = 0; i < num_bboxes; i++) {
    varargout_1_data[i] = bboxes__data[i];
  }
  emxFree_int32_T(&bboxes_);
  MinSize_[0] = varargout_1->size[0];
  MinSize_[1] = 2;
  MaxSize_[0] = varargout_1->size[0];
  MaxSize_[1] = 2;
  rtSubAssignSizeCheck(&MinSize_[0], &MaxSize_[0], &emlrtECI);
  rows = varargout_1->size[0];
  /*  Limit the number of faces to be detected in an image.  insertShape */
  /*  requires that bbox signal must be bounded */
  /* assert(size(bbox, 1) < 10); */
  /*  Insert rectangle shape for bounding box */
  /* outRGB(:) = insertShape(inGray, 'Rectangle', bbox); */
  emxFree_int32_T(&varargout_1);
  return rows;
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void my_ViolaJones_free(void)
{
  if (!faceDetector.matlabCodegenIsDeleted) {
    faceDetector.matlabCodegenIsDeleted = true;
    if (faceDetector.isInitialized == 1) {
      faceDetector.isInitialized = 2;
      if (faceDetector.isSetupComplete) {
        cascadeClassifier_deleteObj(faceDetector.pCascadeClassifier);
      }
    }
  }
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void my_ViolaJones_init(void)
{
  faceDetector_not_empty = false;
  faceDetector.matlabCodegenIsDeleted = true;
}

/*
 * File trailer for my_ViolaJones.c
 *
 * [EOF]
 */
