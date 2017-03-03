#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Time::HiRes qw(time);
use Test::Simple tests => 5;

sub get_time {
    my $data = `cat time.tmp | grep ^real`;
    $data =~ /^real\s+(.*)$/;
    return 0 + $1;
}

my $sys = `./sum_sys 1000000`;
ok($sys =~ /1783293664/, "sum sys 1M"); 

my $hw6 = `/usr/bin/time -p -o time.tmp ./sum_hw06 1000000`;
my $tA  = get_time();
ok($hw6 =~ /1783293664/, "sum hw06 1M"); 

my $opt = `/usr/bin/time -p -o time.tmp ./sum_opt 1000000`;
my $tB  = get_time();
ok($opt =~ /1783293664/, "sum opt 1M");

ok($tB < $tA, "speedup for opt over hw06");

ok(-f "report.txt", "You have a report.txt");
