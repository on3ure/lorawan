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
#
#    $redis->set( "teamleader-taxcode-" . $taxcode, encode_json $tldata)
#      if $taxcode;
#    $redis->set( "teamleader-email-" . $email, encode_json $tldata)
#      if ( $email && !$taxcode );
#    $redis->set( "teamleader-email-" . $email, encode_json $tldata)
#      if ( $email && !$redis->get( "teamleader-email-" . $email ) );
#
#    if (
#          !$taxcode
#        && $email
#        && !(
#            match(
#                $email, $redis->lrange( "teamleader-customers-email", 0, -1 )
#            )
#        )
#      )
#    {
#        $redis->rpush( "teamleader-customers-email",     $email );
#        $redis->rpush( "teamleader-customers-email-add", $email );
#    }
#
#    if (
#        $taxcode
#        && !(
#            match(
#                $taxcode,
#                $redis->lrange( "teamleader-customers-taxcode", 0, -1 )
#            )
#        )
#      )
#    {
#        $redis->rpush( "teamleader-customers-taxcode",     $taxcode );
#        $redis->rpush( "teamleader-customers-taxcode-add", $taxcode );
#    }

#    $self->log( '[' . __LINE__ . '][' . $appuser . '] add [' . $email . ']' )
#      if ( $email && !$taxcode );
#    $self->log( '[' . __LINE__ . '][' . $appuser . '] add [' . $taxcode . ']' )
#      if $taxcode;
    #


    $self->log(decode_json($data));
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
