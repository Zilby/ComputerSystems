#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use IO::Handle;

use Test::Simple tests => 5;
my $MARS = "Mars4_5.jar";

sub check_sort {
    my ($nn) = @_;
    
    my @data = ();
    for (1 .. $nn) {
        push @data, int(rand 50);
    }

    my $input = "/tmp/sort-input.$$";
    open my $inph, ">", $input;
    $inph->say($nn);
    for my $dd (@data) {
        $inph->say($dd);
    }
    close $inph;

    my @srtd = sort { $a <=> $b } @data;

    open my $check, "-|", qq{java -jar $MARS isort.asm < "$input"};
    local $/ = undef;
    my $res = <$check>;

    $res =~ s/^MARS.*?Vollmar$//m;
    $res =~ s/\n/ /g;

    my $ok  = ($res =~ /@data/m && $res =~ /@srtd/m);
    say "# Correct:";
    say "# xs = @data";
    say "# ys = @srtd";
    say "# Actual:";
    say "# $res"; 
    ok($ok);
    close $check;
}

check_sort(5);
check_sort(20);
my ($aa, $bb) = (int(rand 20), int(rand 20));
check_sort($aa);
check_sort($bb);
check_sort(1);
