/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: my_ViolaJones_terminate.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 05-Mar-2022 17:43:35
 */

/* Include Files */
#include "my_ViolaJones_terminate.h"
#include "my_ViolaJones.h"
#include "my_ViolaJones_data.h"

/* Function Definitions */
/*
 * Arguments    : void
 * Return Type  : void
 */
void my_ViolaJones_terminate(void)
{
  my_ViolaJones_free();
  isInitialized_my_ViolaJones = false;
}

/*
 * File trailer for my_ViolaJones_terminate.c
 *
 * [EOF]
 */
