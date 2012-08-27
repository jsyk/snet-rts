#!/usr/bin/perl
use strict;
use warnings;

use Time::HiRes qw( usleep ualarm gettimeofday tv_interval nanosleep
                      clock_gettime clock_getres clock_nanosleep clock
                      stat );
use Sys::Hostname;

my @cores = ( 1, 2, 3, 4, 6, 8 );
my $totcores = 8;
my @wkloads = ( 'abc' );
my @iters = (1);

# csv header
print "host;wkload;acores;bcores;ccores;iter;duration\n";

foreach my $it (@iters) {
    foreach my $acr (@cores) {
        foreach my $bcr (@cores) {
            foreach my $ccr (@cores) {
                # use all 8 cores
                if (($acr + $bcr + $ccr) == $totcores) {
                    foreach my $wkld (@wkloads) {
                        # the first free core; we second-guess snet-rts that it allocates
                        # the first 3 cores to the boxes
                        my $freecore = 3;
                        
                        # construct the mapping for box A
                        my $amap = '-1';
                        for (my $i = 1; $i < $acr; ++$i) {
                            $amap = $amap . ',' . $freecore;
                            ++$freecore;
                        }
                        
                        # construct the mapping for box B
                        my $bmap = '-1';
                        for (my $i = 1; $i < $bcr; ++$i) {
                            $bmap = $bmap . ',' . $freecore;
                            ++$freecore;
                        }
                        
                        # construct the mapping for box C
                        my $cmap = '-1';
                        for (my $i = 1; $i < $ccr; ++$i) {
                            $cmap = $cmap . ',' . $freecore;
                            ++$freecore;
                        }
                        
                        my $prgline = "AMAP=$amap BMAP=$bmap CMAP=$cmap  ./gigo_3-lpel  <wkload-" . $wkld . '.xml';
                        
                        my $cmdline = $prgline;
                        print STDERR "\nHost: " . hostname . '; Workload: '. $wkld . "; Cores: ($acr, $bcr, $ccr); Iter: " . $it . "\n";
                        print STDERR "  cmd: \"$cmdline\"\n";
                        
                        my $t0 = [gettimeofday];
                        my $output = qx($cmdline);
                        my $duration = tv_interval ( $t0, [gettimeofday]);
                        
                        print STDERR "    Output: $output\n";
                        print STDERR "    Execution time: $duration s\n";
                        
                        print '"' . hostname . '";"' . $wkld . '";' . $acr . ';' . $bcr . ';' . $ccr . ';' . $it . ';' . $duration . "\n";
                    }
                }
            }
        }
    }
}
