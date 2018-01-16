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
use IO::Socket;
use Geo::Coordinates::DecimalDegrees;

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

post '/aprs-tracker/:token/feed' => sub {
    my $self = shift;

    # reder when we are done
    $self->render_later;

    # add securityheaders
    $self->securityheaders;

    # authenticate
    my $appuser = $self->auth;
    return unless $appuser;

    my $data = $self->req->json;

    my ( $degreesn, $minutesn, $secondsn, $signn ) =
      decimal2dms( $data->{payload_fields}{latitude} );
    my ( $degreese, $minutese, $secondse, $signe ) =
      decimal2dms( $data->{payload_fields}{longitude} );

    my $type = ">"; #default car
    $type = "v" if $config->{lora}{$data->{hardware_serial}}{type} eq "van";
    $type = "k" if $config->{lora}{$data->{hardware_serial}}{type} eq "pickup";

    my $coord = sprintf(
        "%02d%02d.%02dN/%03d%02d.%02dE%1s",
        $degreesn, $minutesn, $secondsn, $degreese,
        $minutese, $secondse, $type
    );

    my $aprsServer = "belgium.aprs2.net";
    my $port       = 14580;
    my $callsign   = $config->{lora}{$data->{hardware_serial}}{callsign};
    my $pass       = $config->{lora}{$data->{hardware_serial}}{password};               # can be computed with aprspass
    my $altInFeet = $data->{payload_fields}{altitude};
    $altInFeet = 1 if $altInFeet eq 0;
    my $comment = "received with LoRa";

    my $sock = new IO::Socket::INET(
        PeerAddr => $aprsServer,
        PeerPort => $port,
        Proto    => 'tcp'
    );
    $self->log("Could not create socket: $!") unless $sock;

    my $recv_data;

    $sock->recv( $recv_data, 1024 );

    print $sock "user $callsign pass $pass ver\n";

    $sock->recv( $recv_data, 1024 );
    if ( $recv_data !~ /^# logresp $callsign verified.*/ ) {
        $self->log("Error: invalid response from server: $recv_data");
    }

    my ( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday ) = gmtime();
    my $message = sprintf( "%s>APRS,TCPIP*:@%02d%02d%02dz%s/A=%06d %s",
        $callsign, $hour, $min, $sec, $coord, $altInFeet, $comment );
    $self->log("beacon sent:" . $message);
    print $sock $message . "\n";
    close($sock);

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
