#!/usr/bin/perl
use strict;
use warnings;

use Time::HiRes qw( usleep ualarm gettimeofday tv_interval nanosleep
                      clock_gettime clock_getres clock_nanosleep clock
                      stat );
use Sys::Hostname;

my @cores = ( 1, 2, 3, 4, 6, 8 );
my @wkloads = ( 'A00', 'A05', 'A10' );
my @iters = (1);

# csv header
print "host;wkload;cores;iter;duration\n";

foreach my $it (@iters) {
    foreach my $cr (@cores) {
        foreach my $wkld (@wkloads) {
            # construct the mapping
            my $amap = '-1';
            for (my $i = 1; $i < $cr; ++$i) {
                $amap = $amap . ',' . $i;
            }
            
            my $prgline = 'AMAP=' . $amap . '  ./gigo_1-lpel  <wkload-' . $wkld . '.xml';
            
            my $cmdline = $prgline;
            print STDERR "\nHost: " . hostname . '; Workload: '. $wkld . '; Cores: ' . $cr . '; Iter: ' . $it . "\n";
            print STDERR "  cmd: \"$cmdline\"\n";
            
            my $t0 = [gettimeofday];
            my $output = qx($cmdline);
            my $duration = tv_interval ( $t0, [gettimeofday]);
            
            print STDERR "    Output: $output";
            print STDERR "    Execution time: $duration s\n";
            
            print '"' . hostname . '";"' . $wkld . '";' . $cr . ';' . $it . ';' . $duration . "\n";
        }
    }
}
