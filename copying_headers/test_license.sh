#!/bin/bash

repo=$1
basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cat $repo/copying_headers/MANIFEST.* | sort > MANIFEST.sum
cat $repo/MANIFEST | sort > MANIFEST.sort
diff MANIFEST.sum MANIFEST.sort
sumcheck=$?
rm MANIFEST.sum MANIFEST.sort
if [ $sumcheck -ne 0 ]; then
    echo "Files not accounted for in license manifest files"
    exit -1
fi

cd $repo/copying_headers
tags=$(ls MANIFEST.* | sed 's|^MANIFEST.||' | grep -v EXEMPT)

for tag in $tags; do
    for path in $(cat MANIFEST.$tag); do
        $basedir/check license_$tag ../$path
        if [ $? -ne 0 ]; then
            echo "ERROR: $path missing license tag"
            exit -2
        fi
    done
done

