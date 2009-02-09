#!/bin/sh
LEASE_DIR=${TMPDIR:/var/tmp}
mkdir -p ${LEASE_DIR}
LEASE_FILE=${LEASE_FILE}/izcoordinator.leases
./parsetest ${LEASE_FILE}
exit $?

