#!/usr/bin/perl

@lines = <>;
$text = join '', @lines;
$text =~ s/\/[*].+?[*]\///sg;
$text =~ s/\n(\s+\n)+/\n/g;
print $text;
