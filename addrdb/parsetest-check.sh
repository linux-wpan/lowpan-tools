#!/bin/sh
LEASE_FILE=${TMPDIR:/var/tmp}/izcoordinator.leases
./parsetest ${LEASE_FILE}
exit $?

