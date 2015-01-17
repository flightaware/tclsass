/*
 * tclsass.h -- Tcl Package for libsass
 *
 * Written by Joe Mistachkin.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TCLSASS_H_
#define _TCLSASS_H_

/*
 * NOTE: These are the public functions exported by this library.
 */

#ifndef PACKAGE_EXTERN
#define PACKAGE_EXTERN
#endif

PACKAGE_EXTERN int	Sass_Init(Tcl_Interp *interp);
PACKAGE_EXTERN int	Sass_SafeInit(Tcl_Interp *interp);
PACKAGE_EXTERN int	Sass_Unload(Tcl_Interp *interp, int flags);
PACKAGE_EXTERN int	Sass_SafeUnload(Tcl_Interp *interp, int flags);

#endif /* _TCLSASS_H_ */
