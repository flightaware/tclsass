### libsass version support

The libsass library has several API changes over time.  So, the code has dependencies on the libsass version.
Look at the file sass/version.h to find your installed libsass version.  This code should work with 3.3.x, 3.4.x and when released 3.5.x.

Over time there was some mixing of C++ delete and C free in libsass API.  The library libsass should free passed strings.
Look at the file sass/version.h to find your installed libsass version.  This code should work with 3.3.x, 3.4.x and when released 3.5.x.

Set CFLAGS=-DTCLSAS_CALLER_FREE to free strings passed to the API.

### Configuration
 
You should be able to build with

    autoreconf && ./configure && make && make install

If you need to adjust the paths to locate libsass, use the CFLAGS and LDFLAGS variables to pass the paths to configure.


### Building

Some versions of gcc produce bad code compiling libsass.  For example, 4.8.3.

We have found that none of the tclsass tests will pass if libsass is compiled with -O2 but all of them will pass if it is compiled with -O1.

gcc 4.8.5 works OK compiling libsass with -O2.  We don't know what happens with 4.8.4.

libsass is writting using features in the c++0x standard that weren't added until gcc 4.6, so if you get something about option not recognized for -std, your C++ compiler is too old.

### How to use

Here is the revised spec (v3):

Tcl Package Format: TEA

Documentation Format: TEA

Tcl Command Name: "sass"

Sub-Commands: "version", "compile"

The [sass version] sub-command will have no arguments.
It will return the full version of libsass in use.

The [sass compile] sub-command will have the following options:

    -type <type>; # "type" must be "data" or "file".
    -options <dictionary>; # see below.

The [sass compile] sub-command will either return an error -OR-
the compiler output, which will be a dictionary.  The possible
named values within the dictionary will initially be:

    errorStatus; # always available, zero means success
    outputString; # success only
    sourceMapString; # success only (with source maps enabled)
    errorMessage; # failure only
    errorLine; # failure only
    errorColumn; # failure only

For the dictionary value of -options, the following names will
be supported:

    precision (int)
    output_style (enum)
    source_comments (bool)
    source_map_embed (bool)
    source_map_contents (bool)
    omit_source_map_url (bool)
    is_indented_syntax_src (bool)
    indent (string)
    linefeed (string)
    input_path (string)
    output_path (string)
    image_path (string) ... (removed by libsass, now an error)
    include_path (string)
    source_map_file (string)

This above list of options is based on the libsass public
interface and is subject to change in future versions.
