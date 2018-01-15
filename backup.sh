#!/bin/bash

namespace="wp-lorawan"

printf "\x1b[38;5;220mBackup\x1b[38;5;255m\n"
printf "\x1b[38;5;104m -- oc create -f pvc-transfer/backup.yaml\x1b[38;5;255m\n"
pod=$(oc create -f pvc-transfer/backup.yaml -o name -n ${namespace})
tail=255
while [ "${tail}" != "0" ]
do
  oc logs -f ${pod} -n ${namespace}
  tail=$?;
  sleep 1;
done
printf "\x1b[38;5;220mBackup complete\x1b[39m\n"
