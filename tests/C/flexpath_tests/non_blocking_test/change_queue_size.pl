#!/usr/bin/perl -w

$^I = ".bak";
$queue_size = shift(@ARGV);

while(<>) {
    s/QUEUE_SIZE=\d+/QUEUE_SIZE=$queue_size/;
    print;
}
