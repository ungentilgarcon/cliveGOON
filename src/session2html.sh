#!/bin/sh
branch="${1}"
date="${branch%-*}"
date="${date#session-}"
git checkout "${branch}"
git log --pretty=format:"%H" --reverse --after "${date}" |
tee commits.txt |
while read commit
do
  echo "<div class='commit' id='commit-$commit'>" &&
  git show --pretty=format:"diff @ %ci" $commit |
  python diffc2html.py &&
  echo "</div>"
done > diffs.html &&
osecs=0
cat commits.txt |
while read commit
do
  secs="$(date -d "$(git show --pretty=%ci $commit | head -n 1)" +%s)"
  d="$((secs-osecs))"
  if [ "${d}" -gt 9999 ] ; then d=0 ; fi
  osecs="${secs}"
  echo "#commit-$commit { margin-top: ${d}em; }"
done > commits.css &&
(
  echo "<html><head><style type='text/css'>" &&
  cat stylesheet.css commits.css &&
  echo "</style></head><body>" &&
  cat header.html diffs.html footer.html &&
  echo "</body></html>"
) > "${branch}.html"
