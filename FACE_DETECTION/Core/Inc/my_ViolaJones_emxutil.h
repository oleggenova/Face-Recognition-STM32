/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: my_ViolaJones_emxutil.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 05-Mar-2022 17:43:35
 */

#ifndef MY_VIOLAJONES_EMXUTIL_H
#define MY_VIOLAJONES_EMXUTIL_H

/* Include Files */
#include "my_ViolaJones_types.h"
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
extern void emxEnsureCapacity_int32_T(emxArray_int32_T *emxArray, int oldNumel);

extern void emxFree_int32_T(emxArray_int32_T **pEmxArray);

extern void emxInit_int32_T(emxArray_int32_T **pEmxArray);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for my_ViolaJones_emxutil.h
 *
 * [EOF]
 */
