#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Test::Simple tests => 5;

system("mkdir -p /tmp/hw04test");
my $inp = "/tmp/hw04test/words.$$";
my $out = "/tmp/hw04test/words-out.$$";
my $ans = "/tmp/hw04test/words-ans.$$";

sub setup {
    my ($nn) = @_;
    system(qq{sort -R /usr/share/dict/words | head -n $nn > $inp});
    system(qq{LC_ALL="C" sort $inp > $ans});
}

sub check {
    my $diff = `diff $out $ans | wc -l`;
    return $diff == 0;
}

sub test_sort {
    my ($nn) = @_;
    setup($nn);
    system("./sort-strings $nn < $inp > $out");
    return check();
}

sub check_errors {
    my ($errs) = @_;
    chomp $errs;
    if ($errs ne "") {
        $errs =~ s/^/# /mg;
        warn $errs;
        return 0;
    }
    else {
        return 1;
    }
}

sub clang_check {
    my $errs = `clang-check -analyze *.c --`;
    return check_errors($errs);
}

sub valgrind {
    setup(50);
    my $errs = `(valgrind --leak-check=yes -q ./sort-strings 50 < $inp > $out) 2>&1`;
    return check_errors($errs);
}

ok(test_sort(10), "test with 10 words");
ok(test_sort(100), "test with 100 words");
ok(test_sort(1000), "test with 1000 words");
ok(clang_check(), "clang_check");
ok(valgrind(), "valgrind");
