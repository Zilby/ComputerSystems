#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Test::Simple tests => 7;

sub run {
    my ($threads, $start, $count, $expect) = @_;
    system(qq{rm -f main.out valgrind.out});
    system(qq{valgrind -q --leak-check=full --log-file=valgrind.out ./main $threads $start $count > main.out});

    my $outp = `cat main.out`;
    ok($outp =~ /$expect/, "run $threads $start $count -> $expect");

    my $valg = `cat valgrind.out`;
    chomp $valg;
    ok($valg eq "", "valgrind $threads $start $count");
}

run(1, "73786976294838206464", 1, "2 2 2 2 2 2 2 2");
run(4, 65536, 1024, "227 293");

system("rm -f check.out");
system(qq{(clang-check *.c -- 2>&1) > check.out});
my $check = `cat check.out`;
chomp $check;
ok($check eq "", "clang-check");

ok(-f "report.txt", "report.txt exists");
ok(-f "graph.png", "graph.png exists");
