#! /usr/bin/perl

use IO::Socket;

use Geo::Coordinates::DecimalDegrees;

($degreesn, $minutesn, $secondsn, $sign) = decimal2dms(51.0561679);
($degreese, $minutese, $secondse, $sign) = decimal2dms(4.2099715);

#k =>pickup > =>car v => van
$type = "v";

my $coord = sprintf("%02d%02d.%02dN/%03d%02d.%02dE%1s", $degreesn,$minutesn,$secondsn,$degreese,$minutese,$secondse,$type);

print $coord . "\n";





$aprsServer = "finland.aprs2.net";
$port = 14580;
$callsign = "ON3URE-1";
$pass = "23996"; # can be computed with aprspass
#$coord = "4738.48N/01818.15E/k";
$altInFeet = 502;
$comment = "received with LoRa";

my $sock = new IO::Socket::INET (
				    PeerAddr => $aprsServer,
				    PeerPort => $port,
				    Proto => 'tcp'
				);
die( "Could not create socket: $!\n" ) unless $sock;

$sock->recv( $recv_data,1024 );

print $sock "user $callsign pass $pass ver\n";

$sock->recv( $recv_data,1024 );
if( $recv_data !~ /^# logresp $callsign verified.*/ )
{
    die( "Error: invalid response from server: $recv_data\n" );
}

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday) = gmtime();
$message = sprintf( "%s>APRS,TCPIP*:@%02d%02d%02dz%s/A=%06d %s\n",
    $callsign,$hour,$min,$sec,$coord,$altInFeet,$comment );
print $sock $message;
close( $sock );

print "beacon sent.\n"
