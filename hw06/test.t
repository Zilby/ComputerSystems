#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Test::Simple tests => 28;

sub run_test {
    my ($prog, $arg) = @_;
    $arg //= 10;

    my $code = system(qq{./$prog $arg > out.tmp 2> stats.tmp});

    my $stats = {};
    open my $st, "<", "stats.tmp";
    while (<$st>) {
        chomp;
        next unless /:/;
        my ($k, $v) = split /\s*:\s*/, $_;
        $stats->{$k} = 0 + $v;
    }
    close $st;

    my $outp = `cat out.tmp`;
    return $code, $stats, $outp;
}

my ($code, $stats, $outp);

use Data::Dumper;

($code, $stats, $outp) = run_test("array_sum_ints", 10);
ok($code == 0 && $outp =~ /45/, "array sum 10 = 45");
ok($code == 0 && $stats->{'malloc bytes'} >= 40, "array sum 10 malloc bytes");
ok($code == 0 && $stats->{'malloc chunks'} == 1, "array sum 10 - one chunk");

($code, $stats, $outp) = run_test("array_sum_ints", 1000);
ok($code == 0 && $outp =~ /499500/, "array sum 1k correct");
ok($code == 0 && $stats->{'malloc bytes'} >= 4000, "array sum 1k malloc bytes");
ok($code == 0 && $stats->{'malloc chunks'} == 1, "array sum 1k - one chunk");

($code, $stats, $outp) = run_test("array_sum_ints", 100000);
ok($code == 0 && $outp =~ /704982704/, "array sum 100k correct");
ok($code == 0 && $stats->{'malloc bytes'} >= 400000, "array sum 100k malloc bytes");
ok($code == 0 && $stats->{'malloc chunks'} == 2, "array sum 100k - two chunk");
ok($code == 0 && $stats->{'free chunks'} == 2, "array sum 100k - free two chunks");

($code, $stats, $outp) = run_test("list_sum_ints", 10);
ok($code == 0 && $outp =~ /45/, "list sum 10 = 45");
ok($code == 0 && $stats->{'malloc bytes'} >= 80, "list sum 10 malloc bytes");
ok($code == 0 && $stats->{'malloc chunks'} == 1, "list sum 10 - one chunk");
ok($code == 0 && $stats->{'free list length'} < 5, "list sum 10 - free list");

($code, $stats, $outp) = run_test("list_sum_ints", 1000);
ok($code == 0 && $outp =~ /499500/, "list sum 1k correct");
ok($code == 0 && $stats->{'malloc bytes'} >= 8000, "list sum 1k malloc bytes");
ok($code == 0 && $stats->{'malloc chunks'} == 1, "list sum 1k - one chunk");
ok($code == 0 && $stats->{'free list length'} < 10, "list sum 1k - free list");

($code, $stats, $outp) = run_test("list_sum_ints", 100000);
ok($code == 0 && $outp =~ /704982704/, "array sum 100k correct");
ok($code == 0 && $stats->{'malloc bytes'} >= 800000, "array sum 100k malloc bytes");
ok($code == 0 && $stats->{'malloc chunks'} > 11, "array sum 100k - multiple chunks");
ok($code == 0 && $stats->{'free list length'} < 50, "list sum 100k - free list");

($code, $stats, $outp) = run_test("random_sizes");
ok($code == 0 && $outp =~ /167312273785/, "random_sizes output as expected");
ok($code == 0 && $stats->{'malloc bytes'} >= 50000000, "random_sizes malloc bytes");
ok($code == 0 && $stats->{'malloc bytes'} <= 100000000, "random_sizes malloc bytes 2");
ok($code == 0 && $stats->{'malloc chunks'} < 500, "random_sizes malloc chunks");
ok($code == 0 && $stats->{'free chunks'}   > 20, "random_sizes free chunks");
ok($code == 0 && $stats->{'free list length'} < 500, "random_sizes - free list length");

