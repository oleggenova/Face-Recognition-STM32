/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: my_ViolaJones_internal_types.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 05-Mar-2022 17:43:35
 */

#ifndef MY_VIOLAJONES_INTERNAL_TYPES_H
#define MY_VIOLAJONES_INTERNAL_TYPES_H

/* Include Files */
#include "my_ViolaJones_types.h"
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_cell_wrap_3
#define typedef_cell_wrap_3
typedef struct {
  unsigned int f1[8];
} cell_wrap_3;
#endif /* typedef_cell_wrap_3 */

#ifndef typedef_vision_CascadeObjectDetector
#define typedef_vision_CascadeObjectDetector
typedef struct {
  bool matlabCodegenIsDeleted;
  int isInitialized;
  bool isSetupComplete;
  bool TunablePropsChanged;
  cell_wrap_3 inputVarSize[1];
  double MinSize[2];
  double MaxSize[2];
  double ScaleFactor;
  double MergeThreshold;
  void *pCascadeClassifier;
} vision_CascadeObjectDetector;
#endif /* typedef_vision_CascadeObjectDetector */

#ifndef typedef_rtRunTimeErrorInfo
#define typedef_rtRunTimeErrorInfo
typedef struct {
  int lineNo;
  int colNo;
  const char *fName;
  const char *pName;
} rtRunTimeErrorInfo;
#endif /* typedef_rtRunTimeErrorInfo */

#endif
/*
 * File trailer for my_ViolaJones_internal_types.h
 *
 * [EOF]
 */
