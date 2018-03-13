#!/usr/bin/env bash
#set +e

msg=`ps -ef|grep daemon|grep work`

echo "${msg} hello world"

