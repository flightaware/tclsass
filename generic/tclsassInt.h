/*
 * tclsassInt.h -- Tcl Package for libsass
 *
 * Written by Joe Mistachkin.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TCLSASS_INT_H_
#define _TCLSASS_INT_H_

/*
 * NOTE: These "printf" formats are used for trace message formatting only.
 */

#define PACKAGE_HEX_FMT			"0x%X"
#define PACKAGE_PTR_FMT			"%p"

/*
 * NOTE: The PACKAGE_TRACE macro is used to report important diagnostics when
 *       other means are not available.  Currently, this macro is enabled by
 *       default; however, it may be overridden via the compiler command line.
 */

#ifndef PACKAGE_TRACE
  #ifdef _TRACE
    #define PACKAGE_TRACE(x)			printf x
  #else
    #define PACKAGE_TRACE(x)
  #endif
#endif

/*
 * NOTE: When the package is being compiled with the PACKAGE_DEBUG option
 *       enabled, we want to handle serious errors using either the Tcl_Panic
 *       function from the Tcl API or the printf function from the CRT.  Which
 *       of these functions gets used depends on the build configuration.  In
 *       the "Debug" build configuration, the Tcl_Panic function is used so
 *       that it can immediately abort the process.  In the "Release" build
 *       configuration, the printf function is used to report the error to the
 *       standard output channel, if available.  Currently, there are only two
 *       places where this macro is used and neither of them strictly require
 *       the process to be aborted; therefore, when the package is compiled
 *       without the PACKAGE_DEBUG option enabled, this macro does nothing.
 */

#ifdef PACKAGE_DEBUG
  #ifdef _DEBUG
    #define PACKAGE_PANIC(x)			Tcl_Panic x
  #else
    #define PACKAGE_PANIC(x)			printf x
  #endif
#else
  #define PACKAGE_PANIC(x)
#endif

/*
 * NOTE: Flag values for the <pkg>_Unload callback function (Tcl 8.5+).
 */

#if !defined(TCL_UNLOAD_DETACH_FROM_INTERPRETER)
  #define TCL_UNLOAD_DETACH_FROM_INTERPRETER	(1<<0)
#endif

#if !defined(TCL_UNLOAD_DETACH_FROM_PROCESS)
  #define TCL_UNLOAD_DETACH_FROM_PROCESS	(1<<1)
#endif

/*
 * HACK: This flag means that the <pkg>_Unload callback function is being
 *       called from the <pkg>_Init function to cleanup due to a package load
 *       failure.  This flag is NOT defined by Tcl and MUST NOT conflict with
 *       the unloading related flags defined by Tcl in "tcl.h".
 */

#if !defined(TCL_UNLOAD_FROM_INIT)
  #define TCL_UNLOAD_FROM_INIT			(1<<2)
#endif

/*
 * NOTE: This macro returns the number of elements in an array.
 */

#define ArraySize(X)				((int)(sizeof(X)/sizeof(X[0])))

/*
 * NOTE: This macro is used to slightly simplify the option/argument checking
 *       against various "well-known" option names and dictionary keys passed
 *       as constant strings.
 */

#define CheckString(len,arg,str) \
    (((len) == strlen((str))) && (strcmp((arg), (str)) == 0))

/*
 * NOTE: These are semi-generic function types, used for interfacing with
 *       various functions from the Tcl C API and the Sass C API.
 */

typedef int (fn_get_any) ();
typedef void (fn_set_any) ();

#endif /* _TCLSASS_INT_H_ */
