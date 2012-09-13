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

sub read_batch
{
    my $batchname = shift;
    #
    (open my $batch_fh, "<$batchname") or die "Could not open the batch file!";
    my @batch = ();
    while (<$batch_fh>) {
        chomp;
        # remove comments
        s/#.*$//g;
        #
        if (/^\s*(\d+)\s+(\S+)\s+(\S+)\s+\s+(\S+)\s+(\S+)$/) {
            my $ncyc = $1;
            my $xml = $2;
            my $fx1 = $3;
            my $fx2 = $4;
            my $fx3 = $5;
            (open my $tst, "<$xml") or die "Could not open the xml file '$xml'!";
            close $tst;
            if ($fx1 eq 'NA') {
                $fx1 = undef;
            }
            if ($fx2 eq 'NA') {
                $fx2 = undef;
            }
            if ($fx3 eq 'NA') {
                $fx3 = undef;
            }
            push @batch, { 'ncyc' => $ncyc, 'xmlf' => $xml, 'fx' => [ $fx1, $fx2, $fx3 ] };
        }
    }
    close $batch_fh;
    return \@batch;
}

sub get_batchline
{
    # find cycnum in batch
    my $batch = shift;
    my $cycnum = shift;
    #
    my $c = 1;
    for (my $i = 0; $i < scalar(@$batch); ++$i) {
        my $bl = $batch->[$i];
        $c = $c + $bl->{'ncyc'};
        if ($cycnum < $c) {
            return $bl;
        }
    }
    return undef;
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
            my $tp = $boxes->{$taskname}->{'tp'};
            my $delta = undef;
            if (defined $tp) {
                $delta = ($exetime / $tp) - 1;
            }
            my $fx = $boxes->{$taskname}->{'fx'};
            push @{$data}, [ $p, $exetime,  $delta, $fx ];
        }
    }
}

sub def_box
{
    return { 'data' => [], 'p' => 1, 'Ts' => undef, 'alpha' => undef, 'tp' => undef, 'fx' => undef, 
            'beta' => undef, 'gamma' => undef };
}

sub fstr
{
    # format value to string and handle NAs
    my $format = shift;
    my $val = shift;
    #
    if (defined $val) {
        return sprintf($format, $val);
    } else {
        return 'NA';
    }
}

sub print_boxes
{
    my $boxes = shift;
    #
    while (my ($boxname, $boxinfo) = each(%$boxes)) {
        my $p = $boxinfo->{'p'};
        my $data = $boxinfo->{'data'};
        my $Ts = $boxinfo->{'Ts'};
        my $alpha = $boxinfo->{'alpha'};
        my $beta = $boxinfo->{'beta'};
        my $gamma = $boxinfo->{'gamma'};
        my $tp = $boxinfo->{'tp'};
        my $fx = $boxinfo->{'fx'};
        
        my $strdata = '(';
        for (my $i = 0; $i < scalar(@$data); ++$i) {
            # $strdata = $strdata . '(' . $data->[$i]->[0] . '|' . $data->[$i]->[1] . ') ';
            $strdata = $strdata . sprintf('(%d|%s|%s|%s) ',
                $data->[$i]->[0], fstr('%.3f', $data->[$i]->[1]), fstr('%.3f', $data->[$i]->[2]),
                fstr('%.3f', $data->[$i]->[3]));
        }
        printf("box:%s => p:%d, Ts:%s, alpha:%s, beta:%s, gamma:%s, tp:%s, fx:%s, data:%s)\n",
                $boxname, $p, fstr('%.3f', $Ts), 
                fstr('%.3f', $alpha), fstr('%.3f', $beta), fstr('%.3f', $gamma), 
                fstr('%.3f', $tp), 
                fstr('%.3f', $fx), 
                $strdata);
    }
}

sub write_x_recfile
{
    my $fname = shift;
    my $a_Ts = shift;
    my $a_alpha = shift;
    #
    (open my $rcfh, ">$fname") or die "Could not open '$fname'!";
    print $rcfh  "[ 0,2 :\n    $a_Ts\n    $a_alpha\n    1.800000e+01\n]\n";
    close $rcfh;
}

sub run_experiment
{
    my $boxes = shift;
    my $batchln = shift;        # a config line from the batch
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
    
    # write input records files
    write_x_recfile("inputs/input-x-V1.inp", $batchln->{'fx'}->[0], 0.0);
    write_x_recfile("inputs/input-x-V2.inp", $batchln->{'fx'}->[1], 0.5);
    write_x_recfile("inputs/input-x-V3.inp", $batchln->{'fx'}->[2], 1.0);

    my $wkld = $batchln->{'xmlf'};
    my $prgline = "AMAP=$amap BMAP=$bmap CMAP=$cmap  ./gigo_3-lpel -m A <" . $wkld;
    
    #print "==============================================================\n";
    print "---- Running the snet: $prgline\n";
    my $t0 = [gettimeofday];
    my $output = qx($prgline);
    my $duration = tv_interval ( $t0, [gettimeofday]);
    
    #print "    Output: $output\n";
    print "    Execution time: $duration s\n";
}

sub extrema
{
    my $data = shift;       # data array
    my $col = shift;        # column where to find extrema
    #
    my $minv = undef;
    my $maxv = undef;
    for (my $i = 0; $i < scalar(@$data); ++$i) {
        my @dp = @{ $data->[$i] };
        if (defined $dp[$col]) {
            if (not defined($minv) or ($minv > $dp[$col])) {
                $minv = $dp[$col];
            }
            if (not defined($maxv) or ($maxv < $dp[$col])) {
                $maxv = $dp[$col];
            }
        }
    }
    return ($minv, $maxv);
}

sub separate
{
    my $data = shift;
    my $center = shift;
    my $col = shift;
    #
    my @left = ();
    my @right = ();
    for (my $i = 0; $i < scalar(@$data); ++$i) {
        my @dp = @{ $data->[$i] };
        if (defined $dp[$col]) {
            if ($dp[$col] < $center) {
                push @left, \@dp;
            } else {
                push @right, \@dp;
            }
        }
    }
    return (\@left, \@right);
}


sub aggregate
{
    my $xdata = shift;
    my $col = shift;
    #
    my @data = @{ $xdata };
    #  [$col] = p or fx, [1] = exetime
    my @res = ( $data[0]->[$col], $data[0]->[1] );
    
    #print join(',', @res) . "\n";
    
    for (my $i = 1; $i < scalar(@data); ++$i) {
        my @dp = @{ $data[$i] };
        # for (my $k = 0; $k < 2; ++$k) {
        $res[0] += $dp[$col];
        $res[1] += $dp[1];
        # }
    }
    for (my $k = 0; $k < 2; ++$k) {
        $res[$k] /= scalar(@data);
    }
    
    #print join(',', @res) . "\n\n";
    return @res;
}

sub factor_S_out
{
    my $data = shift;
    my $alpha = shift;
    #
    for (my $i = 0; $i < scalar(@$data); ++$i) {
        my $p = $data->[$i]->[0];
        my $S = $alpha / $p + 1 - $alpha;
        $data->[$i]->[1] /= $S;
    }
}

sub factor_Tseq_out
{
    my $data = shift;
    my $beta = shift;
    my $gamma = shift;
    #
    for (my $i = 0; $i < scalar(@$data); ++$i) {
        my $fx = $data->[$i]->[3];
        if (defined $fx) {
            my $Tseq = $beta * $fx + $gamma;
            $data->[$i]->[1] /= $Tseq;
        }
    }
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

    # determine the min and max fx predictor
    my ($min_fx, $max_fx) = extrema(\@data, 3);

    if (defined $boxinfo->{'alpha'} and defined($min_fx) and defined($max_fx) and ($min_fx != $max_fx)) {
        my $center_fx = ($max_fx + $min_fx) / 2;
        print "  min_fx:$min_fx, max_fx:$max_fx, center_fx:$center_fx\n";

        # separate the data points into the Left and Right groups
        my ($left_fx, $right_fx) = separate(\@data, $center_fx, 3);

        factor_S_out($left_fx, $boxinfo->{'alpha'});
        factor_S_out($right_fx, $boxinfo->{'alpha'});

        # aggregate the groups into two data points
        my @left_fx_dp = aggregate($left_fx, 3);
        my @right_fx_dp = aggregate($right_fx, 3);
        
        print "  left_fx_dp:(" . join(',', @left_fx_dp) . "), right_fx_dp:(" . join(',', @right_fx_dp) . ")\n";
        
        my ($fx1, $fx2) = ($left_fx_dp[0], $right_fx_dp[0]);
        my ($t1, $t2) = ($left_fx_dp[1], $right_fx_dp[1]);

        my $beta = ($t2 - $t1) / ($fx2 - $fx1);
        my $gamma = $t1 - $beta * $fx1;

        print "  beta:$beta, gamma:$gamma\n";

        $boxinfo->{'beta'} = $beta;
        $boxinfo->{'gamma'} = $gamma;
    }


    # determine the min and max number of cores we have measurements for.
    my ($min_p, $max_p) = extrema(\@data, 0);
    
    if ($min_p == $max_p) {
        # insufficient data
        return;
    }
    
    my $center_p = ($max_p + $min_p) / 2;
    print "  min_p:$min_p, max_p:$max_p, center_p:$center_p\n";
    
    # separate the data points into the Left and Right groups
    my ($left, $right) = separate(\@data, $center_p, 0);
    
    if (defined $boxinfo->{'beta'} and defined $boxinfo->{'gamma'}) {
        factor_Tseq_out($left, $boxinfo->{'beta'}, $boxinfo->{'gamma'});
        factor_Tseq_out($right, $boxinfo->{'beta'}, $boxinfo->{'gamma'});
    }

    # aggregate the groups into two data points
    my @left_dp = aggregate($left, 0);
    my @right_dp = aggregate($right, 0);
    
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
    my $beta = $boxinfo->{'beta'};
    my $gamma = $boxinfo->{'gamma'};
    my $fx = $boxinfo->{'fx'};

    if (defined $beta and defined $gamma and defined $fx) {
        $Ts = $beta * $fx + $gamma;
    }

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

my $batchfname = shift;
print "Batch file: $batchfname\n";
my $batch = read_batch($batchfname);

my $boxes = {
    'workload_V1' => def_box(),
    'workload_V2' => def_box(),
    'workload_V3' => def_box()
};

my $cycle = 1;

# $boxes->{'workload_V1'}->{'p'} = 1;
# $boxes->{'workload_V2'}->{'p'} = 4;
# $boxes->{'workload_V3'}->{'p'} = 3;
my $batchln = get_batchline($batch, $cycle);

$boxes->{'workload_V1'}->{'p'} = 1;
$boxes->{'workload_V2'}->{'p'} = 1;
$boxes->{'workload_V3'}->{'p'} = 1;
# get the 'prediction' of the next run
$boxes->{'workload_V1'}->{'fx'} = $batchln->{'fx'}->[0];
$boxes->{'workload_V2'}->{'fx'} = $batchln->{'fx'}->[1];
$boxes->{'workload_V3'}->{'fx'} = $batchln->{'fx'}->[2];

run_experiment($boxes, $batchln);
get_measurements($boxes);
print_boxes($boxes);

$cycle += 1;

# $boxes->{'workload_V1'}->{'p'} = 6;
# $boxes->{'workload_V2'}->{'p'} = 1;
# $boxes->{'workload_V3'}->{'p'} = 1;

$batchln = get_batchline($batch, $cycle);
$boxes->{'workload_V1'}->{'p'} = 2;
$boxes->{'workload_V2'}->{'p'} = 2;
$boxes->{'workload_V3'}->{'p'} = 2;
# get the 'prediction' of the next run
$boxes->{'workload_V1'}->{'fx'} = $batchln->{'fx'}->[0];
$boxes->{'workload_V2'}->{'fx'} = $batchln->{'fx'}->[1];
$boxes->{'workload_V3'}->{'fx'} = $batchln->{'fx'}->[2];

run_experiment($boxes, $batchln);
get_measurements($boxes);
print_boxes($boxes);

# $boxes->{'workload_V1'}->{'p'} = 6;
# $boxes->{'workload_V1'}->{'data'} = [ [1, 21.605362], [6, 23.144419] ];
# $boxes->{'workload_V2'}->{'p'} = 1;
# $boxes->{'workload_V2'}->{'data'} = [ [4, 16.47051], [1,21.941456] ];
# $boxes->{'workload_V3'}->{'p'} = 1;
# $boxes->{'workload_V3'}->{'data'} = [ [3, 10.578793], [1, 19.704458] ];

$cycle += 1;

while (1) {
    print "===== Cycle $cycle\n";
    $batchln = get_batchline($batch, $cycle);
    if (not defined($batchln)) {
        exit 0;
    }

    print_boxes($boxes);
    while (my ($boxname, $boxinfo) = each(%$boxes)) {
        print "Model of $boxname:\n";
        update_model($boxinfo);
    }
    
    # get the 'prediction' of the next run
    $boxes->{'workload_V1'}->{'fx'} = $batchln->{'fx'}->[0];
    $boxes->{'workload_V2'}->{'fx'} = $batchln->{'fx'}->[1];
    $boxes->{'workload_V3'}->{'fx'} = $batchln->{'fx'}->[2];

    mapper($boxes, 8);
    
    run_experiment($boxes, $batchln);
    get_measurements($boxes);
    
    $cycle += 1;
}

