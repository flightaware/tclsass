/*
 * tclsass.c -- Tcl Package for libsass
 *
 * Written by Joe Mistachkin.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tcl.h"		/* NOTE: For public Tcl API. */
#include "sass_context.h"	/* NOTE: For public libsass API. */
#include "pkgVersion.h"		/* NOTE: Package version information. */
#include "tclsassInt.h"		/* NOTE: For private package API. */
#include "tclsass.h"		/* NOTE: For public package API. */

/*
 * NOTE: Private functions defined in this file.
 */

static void		SassExitProc(ClientData clientData);
static int		SassObjCmd(ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[]);
static void		SassObjCmdDeleteProc(ClientData clientData);

/*
 *----------------------------------------------------------------------
 *
 * Sass_Init --
 *
 *	This function initializes the package for the specified Tcl
 *	interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int Sass_Init(
    Tcl_Interp *interp)			/* Current Tcl interpreter. */
{
    int code = TCL_OK;
    Tcl_Command command;

    /*
     * NOTE: Make sure the Tcl interpreter is valid and then try to initialize
     *       the Tcl stubs table.  We cannot call any Tcl API unless this call
     *       succeeds.
     */

    if ((interp == NULL) || !Tcl_InitStubs(interp, PACKAGE_TCL_VERSION, 0)) {
	PACKAGE_TRACE(("Sass_Init: Tcl stubs were not initialized\n"));
	return TCL_ERROR;
    }

    /*
     * NOTE: Add our exit handler prior to performing any actions that need to
     *       be undone by it.  However, first delete it in case it has already
     *       been added.  If it has never been added, trying to delete it will
     *       be a harmless no-op.  This appears to be necessary to ensure that
     *       our exit handler has been added exactly once after this point.
     */

    Tcl_DeleteExitHandler(SassExitProc, NULL);
    Tcl_CreateExitHandler(SassExitProc, NULL);

    /*
     * NOTE: Create our command in the Tcl interpreter.
     */

    command = Tcl_CreateObjCommand(interp, COMMAND_NAME, SassObjCmd, interp,
	SassObjCmdDeleteProc);

    if (command == NULL) {
	Tcl_AppendResult(interp, "command creation failed\n", NULL);
	code = TCL_ERROR;
	goto done;
    }

    /*
     * NOTE: Store the token for the command created by this package.  This
     *       way, we can properly delete it when the package is being unloaded.
     */

    Tcl_SetAssocData(interp, PACKAGE_NAME, NULL, command);

    /*
     * NOTE: Finally, attempt to provide this package in the Tcl interpreter.
     */

    code = Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION);

done:
    /*
     * NOTE: If some step of loading the package failed, attempt to cleanup now
     *       by unloading the package, either from just this Tcl interpreter or
     *       from the entire process.
     */

    if (code != TCL_OK) {
	if (Sass_Unload(interp, TCL_UNLOAD_FROM_INIT) != TCL_OK) {
	    /*
	     * NOTE: We failed to undo something and we have no nice way of
	     *       reporting this failure; therefore, complain about it.
	     */

	    PACKAGE_PANIC(("Sass_Unload: failed via Sass_Init\n"));
	}
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Sass_SafeInit --
 *
 *	This function initializes the package for the specified safe
 *	Tcl interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int Sass_SafeInit(
    Tcl_Interp *interp)			/* Current Tcl interpreter. */
{
    return Sass_Init(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Sass_Unload --
 *
 *	This function unloads the package from the specified Tcl
 *	interpreter -OR- from the entire process.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int Sass_Unload(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    int flags)				/* Unload behavior flags. */
{
    int code = TCL_OK;
    int bShutdown = (flags & TCL_UNLOAD_DETACH_FROM_PROCESS);

    /*
     * NOTE: If we have a valid Tcl interpreter, try to get the token for the
     *       command added to it when the package was being loaded.  We need to
     *       delete the command now because the whole library may be unloading.
     */

    if (interp != NULL) {
	Tcl_Command command = Tcl_GetAssocData(interp, PACKAGE_NAME, NULL);

	if (command != NULL) {
	    if (Tcl_DeleteCommandFromToken(interp, command) != 0) {
		Tcl_AppendResult(interp, "command deletion failed\n", NULL);
		code = TCL_ERROR;
		goto done;
	    }
	}

	/*
	 * NOTE: Always delete our saved association data from the Tcl
	 *       interpreter because the Tcl_GetAssocData function does not
	 *       reserve any return value to indicate "failure" or "not found"
	 *       and calling the Tcl_DeleteAssocData function for association
	 *       data that does not exist is a harmless no-op.
	 */

	Tcl_DeleteAssocData(interp, PACKAGE_NAME);
    }

    /*
     * NOTE: Delete our exit handler after performing the actions that needed
     *       to be undone.  However, this should only be done if the package
     *       is actually being unloaded from the process; otherwise, none of
     *       the process-wide cleanup was done and it must be done later.  If
     *       this function is actually being called from our exit handler now,
     *       trying to delete our exit handler will be a harmless no-op.
     */

    if (bShutdown)
	Tcl_DeleteExitHandler(SassExitProc, NULL);

done:
    /*
     * NOTE: If possible, log this attempt to unload the package, including
     *       the saved package module handle, the associated module file name,
     *       the current Tcl interpreter, the flags, and the return code.
     */

    PACKAGE_TRACE(("Sass_Unload(interp = {"
	PACKAGE_PTR_FMT "}, flags = {" PACKAGE_HEX_FMT "}, code = {%d})\n",
	interp, flags, code));

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Sass_SafeUnload --
 *
 *	This function unloads the package from the specified safe Tcl
 *	interpreter -OR- from the entire process.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int Sass_SafeUnload(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    int flags)				/* Unload behavior flags. */
{
    return Sass_Unload(interp, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * SassExitProc --
 *
 *	Cleanup all the resources allocated by this package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void SassExitProc(
    ClientData clientData)		/* Not used. */
{
    if (Sass_Unload(NULL,
	    TCL_UNLOAD_DETACH_FROM_PROCESS) != TCL_OK)
    {
	/*
	 * NOTE: We failed to undo something and we have no nice way of
	 *       reporting this failure; therefore, complain about it.
	 */

	PACKAGE_PANIC(("Sass_Unload: failed via SassExitProc\n"));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SassObjCmd --
 *
 *	Handles the command(s) added by this package.  This command is
 *	aware of safe Tcl interpreters.  For safe Tcl interpreters, all
 *	sub-commands are allowed.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	One or more libsass library functions may be called, resulting
 *	in whatever effects they may have.
 *
 *----------------------------------------------------------------------
 */

static int SassObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current Tcl interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* The arguments. */
{
    int code = TCL_OK;
    int option;

    static CONST char *cmdOptions[] = {
	"compile", "version", (char *) NULL
    };

    enum options {
	OPT_COMPILE, OPT_VERSION
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], cmdOptions, "option", 0,
	    &option) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options)option) {
	case OPT_COMPILE: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?options? sass");
		return TCL_ERROR;
	    }

	    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
	    break;
	}
	case OPT_VERSION: {
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }

	    Tcl_SetObjResult(interp, Tcl_NewStringObj(libsass_version(), -1));
	    break;
	}
	default: {
	    Tcl_AppendResult(interp, "bad option index\n", NULL);
	    code = TCL_ERROR;
	    goto done;
	}
    }

done:

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * SassObjCmdDeleteProc --
 *
 *	Handles deletion of the command(s) added by this package.
 *	This will cause the saved package data associated with the
 *	Tcl interpreter to be deleted, if it has not been already.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void SassObjCmdDeleteProc(
    ClientData clientData)	/* Current Tcl interpreter. */
{
    /*
     * NOTE: The client data for this callback function should be the
     *       pointer to the Tcl interpreter.  It must be valid.
     */

    Tcl_Interp *interp = (Tcl_Interp *) clientData;

    if (interp == NULL) {
	PACKAGE_TRACE(("SassObjCmdDeleteProc: no Tcl interpreter\n"));
	return;
    }

    /*
     * NOTE: Delete our saved association data from the Tcl interpreter
     *       because it only serves to help us delete the Tcl command
     *       that is apparently already in the process of being deleted.
     */

    Tcl_DeleteAssocData(interp, PACKAGE_NAME);
}
