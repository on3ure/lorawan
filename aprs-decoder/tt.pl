#! /usr/bin/perl

use Ham::APRS::IS;
use Ham::APRS::FAP qw(parseaprs);
use Geo::Coordinates::DecimalDegrees;

$callsign = "ON3URE-1";

my $is = new Ham::APRS::IS('belgium.aprs2.net:14580', $callsign, 'passcode' => Ham::APRS::IS::aprspass($callsign), 'appid' => 'APLORA 1.2');
$is->connect('retryuntil' => 3) || die "Failed to connect: $is->{error}";


($degreesn, $minutesn, $secondsn, $sign) = decimal2dms(51.0561679);
($degreese, $minutese, $secondse, $sign) = decimal2dms(4.2099715);

#k =>pickup > =>car v => van
$type = "v";

my $coord = sprintf("%02d%02d.%02dN/%03d%02d.%02dE%1s", $degreesn,$minutesn,$secondsn,$degreese,$minutese,$secondse,$type);

print $coord . "\n";




$altInFeet = 502;
$comment = "received with LoRa";



($sec,$min,$hour,$mday,$mon,$year,$wday,$yday) = gmtime();
$message = sprintf( "%s>APLORA,TCPIP*:@%02d%02d%02dh%s/A=%06d %s",
    $callsign,$hour,$min,$sec,$coord,$altInFeet,$comment );
  $is->sendline($message);
print $message . "n";
print "beacon sent.\n";

  $is->disconnect() || die "Failed to disconnect: $is->{error}";
