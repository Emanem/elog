#!/usr/bin/perl

# simple script to collate multiple
# histograms of test execution, something
# generated as
# cat test.log | sed -e 's/.*(t://g' -e 's/)//g' | sort -n | uniq -c > hist.32

use strict;
use warnings;

use Getopt::Long;

# Parse options
GetOptions();

# iterate through all files
my %data;
my $index = 0;

foreach(@ARGV) {
	open(my $fh, $_) or die "Can't open file ".$_."!";
	while(my $row = <$fh>) {
		chomp $row;
		$row =~ s/^\s+|\s+$//g;
		my @num = split / /, $row;
		if(scalar(@num) == 2) {
			#print $num[0]."\n";
			if(!defined $data{$num[1]}) {
				my $arr = [];
				for my $i (1..scalar(@ARGV)) {
    					push @$arr, 0;
				}
				$data{$num[1]} = $arr;
			}
			$data{$num[1]}[$index] = $num[0];
		}
	}
	++$index;
}

# print header
print "\t";
foreach(@ARGV) {
	print ",".$_;
}
print "\n";

# print data
foreach my $key (sort {$a<=>$b} keys %data) {
	print $key;
	foreach my $el (@{$data{$key}}) {
		print ",".$el;
	}
	print "\n";
}

