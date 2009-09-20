#ifndef ERROR_H
#define ERROR_H

/**
 * Symbolic names for all possible error codes.
 */
enum error_code
{
	OK = 0,       /** No error, everything is ok */
	E_ALIGN,      /** Address is not page aligned */
	E_NO_MEM,     /** Not enough memory */
	E_INVALID,    /** Invalid parameter */
	E_UNKNOWN,    /** Unknown error */
	E_NUM_ERRORS, /** Invalid error code */
};

/**
 * Holds the last error that was set with seterr.
 */
extern int last_error;

/**
 * Returns the last error.
 * @return Error code
 */
int geterr(void);

/**
 * Sets the error code for the last error.
 * If the code is lower than 0 or bigger then E_NUM_ERRORS
 * it is changed to E_UNKNOWN.
 * @param err Error code
 */
void seterr(int err);

#endif /*ERROR_H*/
