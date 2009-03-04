#!/bin/sh
LEASE_DIR=${TMPDIR}
if [ -z "$LEASE_DIR" ]; then
	LEASE_DIR="/var/tmp"
fi
mkdir -p ${LEASE_DIR}
LEASE_FILE=${LEASE_DIR}/izcoordinator.leases
./parsetest ${LEASE_FILE} || exit 1
exit 0

