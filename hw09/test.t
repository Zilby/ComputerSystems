#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';
use POSIX ":sys_wait_h";

use Test::Simple tests => 8;

sub run {
    my ($threads, $start, $count, $expect) = @_;
    system(qq{rm -f main.out valgrind.out});

    my $cpid = fork();
    my $code = 0;
    if ($cpid) {
        my $ii = 0;
        while (waitpid($cpid, WNOHANG) == 0) {
            sleep 1;
            if (++$ii > 30) {
                say "# timeout";
                kill 'KILL', $cpid;
                system("killall memcheck-amd64-");
            }
        }
        $code = $?;
    }
    else {
        exec(qq{bash -c 'exec valgrind -q --leak-check=full --log-file=valgrind.out ./main $threads $start $count > main.out'});
        die "giving up";
    }

    my $outp = `cat main.out`;
    ok($outp =~ /$expect/ && $code == 0, "run $threads $start $count -> $expect");

    my $valg = `cat valgrind.out`;
    chomp $valg;
    ok($valg eq "", "valgrind $threads $start $count");
}

run(1, "4611686018427387904", 1, "2 2 2 2 2 2 2 2");
run(4, 65536, 1024, "227 293");

system("rm -f check.out");
system(qq{(clang-check *.c -- 2>&1) > check.out});
my $check = `cat check.out`;
chomp $check;
ok($check eq "", "clang-check");

ok(-f "report.txt", "report.txt exists");
ok(-f "graph.png", "graph.png exists");

my $procs = `ps | grep memcheck`;
chomp $procs;

ok($procs eq "", "no stray processes after tests");
if ($procs ne "") {
  system("killall memcheck-amd64-");
}
