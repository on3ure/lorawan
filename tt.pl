#!/usr/bin/perl
#

use MIME::Base64;
use Data::Dumper;


#my $raw = "GHb1OC2XGAYpfs3t";
my $raw = "fJtMQmIAMUDNzAxC";
  
my $bin = decode_base64($raw);

print $bin . "\n";

my $hexstring = unpack('H*', $bin);
print $hexstring . "\n\n";
print "00803F00040004040\n\n";
print length($hexstring) . "\n";
#$hexstring = unpack "H*", reverse pack "H*", $hexstring;


#$hexstring = uc unpack 'H*', reverse pack 'H*', $hexstring;

print unpack "f", pack "H*", substr($hexstring, 0, 8);
print "\n";
print unpack "f", pack "H*", substr($hexstring, 8, 8);
print "\n";
print unpack "f", pack "H*", substr($hexstring, 16, 8);
print "\n";



