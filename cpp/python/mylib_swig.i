%module mylib
%{
/* Includes the header in the wrapper code */
#include "../mylib.h"
%}

/* Parse the header file to generate wrappers */
%include "../mylib.h"

