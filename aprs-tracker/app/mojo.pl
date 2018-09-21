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

# Plugins
plugin 'sentry' => {
    sentry_dsn  => $config->{sentry}->{dsn},
    server_name => $config->{openshift}->{namespace},
    logger      => 'root',
    platform    => 'perl'
};

# RenderFileplugin
plugin 'RenderFile';

my $redis = Redis::Fast->new(
    reconnect => 2,
    every     => 100,
    server    => $REDIS_PERSISTENT_SERVICE_HOST . ':6379',
    encoding  => 'utf8'
);

# Helpers

# log add papertrail later
helper log => sub {
    my $self = shift;
    my $data = shift;

    app->log->info($data);
    return;
};

# whois
helper whois => sub {
    my $self    = shift;
    my $agent   = $self->req->headers->user_agent || 'Anonymous';
    my $ip      = '127.0.0.1';
    my $forward = 'none';
    $ip = $self->req->headers->header('X-Real-IP')
      if $self->req->headers->header('X-Real-IP');
    $ip = $self->req->headers->header('X-Forwarded-For')
      if $self->req->headers->header('X-Forwarded-For');
    return $agent . " (" . $ip . ")";
};

# securityheaders
helper securityheaders => sub {
    my $self = shift;
    foreach my $key (
        keys %{ $config->{ 'securityheaders' . "-" . $config->{environment} } }
      )
    {
        $self->res->headers->append( $key =>
              $config->{ 'securityheaders' . "-" . $config->{environment} }
              ->{$key} );
    }
    return;
};

# token authentication
helper auth => sub {
    my $self = shift;
    if ( !defined( $config->{tokens}->{ $self->stash('token') } ) ) {
        app->log->error( 'authentication denied by token ['
              . $self->stash('token')
              . '] token unknown' );

        $self->render(
            json => {
                'errorcode' => '-1',
                'message'   => 'wrong token',
                'status'    => 'ERROR'
            }
        );

        return;
    }

    my $appuser = $config->{tokens}->{ $self->stash('token') };
    app->log->info( '['
          . __LINE__ . '] ['
          . $appuser
          . '] authenticated by token ['
          . $self->stash('token')
          . ']' )
      if $config->{debug} eq 'true';
    app->log->info( '['
          . __LINE__ . '] ['
          . $appuser
          . '] client info ['
          . $self->whois
          . ']' )
      if $config->{debug} eq 'true';

    return $appuser;
};

# Web API
options '*' => sub {
    my $self = shift;

    $self->securityheaders;
    $self->respond_to( any => { data => '', status => 200 } );
};

get '/' => sub {
    my $self = shift;

    return $self->swagger;
};

get '/healthz' => sub {
    my $self = shift;
    app->log->info('[liveness probe]')
      if $config->{debug} eq 'true';

    if ( $redis->ping() eq 'PONG' ) {
        return $self->render( json => 'Healty', status => '399' );
    }
    else {
        return $self->render( json => 'Broken', status => '599' );
    }
};

post '/aprs-tracker/:token/feed/enco_io' => sub {
    my $self = shift;

    # reder when we are done
    $self->render_later;

    # add securityheaders
    $self->securityheaders;

    # authenticate
    my $appuser = $self->auth;
    return unless $appuser;

    my $data = $self->req->json;

    $self->log( Dumper $data);

    my $hexstring = unpack( 'H*', decode_base64( $data->{payload} ) );
    my $latitude  = unpack "f", pack "H*", substr( $hexstring, 0,  8 );
    my $longitude = unpack "f", pack "H*", substr( $hexstring, 8,  8 );
    my $altitude  = unpack "f", pack "H*", substr( $hexstring, 16, 8 );
    $self->log( "enco.io: Latitude: "
          . $latitude
          . " Longitude: "
          . $longitude
          . " Altitude:"
          . $altitude );

    my ( $degreesn, $minutesn, $secondsn, $signn ) = decimal2dms($latitude);
    my ( $degreese, $minutese, $secondse, $signe ) = decimal2dms($longitude);

    my $type = ">";    #default car
    $type = "v" if $config->{lora}{enco_io}{type} eq "van";
    $type = "k"
      if $config->{lora}{enco_io}{type} eq "pickup";

    my $coord = sprintf(
        "%02d%02d.%02dN/%03d%02d.%02dE%1s",
        $degreesn, $minutesn, $secondsn, $degreese,
        $minutese, $secondse, $type
    );

    my $callsign  = $config->{lora}{enco_io}{callsign};
    my $altInFeet = $altitude;
    my $comment   = "enco.io LoRa";

    my $is = new Ham::APRS::IS(
        'belgium.aprs2.net:14580', $callsign,
        'passcode' => Ham::APRS::IS::aprspass($callsign),
        'appid'    => 'APLORA 1.2'
    );
    $is->connect( 'retryuntil' => 3 )
      || $self->log("Failed to connect: $is->{error}");

    my ( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday ) = gmtime();
    my $message = sprintf( "%s>APLORA,TCPIP*:@%02d%02d%02dh%s/A=%06d %s",
        $callsign, $hour, $min, $sec, $coord, $altInFeet, $comment );
    $is->sendline($message);

    $self->log( "enco_io Beacon Send:" . $message );

    $is->disconnect() || $self->log("Failed to disconnect: $is->{error}");

    $self->render(
        json => {
            'add' => 'ok'
        }
    );
};

post '/aprs-tracker/:token/feed/wirelessthings_be' => sub {
    my $self = shift;

    # reder when we are done
    $self->render_later;

    # add securityheaders
    $self->securityheaders;

    # authenticate
    my $appuser = $self->auth;
    return unless $appuser;

    my $data = $self->req->json;

    $self->log( Dumper $data);

    my $wirelessthings_be = {
        'localTime'        => '2018-01-17 11:51:09',
        'rawData'          => 'QMYcASaADwAIWAHnX4dhH/kKuytgqMWAng==',
        'payload'          => 'XTlMQiC9hkCamRlB',
        'snr'              => -18,
        'devAddr'          => '26011CC6',
        'rssi'             => -113,
        'packetIdentifier' => 'b9f72d29ffbd706ddc7a2f4f9ba3546e',
        'tmst'             => 153373252,
        'frequency'        => '867.5',
        'packetTime'       => '2018-01-17T10:51:07.749087Z',
        'micValid'         => 'true',
        'dataRate'         => 'SF12BW125',
        'fcnt'             => '000F',
        'packetsLeft'      => 0,
        'gatewayEui'       => 'AA555A0000094223'
    };

    if ( !$data->{payload} ) {
        $self->log("not what we need wrong json data");
        return $self->render(
            json => {
                'add' => 'nok'
            }
          )
          if ( !$config->{lora}{wirelessthings_be}{ $data->{devAddr} } ) {
            $self->log("wrong device");
            return $self->render(
                json => {
                    'add' => 'nok'
                }
            );
        }

        my $hexstring = unpack( 'H*', decode_base64( $data->{payload} ) );
        my $latitude =
          ( unpack "L", reverse( pack "H*", substr( $hexstring, 6, 8 ) ) ) *
          0.000001;
        my $longitude =
          ( unpack "L", reverse( pack "H*", substr( $hexstring, 14, 8 ) ) ) *
          0.000001;
        my $altitude = 0;
        $self->log( "WirelessThings.be: Latitude: "
              . $latitude
              . " Longitude: "
              . $longitude
              . " Altitude:"
              . $altitude );

        my ( $degreesn, $minutesn, $secondsn, $signn ) = decimal2dms($latitude);
        my ( $degreese, $minutese, $secondse, $signe ) =
          decimal2dms($longitude);

        my $type = ">";    #default car
        $type = ">"
          if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq
          "car";
        $type = "v"
          if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq
          "van";
        $type = "["
          if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq
          "walk";
        $type = "k"
          if $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{type} eq
          "pickup";

        my $coord = sprintf(
            "%02d%02d.%02dN/%03d%02d.%02dE%1s",
            $degreesn, $minutesn, $secondsn, $degreese,
            $minutese, $secondse, $type
        );

        my $callsign =
          $config->{lora}{wirelessthings_be}{ $data->{devAddr} }{callsign};
        my $altInFeet = $altitude;
        my $comment =
            "WirelessThings LoRa snr:"
          . $data->{snr}
          . " rssi:"
          . $data->{rssi}
          . " freq:"
          . $data->{frequency}
          . " gateway by ON3URE";

        my $is = new Ham::APRS::IS(
            'belgium.aprs2.net:14580', $callsign,
            'passcode' => Ham::APRS::IS::aprspass($callsign),
            'appid'    => 'APLORA 1.2'
        );

        $is->connect( 'retryuntil' => 3 )
          || $self->log("Failed to connect: $is->{error}");

        my ( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday ) = gmtime();
        my $message = sprintf( "%s>APLORA,TCPIP*:@%02d%02d%02dh%s/A=%06d %s",
            $callsign, $hour, $min, $sec, $coord, $altInFeet, $comment );
        $is->sendline($message);

        $self->log( "WirelessThings.be Beacon Send:" . $message );

        $is->disconnect() || $self->log("Failed to disconnect: $is->{error}");

        $self->render(
            json => {
                'add' => 'ok'
            }
        );
    }

    post '/aprs-tracker/:token/feed/thethingsnetwork_org' => sub {
        my $self = shift;

        # reder when we are done
        $self->render_later;

        # add securityheaders
        $self->securityheaders;

        # authenticate
        my $appuser = $self->auth;
        return unless $appuser;

        my $data = $self->req->json;

        $self->log( Dumper $data);

        my $thethingsnetwork_org = {
            'downlink_url' =>
'https://integrations.thethingsnetwork.org/ttn-eu/api/v2/down/wim_app/aprs_api?key=ttn-account-v2.RV7KwZhm7UZ6cDSilrn6tVUtkZBL6f_H9si8zPU0j0c',
            'hardware_serial' => '0042DD9AD323EF0E',
            'port'            => 1,
            'dev_id'          => 'groove_tph',
            'app_id'          => 'wim_app',
            'metadata'        => {
                'time'     => '2018-09-21T07:06:05.581602641Z',
                'gateways' => [
                    {
                        'rf_chain'  => 0,
                        'snr'       => '-14.2',
                        'channel'   => 3,
                        'time'      => '',
                        'gtw_id'    => 'eui-aa555a0000090171',
                        'timestamp' => 1343097572,
                        'rssi'      => -114
                    },
                    {
                        'gtw_id'          => 'eui-b827ebfffe308475',
                        'location_source' => 'registry',
                        'time'            => '2018-09-21T07:06:05.55562Z',
                        'snr'             => '-11.5',
                        'channel'         => 2,
                        'longitude'       => '4.702314',
                        'latitude'        => '50.86965',
                        'altitude'        => 5,
                        'rf_chain'        => 1,
                        'rssi'            => -120,
                        'timestamp'       => 4169756444
                    },
                    {
                        'snr'       => '-2.2',
                        'rf_chain'  => 0,
                        'channel'   => 3,
                        'time'      => '',
                        'gtw_id'    => 'eui-aa555a0000090171',
                        'timestamp' => 4205298508,
                        'rssi'      => -115
                    }
                ],
                'data_rate'   => 'SF12BW125',
                'frequency'   => '868.5',
                'coding_rate' => '4/5',
                'modulation'  => 'LORA'
            },
            'counter'     => 2,
            'payload_raw' => 'fgkAQAoAAAEWlXhETA=='
        };

        if ( !$data->{payload_raw} && $data->{'dev_id'} ) {
            $self->log("not what we need wrong json data");
            return $self->render(
                json => {
                    'add' => 'nok'
                }
            );
        }
        if (
            !$config->{lora}{thethingsnetwork_org}{ $data->{hardware_serial} } )
        {
            $self->log("wrong device");
            return $self->render(
                json => {
                    'add' => 'nok'
                }
            );
        }

        my $hexstring = unpack( 'H*', decode_base64( $data->{payload_raw} ) );
        my $latitude =
          ( unpack "L", reverse( pack "H*", substr( $hexstring, 6, 8 ) ) ) *
          0.000001;
        my $longitude =
          ( unpack "L", reverse( pack "H*", substr( $hexstring, 14, 8 ) ) ) *
          0.000001;
        my $altitude = 0;
        $self->log( "TTN: Latitude: "
              . $latitude
              . " Longitude: "
              . $longitude
              . " Altitude:"
              . $altitude );

        my ( $degreesn, $minutesn, $secondsn, $signn ) = decimal2dms($latitude);
        my ( $degreese, $minutese, $secondse, $signe ) =
          decimal2dms($longitude);

        my $type = ">";    #default car
        $type = ">"
          if $config->{lora}{thethingsnetwork_org}{ $data->{hardware_serial} }
          {type} eq "car";
        $type = "v"
          if $config->{lora}{thethingsnetwork_org}{ $data->{hardware_serial} }
          {type} eq "van";
        $type = "["
          if $config->{lora}{thethingsnetwork_org}{ $data->{hardware_serial} }
          {type} eq "walk";
        $type = "k"
          if $config->{lora}{thethingsnetwork_org}{ $data->{hardware_serial} }
          {type} eq "pickup";

        my $coord = sprintf(
            "%02d%02d.%02dN/%03d%02d.%02dE%1s",
            $degreesn, $minutesn, $secondsn, $degreese,
            $minutese, $secondse, $type
        );

        my $callsign =
          $config->{lora}{thethingsnetwork_org}{ $data->{hardware_serial} }
          {callsign};
        my $altInFeet = $altitude;
        my $comment =
            "TTN LoRa snr:"
          . $data->{metadata}{gateways}[0]{snr}
          . " rssi:"
          . $data->{metadata}{gateways}[0]{rssi}
          . " freq:"
          . $data->{metadata}{frequency}
          . " gateway by ON3URE";

        my $is = new Ham::APRS::IS(
            'belgium.aprs2.net:14580', $callsign,
            'passcode' => Ham::APRS::IS::aprspass($callsign),
            'appid'    => 'APLORA 1.2'
        );

        $is->connect( 'retryuntil' => 3 )
          || $self->log("Failed to connect: $is->{error}");

        my ( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday ) = gmtime();
        my $message = sprintf( "%s>APLORA,TCPIP*:@%02d%02d%02dh%s/A=%06d %s",
            $callsign, $hour, $min, $sec, $coord, $altInFeet, $comment );
        $is->sendline($message);

        $self->log( "WirelessThings.be Beacon Send:" . $message );

        $is->disconnect() || $self->log("Failed to disconnect: $is->{error}");

        $self->render(
            json => {
                'add' => 'ok'
            }
        );
    };

    app->start;

__DATA__

@@  exception.html.ep
% sentryCaptureMessage $exception;

@@  exception.development.html.ep
% sentryCaptureMessage $exception;

@@  not_found.development.html.ep
<!DOCTYPE html>
<html>
<head><title>Page not found</title></head>
<body>Page not found</body>
</html>
