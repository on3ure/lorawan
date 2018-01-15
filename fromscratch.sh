#!/bin/bash

printf "\x1b[38;5;220mNew project\x1b[38;5;255m\n"
oc new-project wp-lorawan \
    --description="Weepee LoRaWAN Services" \
    --display-name="[*] [wp] LoRaWAN Services"

printf "\x1b[38;5;220mNew secrets\x1b[38;5;255m\n"
oc secrets new-sshauth wp-lorawan --ssh-privatekey=keys/wp-lorawan
oc secrets add serviceaccount/builder secrets/wp-lorawan

printf "\x1b[38;5;220mQuota\x1b[38;5;255m\n"
oc delete -f quota.yaml
oc create -f quota.yaml

printf "\x1b[38;5;220mLimits\x1b[38;5;255m\n"
oc delete -f limits.yaml
oc create -f limits.yaml

printf "\x1b[38;5;220mPersistent storage\x1b[38;5;255m\n"
oc create -f letsencrypt/pvc.yaml
oc create -f redis-persistent/pvc.yaml

if [ "$1" -gt "restore" ]; then
  printf "\x1b[38;5;220mRestore\x1b[38;5;255m\n"
  printf "\x1b[38;5;104m -- oc create -f pvc-transfer/restore.yaml\x1b[38;5;255m\n"
  namespace="wp-lorawan"
  pod=$(oc create -f pvc-transfer/restore.yaml -o name -n ${namespace})
  tail=255
  while [ "${tail}" != "0" ]
  do
    oc logs -f ${pod} -n ${namespace}
    tail=$?;
    sleep 1;
  done
  printf "\x1b[38;5;220mRestore complete\x1b[39m\n"
fi

echo "services + dns"
oc create -f letsencrypt/svc.yaml
oc create -f letsencrypt/route.yaml
oc create -f dns-injector/pod.yaml

echo "letsencrypt"
oc create -f letsencrypt/is.yaml
oc create -f letsencrypt/bc.yaml
oc create -f letsencrypt/dc.yaml

echo "redis persistent"
oc create -f redis-persistent/svc.yaml
oc create -f redis-persistent/is.yaml
oc create -f redis-persistent/bc.yaml
oc create -f redis-persistent/dc.yaml

echo "aprs-tracker"
oc create -f aprs-tracker/svc.yaml
oc create -f aprs-tracker/is.yaml
oc create -f aprs-tracker/bc.yaml
oc create -f aprs-tracker/dc.yaml
oc create -f aprs-tracker/hpa.yaml

./rebuild.sh
