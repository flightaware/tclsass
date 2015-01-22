#
# helper.tcl -- Tcl Package for libsass
#
# Defines procedures used with the command(s) created by this package.
#
# Written by Joe Mistachkin.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

namespace eval ::tclsass {
  #
  # NOTE: The syntax error message for the [compileWithSass] procedure.  It is
  #       used in several places.
  #
  variable compileSyntax \
      "wrong # args: should be \"compileWithSass ?options? source\""

  #
  # NOTE: This procedure wraps the [sass compile] sub-command.  The purpose of
  #       it is to add an "input_path" option if the input type is "file".  It
  #       should be noted that this procedure will NOT override a pre-existing
  #       "input_path" option.
  #
  proc compileWithSass { args } {
    variable compileSyntax

    #
    # NOTE: Since the [sass compile] sub-command requires at least one
    #       argument, having none is a syntax error.
    #
    if {[llength $args] == 0} then {
      error $compileSyntax
    }

    #
    # NOTE: Initially, there are no options set and the source is unknown.
    #
    array set options {}; set source ""

    #
    # NOTE: Calculate the ending list index.  This is used in the argument
    #       processing loop to serve as the loop invariant and to simplify
    #       the end-of-options checking.  This value may be zero.  However,
    #       it cannot be less than zero.
    #
    set endIndex [expr {[llength $args] - 1}]

    #
    # NOTE: Process every argument.  The first non-option argument will be
    #       treated as the "source" argument.  It must be the last argument.
    #
    for {set index 0} {$index <= $endIndex} {incr index} {
      #
      # NOTE: Grab the current argument, which may be an option.
      #
      set arg [lindex $args $index]

      #
      # NOTE: If this is not the last argument and it starts with a dash, it
      #       must be an option.
      #
      if {$index < $endIndex && [string index $arg 0] eq "-"} then {
        #
        # NOTE: All the options to this sub-command require a value; advance
        #       to it.
        #
        incr index

        #
        # NOTE: If there are no more arguments left, this is a syntax error.
        #
        if {$index >= $endIndex} then {
          error $compileSyntax
        }

        #
        # NOTE: If this is the formal end-of-options indicator, do nothing;
        #       otherwise, record the option value in the options array.
        #
        if {$arg ne "--"} then {
          set options($arg) [lindex $args $index]
        }
      } elseif {$index == $endIndex} then {
        #
        # NOTE: This is the last argument, which is the Sass source file
        #       or string.
        #
        set source $arg
      } else {
        #
        # NOTE: If this is not an option and not the last argument, this
        #       is a syntax error.
        #
        error $compileSyntax
      }
    }

    #
    # NOTE: Check if the type of input is "file".  If not, do nothing.
    #
    if {[info exists options(-type)] && $options(-type) eq "file"} then {
      #
      # NOTE: Is the "-options" option (not a typo) present?
      #
      if {[info exists options(-options)]} then {
        #
        # NOTE: Yes, so grab the value for it.
        #
        set dictionary $options(-options)

        #
        # NOTE: Does the "-options" option value contain the "input_path"
        #       key?  If so, do nothing; otherwise, add it with the value
        #       of the source file name.
        #
        if {![dict exists $dictionary input_path]} then {
          dict set dictionary input_path $source
        }
      } else {
        #
        # NOTE: No, so create an "-options" option value now with just the
        #       value of the source file name.
        #
        set dictionary [dict create input_path $source]
      }

      #
      # NOTE: Modify the "-options" option value to the modified (or brand
      #       new) dictionary value.
      #
      set options(-options) $dictionary
    }

    #
    # NOTE: Start building the final [sass compile] command to be evaluated,
    #       in the context of the caller.
    #
    set command [list sass compile]

    #
    # NOTE: If there are any options, add them now.
    #
    if {[array size options] > 0} then {
      eval lappend command [array get options]
      lappend command --
    }

    #
    # NOTE: If we get to this point, there must be a valid source, add it.
    #
    lappend command $source

    #
    # NOTE: Execute the command, in the context of the caller, returning its
    #       result as our own.
    #
    uplevel 1 $command
  }

  #
  # NOTE: This procedure is used by the package index file.  It is basically a
  #       trampoline, designed to permit this script file to be evaluated prior
  #       to loading the actual library binary.
  #
  proc load { directory fileName packageName } {
    uplevel 1 [list load [file join $directory $fileName] $packageName]
  }

  #
  # NOTE: Export the [compileWithSass] procedure.
  #
  namespace export compileWithSass
}
