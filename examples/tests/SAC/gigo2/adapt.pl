#!/usr/bin/perl
use strict;
use warnings;
use List::Util qw(first max maxstr min minstr reduce shuffle sum);
use Time::HiRes qw( usleep ualarm gettimeofday tv_interval nanosleep
                      clock_gettime clock_getres clock_nanosleep clock
                      stat );


sub read_mapping
{
    (open my $map_fh, "<n00_tasks.map") or die "Could not open the map file!";
    local $/ = '#';
    my %map = ();
    while (<$map_fh>) {
        chomp;
        #print $_ . "\n";
        if (/^(\d+):S\d+ (\w+) (\d+)$/) {
            my $taskid = $1;
            my $taskname = $2;
            my $workerid = $3;
            $map{$taskname} = { 'taskid' => $taskid, 'workerid' => $workerid };
            print "taskid: $taskid, taskname:$taskname, workerid:$workerid\n";
        }
    }
    close $map_fh;
    return \%map;
}

sub read_log
{
    my $workerid = shift;
    #my $ = shift;
    #
    my $fname = sprintf('mon_n00_worker%02d.log', $workerid);
    (open my $log_fh, "<$fname") or die "Could not open the log file $fname!";
    local $/ = '#';
    my %tasklog = ();
    while (<$log_fh>) {
        chomp;
        #print $_ . "\n";
        if (/^(\d+)(\w)(\d+) (\d+)/) {
            # task has been de-scheduled;
            my $timestamp = $1;
            my $event = $2;
            my $taskid = $3;
            my $exetime = $4;       # in nanoseconds
            
            if (defined $tasklog{$taskid}) {
                my $old_exetime = $tasklog{$taskid}[0];
                $tasklog{$taskid} = [ $old_exetime + $exetime ];
            } else {
                $tasklog{$taskid} = [ $exetime ];
            }
        }
    }
    close $log_fh;
    return \%tasklog;
}

sub get_measurements
{
    my $boxes = shift;
    #
    my $mapping = read_mapping();
    
    while ( my ($taskname, $taskinfo) = each(%$mapping) ) {
        my $taskid = $taskinfo->{'taskid'};
        my $workerid = $taskinfo->{'workerid'};
        
        my $tasklog = read_log($workerid);
        if (not defined $tasklog->{$taskid}) {
            die "Task $taskid not found at the worker $workerid !";
        }
        
        my $exetime = $tasklog->{$taskid}->[0] / 1e9;
        print "$taskname => taskid:$taskid, workerid:$workerid, exetime: $exetime\n";
        $tasklog->{'exetime'} = $exetime;
        
        if (defined($boxes->{$taskname})) {
            my $data = $boxes->{$taskname}->{'data'};
            my $p = $boxes->{$taskname}->{'p'};
            push @{$data}, [ $p, $exetime ];
        }
    }
}

sub def_box
{
    return { 'data' => [], 'p' => 1, 'Ts' => undef, 'alpha' => undef, 'tp' => undef };
}

sub print_boxes
{
    my $boxes = shift;
    while (my ($boxname, $boxinfo) = each(%$boxes)) {
        my $p = $boxinfo->{'p'};
        my $data = $boxinfo->{'data'};
        my $Ts = $boxinfo->{'Ts'};
        my $alpha = $boxinfo->{'alpha'};
        my $tp = $boxinfo->{'tp'};
        if (not defined($Ts)) { $Ts = 'NA'; }
        if (not defined($alpha)) { $alpha = 'NA'; }
        if (not defined($tp)) { $tp = 'NA'; }
        
        my $strdata = '(';
        for (my $i = 0; $i < scalar(@$data); ++$i) {
            $strdata = $strdata . '(' . $data->[$i]->[0] . '|' . $data->[$i]->[1] . ') ';
        }
        print "box:$boxname => p:$p, Ts:$Ts, alpha:$alpha, tp:$tp, data:$strdata)\n";
    }
}

sub run_experiment
{
    my $boxes = shift;
    #
    # the first free core; we second-guess snet-rts that it allocates
    # the first 3 cores to the boxes
    my $freecore = 3;
    
    # construct the mapping for box A
    my $amap = '-1';
    for (my $i = 1; $i < $boxes->{'workload_V1'}->{'p'}; ++$i) {
        $amap = $amap . ',' . $freecore;
        ++$freecore;
    }
    
    # construct the mapping for box B
    my $bmap = '-1';
    for (my $i = 1; $i < $boxes->{'workload_V2'}->{'p'}; ++$i) {
        $bmap = $bmap . ',' . $freecore;
        ++$freecore;
    }
    
    # construct the mapping for box C
    my $cmap = '-1';
    for (my $i = 1; $i < $boxes->{'workload_V3'}->{'p'}; ++$i) {
        $cmap = $cmap . ',' . $freecore;
        ++$freecore;
    }
    
    my $wkld = 'abc2';
    my $prgline = "AMAP=$amap BMAP=$bmap CMAP=$cmap  ./gigo_3-lpel -m A <wkload-" . $wkld . '.xml';
    
    #print "==============================================================\n";
    print "---- Running the snet: $prgline\n";
    my $t0 = [gettimeofday];
    my $output = qx($prgline);
    my $duration = tv_interval ( $t0, [gettimeofday]);
    
    #print "    Output: $output\n";
    print "    Execution time: $duration s\n";
}

sub aggregate
{
    my $xdata = shift;
    #
    my @data = @{ $xdata };
    my @res = @{ $data[0] };
    
    #print join(',', @res) . "\n";
    
    for (my $i = 1; $i < scalar(@data); ++$i) {
        my @dp = @{ $data[$i] };
        for (my $k = 0; $k < 2; ++$k) {
            $res[$k] += $dp[$k];
        }
    }
    for (my $k = 0; $k < 2; ++$k) {
        $res[$k] /= scalar(@data);
    }
    
    #print join(',', @res) . "\n\n";
    return @res;
}

sub update_model
{
    my $boxinfo = shift;
    #
    my @data = @{ $boxinfo->{'data'} };
    
    if (scalar(@data) < 2) {
        # insufficient data
        return;
    }
    # determine the min and max number of cores we have measurements for.
    my $min_p = undef;
    my $max_p = undef;
    for (my $i = 0; $i < scalar(@data); ++$i) {
        my @dp = @{ $data[$i] };
        if (not defined($min_p) or ($min_p > $dp[0])) {
            $min_p = $dp[0];
        }
        if (not defined($max_p) or ($max_p < $dp[0])) {
            $max_p = $dp[0];
        }
    }
    
    if ($min_p == $max_p) {
        # insufficient data
        return;
    }
    
    my $center = ($max_p + $min_p) / 2;
    print "  min_p:$min_p, max_p:$max_p, center:$center\n";
    
    # separate the data points into the Left and Right groups
    my @left = ();
    my @right = ();
    for (my $i = 0; $i < scalar(@data); ++$i) {
        my @dp = @{ $data[$i] };
        if ($dp[0] < $center) {
            push @left, \@dp;
        } else {
            push @right, \@dp;
        }
    }
    
    # aggregate the groups into two data points
    my @left_dp = aggregate(\@left);
    my @right_dp = aggregate(\@right);
    
    print "  left_dp:(" . join(',', @left_dp) . "), right_dp:(" . join(',', @right_dp) . ")\n";
    
    my ($p1, $p2) = ($left_dp[0], $right_dp[0]);
    my ($t1, $t2) = ($left_dp[1], $right_dp[1]);
    
    my $alpha = ($p1 * $p2 * (1 - $t1/$t2)) / ($t1/$t2 * $p1 * (1 - $p2) - $p2 * (1 - $p1));
    my $Ts = $t1 / ($alpha / $p1 + 1 - $alpha);
    
    print "  Ts:$Ts, alpha:$alpha\n";
    
    $boxinfo->{'Ts'} = $Ts;
    $boxinfo->{'alpha'} = $alpha;
}

sub find_possible_configs
{
    my $bxconfigs = shift;
    my $totcores = shift;
    my $bxnames = shift;
    my $x_bxcores = shift;
    #
    my @bxcores = @{ $x_bxcores };
    
    my $ass = sum(@bxcores);        # already assigned
    if (not defined($ass)) { $ass = 0; }
    
    if (scalar(@bxcores) + 1 < scalar(@$bxnames)) {
        # a node in the tree;
        # remaining cores
        my $rem = $totcores - $ass - (scalar(@$bxnames) - scalar(@bxcores)) + 1;
        for (my $c = 1; $c <= $rem; ++$c) {
            find_possible_configs($bxconfigs, $totcores, $bxnames, [@bxcores, $c]);
        }
    } else {
        # a leaf node; assign all the remaining cores
        my $rem = ($totcores - $ass);
        if ($rem > 0) {
            push @$bxconfigs, [ @bxcores, $rem ];
        }
    }
}

sub predict_latency
{
    my $boxinfo = shift;
    my $p = shift;
    #
    my $Ts = $boxinfo->{'Ts'};
    my $alpha = $boxinfo->{'alpha'};
    return ($Ts * ($alpha / $p + 1 - $alpha));
}

sub mapper
{
    my $boxes = shift;
    my $totcores = shift;
    #
    
    my @bxnames = sort(keys %$boxes);
    my @bxconfigs = ();
    
    find_possible_configs(\@bxconfigs, $totcores, \@bxnames, []);
    
    my @latencies = ();
    my $min_lat_i = 0;
    
    for (my $i = 0; $i < scalar(@bxconfigs); ++$i) {
        my @bxcf = @{ $bxconfigs[$i] };
        #print join(',', @bxcf) . "\n";
        my $lat = 0.0;
        for (my $k = 0; $k < scalar(@bxcf); ++$k) {
            my $bxnm = $bxnames[$k];
            $lat += predict_latency($boxes->{$bxnm}, $bxcf[$k]);
        }
        push @latencies, $lat;
        if ($latencies[$min_lat_i] > $lat) {
            $min_lat_i = $i;
        }
    }
    
    my @bestconf = @{ $bxconfigs[$min_lat_i] };
    my $minlat = $latencies[$min_lat_i];
    print "Min latency: $minlat; cfg:(" . join(',', @bestconf) . ")\n";
    
    for (my $k = 0; $k < scalar(@bestconf); ++$k) {
        my $bxnm = $bxnames[$k];
        $boxes->{$bxnm}->{'p'} = $bestconf[$k];
        $boxes->{$bxnm}->{'tp'} = predict_latency($boxes->{$bxnm}, $bestconf[$k]);
    }
}

##########################################################################################

my $boxes = {
    'workload_V1' => def_box(),
    'workload_V2' => def_box(),
    'workload_V3' => def_box()
};

my $cycle = 1;

$boxes->{'workload_V1'}->{'p'} = 1;
$boxes->{'workload_V2'}->{'p'} = 4;
$boxes->{'workload_V3'}->{'p'} = 3;

run_experiment($boxes);
get_measurements($boxes);
print_boxes($boxes);

$boxes->{'workload_V1'}->{'p'} = 6;
$boxes->{'workload_V2'}->{'p'} = 1;
$boxes->{'workload_V3'}->{'p'} = 1;

run_experiment($boxes);
get_measurements($boxes);
print_boxes($boxes);

# $boxes->{'workload_V1'}->{'p'} = 6;
# $boxes->{'workload_V1'}->{'data'} = [ [1, 21.605362], [6, 23.144419] ];
# $boxes->{'workload_V2'}->{'p'} = 1;
# $boxes->{'workload_V2'}->{'data'} = [ [4, 16.47051], [1,21.941456] ];
# $boxes->{'workload_V3'}->{'p'} = 1;
# $boxes->{'workload_V3'}->{'data'} = [ [3, 10.578793], [1, 19.704458] ];

while (1) {
    print "===== Cycle $cycle\n";
    print_boxes($boxes);
    while (my ($boxname, $boxinfo) = each(%$boxes)) {
        print "Model of $boxname:\n";
        update_model($boxinfo);
    }
    
    mapper($boxes, 8);
    
    run_experiment($boxes);
    get_measurements($boxes);
    
    $cycle += 1;
}

