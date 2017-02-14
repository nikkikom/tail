#!/bin/bash -x

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

:  "${BRANCHES_TO_UPDATE?}" "${TRAVIS_BRANCH?}"

# Save some useful information
REPO=$(git config remote.origin.url)
GITHUB_REPO=${REPO//*github.com\//}
SSH_REPO=${REPO/https:\/\/github.com\//git@github.com:}
SHA=$(git rev-parse --verify HEAD)
REV=$(git rev-parse --short HEAD)

if ! grep -q "$BRANCHES_TO_UPDATE" <<< "$TRAVIS_BRANCH"; then
  printf "Current branch %s doesn't match regex %s, exiting\\n" \
  "$TRAVIS_BRANCH" "$BRANCHES_TO_UPDATE" >&2
  exit 0
fi

# Since Travis does a partial checkout, we need to get the whole thing
repo_temp=$(mktemp -d)
git clone "$REPO" "$repo_temp"

prev_wd=$PWD

# shellcheck disable=SC2164
cd "$repo_temp"

git config user.name "Travis CI"
git config user.email "$COMMIT_AUTHOR_EMAIL"

printf 'Checking out %s\n' "$TRAVIS_BRANCH" >&2
git checkout "$TRAVIS_BRANCH"

/bin/cp -f "$prev_wd"/build/tests/*.png "$repo_temp"/tests/

echo "TRAVIS_PULL_REQUEST=$TRAVIS_PULL_REQUEST"

printf 'Updating %s\n' "$TRAVIS_COMMIT" >&2
# git merge --ff-only "$TRAVIS_COMMIT"
git commit -m "Update performance data" tests/*.png

printf 'Pushing to %s\n' "$REPO" >&2

# push_uri="https://$GITHUB_SECRET_TOKEN@github.com/$GITHUB_REPO"

# Redirect to /dev/null to avoid secret leakage
# git push "$push_uri" "$TRAVIS_BRANCH" >/dev/null 2>&1
# git push "$push_uri" :"$TRAVIS_BRANCH" >/dev/null 2>&1
