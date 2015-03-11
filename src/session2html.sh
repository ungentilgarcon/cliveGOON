#!/bin/sh
if [ "x${1}" = "x" ]
then
  echo "usage: ${0} session-branch [session-start]"
  exit 1
fi
branch="${1}"
date="$(echo "${2:-${1}}" | sed "s/session-\(....-..-..\)-\(..\)\(..\)\(..\)/\1 \2:\3:\4/")"
git log --pretty=format:"%H" --reverse --after "${date}" "${branch}" |
tee commits.txt |
while read commit
do
  echo "<div class='commit' id='commit-$commit'>" &&
  git show --pretty=format:"diff @ %ci" $commit |
  python diffc2html.py &&
  echo "</div>"
done > diffs.html &&
cat commits.txt |
(
  osecs="$(date -d "${date}" +%s)"
  while read commit
  do
    secs="$(date -d "$(git show --pretty=%ci $commit | head -n 1)" +%s)"
    delta="$((secs - osecs))"
    echo "${delta}"
  done
) > times.txt
(
  cat << EOF &&
<!DOCTYPE html>
<html>
<head>
<title>${branch}</title>
<style type='text/css'>
body {
  text-align: center;
  background-color: grey;
}
h1 {
  background-color: white;
  width: 40ex;
  margin: 1em auto;
  padding: 0.25em 1em;
  border 1px solid;
}
audio { width: 100%; }
h2 { text-align: center; }
p { text-align: justify; }
.commit {
  text-align: left;
  background-color: white;
  width: 80ex;
  margin: 2em auto;
  padding: 0 1em;
  border: 1px solid;
  display: none;
}
.commit .k, .commit .kr, .commit .kt, .commit .p { font-weight: bold; }
.commit .dh:first-child { font-weight: bold; color: black; }
.commit .dh { width: 32ex; display: inline-block; float: right; color: silver; }
.commit .dl { color: blue; }
.commit .du { color: grey; }
.commit .di { color: green; }
.commit .dd { color: red; }
</style>
<script type="text/javascript">
var times;
var commits;
function search(t) {
  var lo = 0;
  var hi = times.length - 1;
  while (lo + 1 < hi) {
    var mi = Math.floor((lo + hi) / 2);
    if (times[mi] <= t) {
      lo = mi;
    } else {
      hi = mi;
    }
  }
  return commits[lo];
}
function go() {
  var current = null;
  var player = document.getElementById("player");
  player.ontimeupdate =
    function() {
      var commit = document.getElementById("commit-" + search(player.currentTime));
      if (!!current) {
        current.style.display = "none";
      }
      if (!!commit) {
        commit.style.display = "block";
      }
      current = commit;
    };
}
times = [
0,
EOF
  cat times.txt |
  sed 's/$/,/'
  cat << EOF
1000000];
commits = [
"intro",
EOF
  cat commits.txt |
  sed 's/^/"/' |
  sed 's/$/",/'
  cat << EOF
"outro"];
</script>
</head>
<body onload="javascript:go()">
<h1>${branch}</h1>
<audio id="player" autoplay controls>
<source src="${branch}.ogg" type="audio/ogg; codecs=vorbis">
</audio>
<div class='commit' id='commit-intro'>
<h2><a href="http://code.mathr.co.uk/clive" title="clive">clive</a></h2>
<p>clive is a C audio live-coding skeleton.   It allows you to hot-swap
DSP processing callbacks, providing not much more than automatic C
code recompilation, object code reloading, and state preservation.</p>
</div>
EOF
  cat diffs.html &&
  cat << EOF
<div class='commit' id='commit-outro'>
<h2>the end</h2>
</div>
</body>
</html>
EOF
) > "${branch}.html"
