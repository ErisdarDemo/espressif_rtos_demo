/**************************************************************************************************/
/** @file     utils.c
 *  @brief    Common code utilities
 *  @details  x
 */


//************************************************************************************************//
//                                            INCLUDES                                            //
//************************************************************************************************//

//Standard Library Includes
#include <limits.h>
#include <string.h>
#include <stdio.h>


//************************************************************************************************//
//                                          PUBLIC FUNCTIONS                                      //
//************************************************************************************************// 
 
/**************************************************************************************************/
/** @fcn        int numPlaces (int n)
 *  @brief      determine number of characters in a printed number
 *  @details    x
 *
 *  @param    [in]  (int) n - number to evaluate for length
 */
/**************************************************************************************************/
int numPlaces (int n) {
    int r = 1;
    if (n < 0) n = (n == INT_MIN) ? INT_MAX: -n;
    while (n > 9) {
        n /= 10;
        r++;
    }
    return r;
}

