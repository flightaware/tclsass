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
 * NOTE: These are the types of Sass contexts supported by the [sass compile]
 *       sub-command.  They are used to process the -type option.
 */

enum Sass_Context_Type {
  SASS_CONTEXT_NONE,
  SASS_CONTEXT_DATA,
  SASS_CONTEXT_FILE
};

/*
 * NOTE: Private functions defined in this file.
 */

static int		GetContextTypeFromObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, enum Sass_Context_Type *typePtr);
static int		GetOutputStyleFromObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, enum Sass_Output_Style *stylePtr);
static int		ProcessAllOptions(Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[], int *idxPtr,
			    enum Sass_Context_Type *typePtr,
			    struct Sass_Options *optsPtr);
static int		SetResultFromContext(Tcl_Interp *interp,
			    struct Sass_Context *ctxPtr);
static void		SassExitProc(ClientData clientData);
static int		SassObjCmd(ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[]);
static void		SassObjCmdDeleteProc(ClientData clientData);

/*
 *----------------------------------------------------------------------
 *
 * GetContextTypeFromObj --
 *
 *	This function attempts to convert a string context type name
 *	into an actual Sass_Context_Type value.  The valid values for
 *	the string context type name are:
 *
 *		data
 *		file
 *
 *	If the string context type name does not conform to one of the
 *	above values, it will be rejected and a script error will be
 *	generated.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int GetContextTypeFromObj(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    Tcl_Obj *objPtr,			/* The string to convert. */
    enum Sass_Context_Type *typePtr)	/* OUT: The context type. */
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetOutputStyleFromObj --
 *
 *	This function attempts to convert a string output style name
 *	into an actual Sass_Output_Style value.  The valid values for
 *	the string context type name are:
 *
 *		nested
 *		expanded
 *		compact
 *		compressed
 *
 *	If the string output style name does not conform to one of the
 *	above values, it will be rejected and a script error will be
 *	generated.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int GetOutputStyleFromObj(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    Tcl_Obj *objPtr,			/* The string to convert. */
    enum Sass_Output_Style *stylePtr)	/* OUT: The output style. */
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcessAllOptions --
 *
 *	This function processes options supported by the [sass compile]
 *	sub-command.  If an option does not coform to the expected type
 *	-OR- an unknown option is encountered, a script error will be
 *	generated.  All valid options, except -type, are processed by
 *	setting the appropriate field within the Sass_Options struct,
 *	using the public API.  The -type option is handled by processing
 *	the resulting Sass_Context_Type into the provided value pointer.
 *	The first argument index after all options are processed will be
 *	stored into the idxPtr argument, if applicable.  If there are no
 *	more arguments after processing the options, a value of -1 will
 *	be stored.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int ProcessAllOptions(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[],		/* The arguments. */
    int *idxPtr,			/* OUT: First non-option argument. */
    enum Sass_Context_Type *typePtr,	/* OUT: The context type. */
    struct Sass_Options *optsPtr)	/* IN/OUT: The context options. */
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SetResultFromContext --
 *
 *	This function quries the specified Sass_Context and uses the
 *	error status and output string to modify the result of the Tcl
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

static int SetResultFromContext(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    struct Sass_Context *ctxPtr)	/* IN: Get status/result from here. */
{
    return TCL_OK;
}

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
    Tcl_Obj *listPtr;

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

	    listPtr = Tcl_NewListObj(0, NULL);

	    if (listPtr == NULL) {
		Tcl_AppendResult(interp, "out of memory: listPtr\n", NULL);
		goto done;
	    }

	    Tcl_IncrRefCount(listPtr);
	    Tcl_ListObjAppendElement(interp, listPtr,
		Tcl_NewStringObj("libsass", -1));
	    Tcl_ListObjAppendElement(interp, listPtr,
		Tcl_NewStringObj(libsass_version(), -1));
	    Tcl_SetObjResult(interp, listPtr);
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
