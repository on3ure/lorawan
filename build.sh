#!/bin/bash

what=$1;
namespace="wp-lorawan"

printf "\x1b[38;5;220mBuilding [${what}]\x1b[38;5;255m\n"
printf "\x1b[38;5;104m -- oc start-build ${what}\x1b[39m\n"
pod=$(oc start-build ${what} -o name -n ${namespace})
tail=255
while [ "${tail}" != "0" ]
do
  oc logs -f ${pod} -n ${namespace}
  tail=$?;
  sleep 1;
done
printf "\x1b[38;5;104m -- oc get is/${what}\x1b[38;5;255m\n"
oc get is/${what} -o wide -n ${namespace} | grep -v "^NAME"
printf "\x1b[38;5;220mBuild complete [${what}]\x1b[39m\n"
