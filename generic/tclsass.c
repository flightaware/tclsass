/*
 * tclsass.c -- Tcl Package for libsass
 *
 * Written by Joe Mistachkin.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <stdlib.h>		/* NOTE: For free(). */
#include <string.h>		/* NOTE: For strlen(), strcmp(), strdup(). */
#include "tcl.h"		/* NOTE: For public Tcl API. */
#include "sass_context.h"	/* NOTE: For public libsass API. */
#include "pkgVersion.h"		/* NOTE: Package version information. */
#include "tclsassInt.h"		/* NOTE: For private package API. */
#include "tclsass.h"		/* NOTE: For public package API. */

/*
 * NOTE: These are the types of Sass contexts supported by the [sass compile]
 *       sub-command.  They are used to process the -type option.  The values
 *       were stolen from the Sass_Input_Style enumeration within the libsass
 *       source code file "sass_context.cpp".
 */

enum Sass_Context_Type {
  SASS_CONTEXT_NULL,
  SASS_CONTEXT_FILE,
  SASS_CONTEXT_DATA,
  SASS_CONTEXT_FOLDER
};

/*
 * NOTE: Private functions defined in this file.
 */

static int		GetContextTypeFromObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, enum Sass_Context_Type *typePtr);
static int		GetOutputStyleFromObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, enum Sass_Output_Style *stylePtr);
static int		ProcessContextOptions(Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[], int *idxPtr,
			    enum Sass_Context_Type *typePtr,
			    struct Sass_Options *optsPtr);
static int		SetResultFromContext(Tcl_Interp *interp,
			    struct Sass_Context *ctxPtr);
static int		CompileForType(Tcl_Interp *interp,
			    enum Sass_Context_Type type,
			    struct Sass_Options **pOptsPtr,
			    const char* zSource);
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
    const char *zType;
    int length;

    if (interp == NULL) {
	PACKAGE_TRACE(("GetContextTypeFromObj: no Tcl interpreter\n"));
	return TCL_ERROR;
    }

    if (objPtr == NULL) {
	Tcl_AppendResult(interp, "no context type object\n", NULL);
	return TCL_ERROR;
    }

    if (typePtr == NULL) {
	Tcl_AppendResult(interp, "no context type pointer\n", NULL);
	return TCL_ERROR;
    }

    zType = Tcl_GetStringFromObj(objPtr, &length);

    if ((zType == NULL) || (length < 0)) {
	Tcl_AppendResult(interp, "bad context type string\n", NULL);
	return TCL_ERROR;
    }

    if ((length == 4) && (strcmp(zType, "data") == 0)) {
	*typePtr = SASS_CONTEXT_DATA;
	return TCL_OK;
    }

    if ((length == 4) && (strcmp(zType, "file") == 0)) {
	*typePtr = SASS_CONTEXT_FILE;
	return TCL_OK;
    }

    Tcl_AppendResult(interp,
	"unsupported context type, must be: data or file\n", NULL);

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetOutputStyleFromObj --
 *
 *	This function attempts to convert a string output style name
 *	into an actual Sass_Output_Style value.  The valid values for
 *	the string output style name are:
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
    const char *zStyle;
    int length;

    if (interp == NULL) {
	PACKAGE_TRACE(("GetOutputStyleFromObj: no Tcl interpreter\n"));
	return TCL_ERROR;
    }

    if (objPtr == NULL) {
	Tcl_AppendResult(interp, "no output style object\n", NULL);
	return TCL_ERROR;
    }

    if (stylePtr == NULL) {
	Tcl_AppendResult(interp, "no output style pointer\n", NULL);
	return TCL_ERROR;
    }

    zStyle = Tcl_GetStringFromObj(objPtr, &length);

    if ((zStyle == NULL) || (length < 0)) {
	Tcl_AppendResult(interp, "bad output style string\n", NULL);
	return TCL_ERROR;
    }

    if ((length == 6) && (strcmp(zStyle, "nested") == 0)) {
	*stylePtr = SASS_STYLE_NESTED;
	return TCL_OK;
    }

    if ((length == 8) && (strcmp(zStyle, "expanded") == 0)) {
	*stylePtr = SASS_STYLE_EXPANDED;
	return TCL_OK;
    }

    if ((length == 7) && (strcmp(zStyle, "compact") == 0)) {
	*stylePtr = SASS_STYLE_COMPACT;
	return TCL_OK;
    }

    if ((length == 10) && (strcmp(zStyle, "compressed") == 0)) {
	*stylePtr = SASS_STYLE_COMPRESSED;
	return TCL_OK;
    }

    Tcl_AppendResult(interp,
	"unsupported output style, must be: nested, expanded, compact, "
	"or compressed\n", NULL);

    return TCL_ERROR;
}

static int CheckNameAndGetValue(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[],		/* The array of arguments. */
    int *idxPtr,			/* IN/OUT: Arg before/after value. */
    struct Sass_Options *optsPtr)	/* IN/OUT: The context options. */
{
    if (interp == NULL) {
	PACKAGE_TRACE(("CheckNameAndGetValue: no Tcl interpreter\n"));
	return TCL_ERROR;
    }




    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcessContextOptions --
 *
 *	This function processes options supported by the [sass compile]
 *	sub-command.  If an option does not coform to the expected type
 *	-OR- an unknown option is encountered, a script error will be
 *	generated.  All valid options, except -type, are processed by
 *	setting the appropriate field within the Sass_Options struct,
 *	using the public API.  The -type option is handled by processing
 *	the resulting Sass_Context_Type into the provided value pointer.
 *	The first option argument index to check is queried from the
 *	idxPtr argument.  Furthermore, the first non-option argument
 *	index after all options are processed will be stored into the
 *	idxPtr argument, if applicable.  If there are no more arguments
 *	after processing the options, a value of -1 will be stored.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int ProcessContextOptions(
    Tcl_Interp *interp,			/* Current Tcl interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[],		/* The array of arguments. */
    int *idxPtr,			/* IN/OUT: 1st [non-]option argument. */
    enum Sass_Context_Type *typePtr,	/* OUT: The context type. */
    struct Sass_Options *optsPtr)	/* IN/OUT: The context options. */
{
    int index;

    if (interp == NULL) {
	PACKAGE_TRACE(("ProcessContextOptions: no Tcl interpreter\n"));
	return TCL_ERROR;
    }

    if (objv == NULL) {
	Tcl_AppendResult(interp, "no arguments array\n", NULL);
	return TCL_ERROR;
    }

    if (idxPtr == NULL) {
	Tcl_AppendResult(interp, "no argument index pointer\n", NULL);
	return TCL_ERROR;
    }

    if (typePtr == NULL) {
	Tcl_AppendResult(interp, "no context type pointer\n", NULL);
	return TCL_ERROR;
    }

    if (optsPtr == NULL) {
	Tcl_AppendResult(interp, "no options pointer\n", NULL);
	return TCL_ERROR;
    }

    *typePtr = SASS_CONTEXT_DATA; /* TODO: Good default? */

    for (index = *idxPtr; index < objc; index++) {
	Tcl_Obj objPtr;
	const char *zArg;
	int length;

	objPtr = objv[index];

	if (objPtr == NULL) {
	    Tcl_AppendResult(interp, "bad argument object\n", NULL);
	    return TCL_ERROR;
	}

	zArg = Tcl_GetStringFromObj(objPtr, &length);

	if ((zArg == NULL) || (length < 0)) {
	    Tcl_AppendResult(interp, "bad argument string\n", NULL);
	    return TCL_ERROR;
	}

	if ((length == 2) && (strcmp(zArg, "--") == 0)) {
	    *idxPtr = index;
	    return TCL_OK;
	}

	if ((length == 5) && (strcmp(zArg, "-type") == 0)) {
	    index++;
	    if (index >= objc) {
		Tcl_AppendResult(interp, "missing context type\n", NULL);
		return TCL_ERROR;
	    }
	    objPtr = objv[index];
	    if (GetContextTypeFromObj(interp, objPtr, typePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    continue;
	}

	if ((length == 8) && (strcmp(zArg, "-options") == 0)) {
	    int dictObjc;
	    Tcl_Obj *dictObjv[];
	    int dictIndex;
	    index++;
	    if (index >= objc) {
		Tcl_AppendResult(interp, "missing options dictionary\n", NULL);
		return TCL_ERROR;
	    }
	    objPtr = objv[index];
	    if (Tcl_ListObjGetElements(interp, objPtr, &dictObjc, &dictObjv)) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((dictObjc % 2) != 0) {
		Tcl_AppendResult(interp, "malformed dictionary\n", NULL);
		return TCL_ERROR;
	    }
	    for (dictIndex = 0; dictIndex < dictObjc; dictIndex += 2) {
		int intValue;
		enum Sass_Output_Style styleValue;
		bool boolValue;
		const char *zValue;
		objPtr = dictObjv[dictIndex];
		zArg = Tcl_GetStringFromObj(objPtr, &length);
		if ((zArg == NULL) || (length < 0)) {
		    Tcl_AppendResult(interp, "bad dictionary string\n", NULL);
		    return TCL_ERROR;
		}
		if ((length == 9) && (strcmp(zArg, "precision") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (Tcl_GetIntFromObj(interp, objPtr, &intValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_precision(optsPtr, intValue);
		    continue;
		}
		if ((length == 12) && (strcmp(zArg, "output_style") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (GetOutputStyleFromObj(interp, objPtr, &styleValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_output_style(optsPtr, styleValue);
		    continue;
		}
		if ((length == 15) && (strcmp(zArg, "source_comments") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (Tcl_GetBooleanFromObj(interp, objPtr, &boolValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_source_comments(optsPtr, boolValue);
		    continue;
		}
		if ((length == 16) && (strcmp(zArg, "source_map_embed") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (Tcl_GetBooleanFromObj(interp, objPtr, &boolValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_source_map_embed(optsPtr, boolValue);
		    continue;
		}
		if ((length == 19) && (strcmp(zArg, "source_map_contents") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (Tcl_GetBooleanFromObj(interp, objPtr, &boolValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_source_map_contents(optsPtr, boolValue);
		    continue;
		}
		if ((length == 19) && (strcmp(zArg, "omit_source_map_url") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (Tcl_GetBooleanFromObj(interp, objPtr, &boolValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_omit_source_map_url(optsPtr, boolValue);
		    continue;
		}
		if ((length == 22) && (strcmp(zArg, "is_indented_syntax_src") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    if (Tcl_GetBooleanFromObj(interp, objPtr, &boolValue) != TCL_OK) {
			return TCL_ERROR;
		    }
		    sass_option_set_is_indented_syntax_src(optsPtr, boolValue);
		    continue;
		}
		if ((length == 6) && (strcmp(zArg, "indent") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_indent(optsPtr, zValue);
		    continue;
		}
		if ((length == 8) && (strcmp(zArg, "linefeed") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_linefeed(optsPtr, zValue);
		    continue;
		}
		if ((length == 10) && (strcmp(zArg, "input_path") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_input_path(optsPtr, zValue);
		    continue;
		}
		if ((length == 11) && (strcmp(zArg, "output_path") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_output_path(optsPtr, zValue);
		    continue;
		}
		if ((length == 10) && (strcmp(zArg, "image_path") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_image_path(optsPtr, zValue);
		    continue;
		}
		if ((length == 12) && (strcmp(zArg, "include_path") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_include_path(optsPtr, zValue);
		    continue;
		}
		if ((length == 15) && (strcmp(zArg, "source_map_file") == 0)) {
		    objPtr = dictObjv[dictIndex + 1];
		    zValue = Tcl_GetStringFromObj(objPtr, &length);
		    if ((zValue == NULL) || (length < 0)) {
			Tcl_AppendResult(interp, "bad value string\n", NULL);
			return TCL_ERROR;
		    }
		    sass_option_set_source_map_file(optsPtr, zValue);
		    continue;
		}

		Tcl_AppendResult(interp, "unsupported option, must be: "
		    "precision, output_style, source_comments, "
		    "source_map_embed, source_map_contents, "
		    "omit_source_map_url, is_indented_syntax_src, indent, "
		    "linefeed, input_path, output_path, image_path, "
		    "include_path, or source_map_file\n", NULL);

		return TCL_ERROR;
	    }
	    continue;
	}

	*idxPtr = index;
	return TCL_OK;
    }

    *idxPtr = -1;
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
    int code;
    int rc;
    struct Sass_Options *optsPtr;
    const char *zSourceMapFile;
    Tcl_Obj *listPtr = NULL;
    Tcl_Obj *objPtr;

    if (interp == NULL) {
	PACKAGE_TRACE(("SetResultFromContext: no Tcl interpreter\n"));
	return TCL_ERROR;
    }

    if (ctxPtr == NULL) {
	Tcl_AppendResult(interp, "no context\n", NULL);
	return TCL_ERROR;
    }

    listPtr = Tcl_NewListObj(0, NULL);

    if (listPtr == NULL) {
	Tcl_AppendResult(interp, "out of memory: listPtr\n", NULL);
	code = TCL_ERROR;
	goto done;
    }

    Tcl_IncrRefCount(listPtr);
    objPtr = Tcl_NewStringObj("errorStatus", -1);

    if (objPtr == NULL) {
	Tcl_AppendResult(interp, "out of memory: errorStatus1\n", NULL);
	code = TCL_ERROR;
	goto done;
    }

    Tcl_IncrRefCount(objPtr);
    code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
    Tcl_DecrRefCount(objPtr);

    if (code != TCL_OK)
	goto done;

    rc = sass_context_get_error_status(ctxPtr);
    objPtr = Tcl_NewIntObj(rc);

    if (objPtr == NULL) {
	Tcl_AppendResult(interp, "out of memory: errorStatus2\n", NULL);
	code = TCL_ERROR;
	goto done;
    }

    Tcl_IncrRefCount(objPtr);
    code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
    Tcl_DecrRefCount(objPtr);

    if (code != TCL_OK)
	goto done;

    if (rc == 0) {
	objPtr = Tcl_NewStringObj("outputString", -1);

	if (objPtr == NULL) {
	    Tcl_AppendResult(interp, "out of memory: outputString1\n", NULL);
	    code = TCL_ERROR;
	    goto done;
	}

	Tcl_IncrRefCount(objPtr);
	code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	Tcl_DecrRefCount(objPtr);

	if (code != TCL_OK)
	    goto done;

	objPtr = Tcl_NewStringObj(sass_context_get_output_string(ctxPtr), -1);

	if (objPtr == NULL) {
	    Tcl_AppendResult(interp, "out of memory: outputString2\n", NULL);
	    code = TCL_ERROR;
	    goto done;
	}

	Tcl_IncrRefCount(objPtr);
	code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	Tcl_DecrRefCount(objPtr);

	if (code != TCL_OK)
	    goto done;

	optsPtr = sass_context_get_options(ctxPtr);

	zSourceMapFile = (optsPtr != NULL) ?
	    sass_option_get_source_map_file(optsPtr) : NULL;

	if ((zSourceMapFile != NULL) && (strlen(zSourceMapFile) > 0)) {
	    objPtr = Tcl_NewStringObj("sourceMapString", -1);

	    if (objPtr == NULL) {
		Tcl_AppendResult(interp,
		    "out of memory: sourceMapString1\n", NULL);

		code = TCL_ERROR;
		goto done;
	    }

	    Tcl_IncrRefCount(objPtr);
	    code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    Tcl_DecrRefCount(objPtr);

	    if (code != TCL_OK)
		goto done;

	    objPtr = Tcl_NewStringObj(
		sass_context_get_source_map_string(ctxPtr), -1);

	    if (objPtr == NULL) {
		Tcl_AppendResult(interp,
		    "out of memory: sourceMapString2\n", NULL);

		code = TCL_ERROR;
		goto done;
	    }

	    Tcl_IncrRefCount(objPtr);
	    code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    Tcl_DecrRefCount(objPtr);

	    if (code != TCL_OK)
		goto done;
	}
    } else {
	objPtr = Tcl_NewStringObj("errorMessage", -1);

	if (objPtr == NULL) {
	    Tcl_AppendResult(interp, "out of memory: errorMessage1\n", NULL);
	    code = TCL_ERROR;
	    goto done;
	}

	Tcl_IncrRefCount(objPtr);
	code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	Tcl_DecrRefCount(objPtr);

	if (code != TCL_OK)
	    goto done;

	objPtr = Tcl_NewStringObj(sass_context_get_error_message(ctxPtr), -1);

	if (objPtr == NULL) {
	    Tcl_AppendResult(interp, "out of memory: errorMessage2\n", NULL);
	    code = TCL_ERROR;
	    goto done;
	}

	Tcl_IncrRefCount(objPtr);
	code = Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	Tcl_DecrRefCount(objPtr);

	if (code != TCL_OK)
	    goto done;
    }

    Tcl_SetObjResult(interp, listPtr);

done:
    if (listPtr != NULL) {
	Tcl_DecrRefCount(listPtr);
	listPtr = NULL;
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileForType --
 *
 *	This function attempts to create a Sass_Context based on the
 *	specified Sass_Context_Type, compile it, and then set the Tcl
 *	interpreter result based on its output.  A script error will
 *	be generated if the context type is unsupported -OR- context
 *	creation fails -OR- context compilation fails.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int CompileForType(
    Tcl_Interp *interp,
    enum Sass_Context_Type type,
    struct Sass_Options **pOptsPtr,
    const char* zSource)
{
    int rc;

    if (interp == NULL) {
	PACKAGE_TRACE(("CompileForType: no Tcl interpreter\n"));
	return TCL_ERROR;
    }

    if (pOptsPtr == NULL) {
	Tcl_AppendResult(interp, "no options pointer\n", NULL);
	return TCL_ERROR;
    }

    if (zSource == NULL) {
	Tcl_AppendResult(interp, "no source\n", NULL);
	return TCL_ERROR;
    }

    switch (type) {
	case SASS_CONTEXT_FILE: {
	    struct Sass_File_Context *ctxPtr;

	    ctxPtr = sass_make_file_context(zSource);

	    if (ctxPtr == NULL) {
		Tcl_AppendResult(interp, "out of memory: ctxPtr\n", NULL);
		return TCL_ERROR;
	    }

	    if (*pOptsPtr != NULL) {
		sass_file_context_set_options(ctxPtr, *pOptsPtr);
		*pOptsPtr = NULL;
	    }

	    rc = sass_compile_file_context(ctxPtr);
	    SetResultFromContext(interp, (struct Sass_Context *)ctxPtr);
	    sass_delete_file_context(ctxPtr);
	    return (rc == 0) ? TCL_OK : TCL_ERROR;
	}
	case SASS_CONTEXT_DATA: {
	    struct Sass_Data_Context *ctxPtr;
	    char *zDup = strdup(zSource);

	    if (zDup == NULL) {
		Tcl_AppendResult(interp, "out of memory: zDup\n", NULL);
		return TCL_ERROR;
	    }

	    ctxPtr = sass_make_data_context(zDup);

	    if (ctxPtr == NULL) {
		free(zDup);
		Tcl_AppendResult(interp, "out of memory: ctxPtr\n", NULL);
		return TCL_ERROR;
	    }

	    if (*pOptsPtr != NULL) {
		sass_data_context_set_options(ctxPtr, *pOptsPtr);
		*pOptsPtr = NULL;
	    }

	    rc = sass_compile_data_context(ctxPtr);
	    SetResultFromContext(interp, (struct Sass_Context *)ctxPtr);
	    sass_delete_data_context(ctxPtr);
	    free(zDup);

	    return (rc == 0) ? TCL_OK : TCL_ERROR;
	}
	default: {
	    char buffer[50] = {0};

	    snprintf(buffer, sizeof(buffer) - 1,
		"cannot compile, unsupported type %d\n", type);

	    Tcl_AppendResult(interp, buffer, NULL);
	    return TCL_ERROR;
	}
    }
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
    Tcl_Obj *CONST objv[])	/* The array of arguments. */
{
    int code = TCL_OK;
    int option;
    enum Sass_Context_Type type = SASS_CONTEXT_NULL;
    struct Sass_Options *optsPtr = NULL;

    static CONST char *cmdOptions[] = {
	"compile", "version", (char *) NULL
    };

    enum options {
	OPT_COMPILE, OPT_VERSION
    };

    if (interp == NULL) {
	PACKAGE_TRACE(("SassObjCmd: no Tcl interpreter\n"));
	return TCL_ERROR;
    }

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
	    int index = 2; /* NOTE: Start right after [sass compile]. */

	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?options? source");
		return TCL_ERROR;
	    }

	    optsPtr = sass_make_options();

	    if (optsPtr == NULL) {
		Tcl_AppendResult(interp, "out of memory: optsPtr\n", NULL);
		code = TCL_ERROR;
		goto done;
	    }

	    code = ProcessContextOptions(interp, objc, objv, &index, &type,
		optsPtr);

	    if (code != TCL_OK)
		goto done;

	    if ((index < 0) || ((index + 1) != objc)) {
		Tcl_WrongNumArgs(interp, 2, objv, "?options? source");
		code = TCL_ERROR;
		goto done;
	    }

	    code = CompileForType(interp, type, &optsPtr,
		Tcl_GetString(objv[index]));

	    break;
	}
	case OPT_VERSION: {
	    Tcl_Obj *listPtr;
	    Tcl_Obj *objPtr1;
	    Tcl_Obj *objPtr2;

	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }

	    listPtr = Tcl_NewListObj(0, NULL);

	    if (listPtr == NULL) {
		Tcl_AppendResult(interp, "out of memory: listPtr\n", NULL);
		code = TCL_ERROR;
		goto done;
	    }

	    Tcl_IncrRefCount(listPtr);
	    objPtr1 = Tcl_NewStringObj("libsass", -1);

	    if (objPtr1 == NULL) {
		Tcl_DecrRefCount(listPtr);
		Tcl_AppendResult(interp, "out of memory: objPtr1\n", NULL);
		code = TCL_ERROR;
		goto done;
	    }

	    Tcl_IncrRefCount(objPtr1);
	    objPtr2 = Tcl_NewStringObj(libsass_version(), -1);

	    if (objPtr2 == NULL) {
		Tcl_DecrRefCount(listPtr);
		Tcl_DecrRefCount(objPtr1);
		Tcl_AppendResult(interp, "out of memory: objPtr2\n", NULL);
		code = TCL_ERROR;
		goto done;
	    }

	    Tcl_IncrRefCount(objPtr2);
	    code = Tcl_ListObjAppendElement(interp, listPtr, objPtr1);

	    if (code != TCL_OK) {
		Tcl_DecrRefCount(listPtr);
		Tcl_DecrRefCount(objPtr1);
		Tcl_DecrRefCount(objPtr2);
		code = TCL_ERROR;
		goto done;
	    }

	    code = Tcl_ListObjAppendElement(interp, listPtr, objPtr2);

	    if (code != TCL_OK) {
		Tcl_DecrRefCount(listPtr);
		Tcl_DecrRefCount(objPtr1);
		Tcl_DecrRefCount(objPtr2);
		code = TCL_ERROR;
		goto done;
	    }

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
    if (optsPtr != NULL) {
	free(optsPtr); /* HACK: No official destructor function. */
	optsPtr = NULL;
    }

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
