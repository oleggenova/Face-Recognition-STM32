/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: my_ViolaJones_types.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 05-Mar-2022 17:43:35
 */

#ifndef MY_VIOLAJONES_TYPES_H
#define MY_VIOLAJONES_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
#ifndef struct_emxArray_int32_T
#define struct_emxArray_int32_T
struct emxArray_int32_T {
  int *data;
  int *size;
  int allocatedSize;
  int numDimensions;
  bool canFreeData;
};
#endif /* struct_emxArray_int32_T */
#ifndef typedef_emxArray_int32_T
#define typedef_emxArray_int32_T
typedef struct emxArray_int32_T emxArray_int32_T;
#endif /* typedef_emxArray_int32_T */

#endif
/*
 * File trailer for my_ViolaJones_types.h
 *
 * [EOF]
 */
