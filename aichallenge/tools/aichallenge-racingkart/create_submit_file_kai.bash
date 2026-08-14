#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

tar zcvf "${SCRIPT_DIR}/submit/aichallenge_submit.tar.gz" \
  -C "${SCRIPT_DIR}/aichallenge/workspace/src" aichallenge_submit \
  -C "${SCRIPT_DIR}/aichallenge/tools" .
