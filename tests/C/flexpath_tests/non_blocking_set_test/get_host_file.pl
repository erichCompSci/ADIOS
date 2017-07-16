#!/usr/bin/perl

use strict;

my $output_from_wc = `wc -l $ARGV[0]`;
$output_from_wc =~ /(\d+)/;
my $num_nodes = $1 - 1;

my $curr_node = 1;

while(<>)
{
    if(/(.*):/)
    {
	open MPI_HOSTS, '>', "$1_mpi_hosts.temp";
	print "Skipping: $_";
	next;
    }

    print "Checking node $curr_node of $num_nodes\n";

    chomp;
    my $procs = `ssh elohrman\@$_ nproc`;
    chomp($procs);
    if($procs =~ /\d+/)
    {
	print MPI_HOSTS "$_ slots=$procs max-slots=$procs\n";
	#print "$procs\n";
    }
    else
    {
	exit(1);
    }
    
    $curr_node++;
}
