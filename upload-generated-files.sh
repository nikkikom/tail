#!/bin/bash -e

#  upload-generated-files.sh
#  Tail
#
#  Created by Nikki Chumakov on 12/02/2017.
#
#  This is free and unencumbered software released into the public domain.
#
#  Anyone is free to copy, modify, publish, use, compile, sell, or
#  distribute this software, either in source code form or as a compiled
#  binary, for any purpose, commercial or non-commercial, and by any
#  means.
#

set -o errexit -o nounset

: "${BRANCHES_TO_MERGE_REGEX?}" "${BRANCH_TO_MERGE_INTO?}"
: "${GITHUB_SECRET_TOKEN?}" "${GITHUB_REPO?}"

# Save some useful information
REPO=$(git config remote.origin.url)
SSH_REPO=${REPO/https:\/\/github.com\//git@github.com:}
SHA=$(git rev-parse --verify HEAD)
REV=$(git rev-parse --short HEAD)

if ! grep -q "$BRANCHES_TO_MERGE_REGEX" <<< "$TRAVIS_BRANCH"; then
  printf "Current branch %s doesn't match regex %s, exiting\\n" \
  "$TRAVIS_BRANCH" "$BRANCHES_TO_MERGE_REGEX" >&2
  exit 0
fi
