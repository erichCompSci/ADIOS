#!/usr/bin/perl -w

$^I = ".bak";
$method = shift(@ARGV);

while(<>) {
    s/(.*QUEUE_SIZE=\d+;).*(<.*)/$1$method$2/;
    print;
}
