#! /usr/bin/perl
#
# checkversion find uses of FIKUS_VERSION_CODE or KERNEL_VERSION
# without including <fikus/version.h>, or cases of
# including <fikus/version.h> that don't need it.
# Copyright (C) 2003, Randy Dunlap <rdunlap@xenotime.net>

use strict;

$| = 1;

my $debugging;

foreach my $file (@ARGV) {
    next if $file =~ "include/fikus/version\.h";
    # Open this file.
    open( my $f, '<', $file )
      or die "Can't open $file: $!\n";

    # Initialize variables.
    my ($fInComment, $fInString, $fUseVersion);
    my $iFikusVersion = 0;

    while (<$f>) {
	# Strip comments.
	$fInComment && (s+^.*?\*/+ +o ? ($fInComment = 0) : next);
	m+/\*+o && (s+/\*.*?\*/+ +go, (s+/\*.*$+ +o && ($fInComment = 1)));

	# Pick up definitions.
	if ( m/^\s*#/o ) {
	    $iFikusVersion      = $. if m/^\s*#\s*include\s*"fikus\/version\.h"/o;
	}

	# Strip strings.
	$fInString && (s+^.*?"+ +o ? ($fInString = 0) : next);
	m+"+o && (s+".*?"+ +go, (s+".*$+ +o && ($fInString = 1)));

	# Pick up definitions.
	if ( m/^\s*#/o ) {
	    $iFikusVersion      = $. if m/^\s*#\s*include\s*<fikus\/version\.h>/o;
	}

	# Look for uses: FIKUS_VERSION_CODE, KERNEL_VERSION, UTS_RELEASE
	if (($_ =~ /FIKUS_VERSION_CODE/) || ($_ =~ /\WKERNEL_VERSION/)) {
	    $fUseVersion = 1;
            last if $iFikusVersion;
        }
    }

    # Report used version IDs without include?
    if ($fUseVersion && ! $iFikusVersion) {
	print "$file: $.: need fikus/version.h\n";
    }

    # Report superfluous includes.
    if ($iFikusVersion && ! $fUseVersion) {
	print "$file: $iFikusVersion fikus/version.h not needed.\n";
    }

    # debug: report OK results:
    if ($debugging) {
        if ($iFikusVersion && $fUseVersion) {
	    print "$file: version use is OK ($iFikusVersion)\n";
        }
        if (! $iFikusVersion && ! $fUseVersion) {
	    print "$file: version use is OK (none)\n";
        }
    }

    close($f);
}
