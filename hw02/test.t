#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Test::Simple tests => 16;
my $MARS = "Mars4_5.jar";

sub check_calc {
    my ($op, $aa, $bb, $yy) = @_;
    open my $check, "-|", qq{printf -- "$op\n$aa\n$bb\n" | java -jar $MARS calculator.asm};
    local $/ = undef;
    my $res = <$check>;
    $res =~ s/^MARS.*?Vollmar$//m;
    $res =~ s/\n/ /g;
    my $ok  = $res =~ /$yy/;
    say "# ($op $aa $bb) => $yy";
    say "# $res";
    ok($ok);
    close $check;
}

check_calc("+", 10, 15, 25);

my ($aa, $bb) = (int(rand 20), int(rand 20));
check_calc("+", 200, 38, 238);
check_calc("+", $aa, $bb, $aa + $bb);

check_calc("-", 15, 30, -15);
check_calc("-", $aa, $bb, $aa - $bb);

check_calc("*", 3, 5, 15);
check_calc("*", $aa, $bb, $aa * $bb);

check_calc("/", 10, 2, 5);
check_calc("/", 41, 4, 10);
check_calc("/", $aa, $bb, int($aa / $bb));


sub check_fib {
    my ($xx, $yy) = @_;
    open my $check, "-|", qq{printf -- "$xx\n" | java -jar $MARS fib.asm};
    local $/ = undef;
    my $res = <$check>;
    $res =~ s/^MARS.*?Vollmar$//m;
    $res =~ s/\n/ /g;
    my $ok  = $res =~ /$yy/;
    say "# (fib $xx) => $yy";
    say "# $res";
    ok($ok);
    close $check;
}

sub fib {
    my ($xx) = @_;
    return 0 if ($xx == 0);
    return 1 if ($xx == 1);
    return fib($xx - 1) + fib($xx - 2);
}

for (my $ii = 1; $ii < 18; $ii += 4) {
    check_fib($ii, fib($ii));
}

check_fib(20, 6765);

