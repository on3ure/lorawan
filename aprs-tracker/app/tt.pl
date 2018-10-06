#!/usr/bin/perl
#
# The weepee routing lookup rest service

use v5.10;
use strict;
use utf8;
use Mojolicious::Lite;
use Mojo::UserAgent;
use Mojo::DOM;
use Mojo::JSON qw(decode_json encode_json);
use Mojolicious::Plugin::Sentry;
use Mojolicious::Plugin::RenderFile;
use Redis::Fast;
use Mojo::ByteStream 'b';
use Config::YAML;
use boolean ':all';
use match::simple qw(match);
use Data::Dumper;
use Geo::Coordinates::DecimalDegrees;
use Ham::APRS::IS;
use MIME::Base64;

# Environment var Mapping and fallback
use Env qw(REDIS_PERSISTENT_SERVICE_HOST);
$REDIS_PERSISTENT_SERVICE_HOST = '127.0.0.1'
  if ( !$REDIS_PERSISTENT_SERVICE_HOST );

# Mojo config
my $mojoconfig = plugin Config => { file => 'mojo.conf' };

# YAML based config
my $config = Config::YAML->new( config => "config.yaml" );



# Helpers


    my $data = 
   {
          'tmst' => 4185232764,
          'rssi' => -121,
          'localTime' => '2018-09-17 18:00:14',
          'packetsLeft' => 0,
          'dataRate' => 'SF12BW125',
          'frequency' => '868.1',
          'snr' => '-10.8',
          'fcnt' => '00B1',
          'packetTime' => '2018-09-17T16:00:09.614138Z',
          'payload' => 'AIJNAwiDCABIVr0=',
          'micValid' => 'true',
          'gatewayEui' => 'AA555A0000098062',
          'packetIdentifier' => '58089312738512bab92d10fd1e42e515',
          'rawData' => 'QOQVASaAsQACLhYfrt2tGxuq8ock9uLg',
          'devName' => 'gps tracker',
          'devAddr' => '260115E4'
        }; 
    

    my $hexstring = unpack('H*',decode_base64($data->{'payload'}));
   print $hexstring . "\n";

    my $latitude = (unpack "L", reverse(pack "H*", substr($hexstring, 6, 8))) * 0.000001;
    my $longitude = (unpack "L", reverse(pack "H*", substr($hexstring, 14, 8))) * 0.000001;
    my $altitude = 0;
    print "WirelessThings.be: Latitude: " . $latitude . " Longitude: " . $longitude . " Altitude:" . $altitude . "\n";

    my ( $degreesn, $minutesn, $secondsn, $signn ) =
      decimal2dms( $latitude );
    my ( $degreese, $minutese, $secondse, $signe ) =
      decimal2dms( $longitude );

    my $type = ">";    #default car
    $type = ">" if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq "car";
    $type = "v" if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq "van";
    $type = "[" if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq "walk";
    $type = "k"
      if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq "pickup";

    my $coord = sprintf(
        "%02d%02d.%02dN/%03d%02d.%02dE%1s",
        $degreesn, $minutesn, $secondsn, $degreese,
        $minutese, $secondse, $type
    );

    my $callsign  = $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{callsign};
    my $altInFeet = $altitude;
    my $comment   = "WirelessThings LoRa snr:" . $data->{snr} . " rssi:" . $data->{rssi} . " freq:" . $data->{frequency} . " gateway by ON3URE";

    my $is = new Ham::APRS::IS(
        'belgium.aprs2.net:14580', $callsign,
        'passcode' => Ham::APRS::IS::aprspass($callsign),
        'appid'    => 'APLORA 1.2'
    );
    $is->connect( 'retryuntil' => 3 );

    my( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday ) = gmtime();
    my $message = sprintf( "%s>APLORA,TCPIP*:@%02d%02d%02dh%s/A=%06d %s",
        $callsign, $hour, $min, $sec, $coord, $altInFeet, $comment );
    $is->sendline($message);

    print "WirelessThings.be Beacon Send:" . $message . "\n";

    $is->disconnect();

