#!/bin/bash

DDWRT_USER=admin
DDWRT_PASSWORD=admin #set the actual password of the ddwrt router
DDWRT_FETCH_URL=http://192.168.1.1/fetchif.cgi?ppp0
curl -s -u $DDWRT_USER:$DDWRT_PASSWORD $DDWRT_FETCH_URL
