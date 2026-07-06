#!/bin/bash
# run.sh -- classify extracted ruby/spec examples against spinel.
#
# Usage: tools/rubyspec/run.sh EXTRACTED_DIR [RESULTS_TSV]
#
# Per example: compile with spinel, run, and classify:
#   PASS         compiled, ran, MSPEC-DONE fail=0
#   FAIL         compiled, ran, some expectation failed
#   REJECT       spinel refused to compile (diagnostic recorded + clustered)
#   ERROR        compiled but crashed / timed out / no MSPEC-DONE line
#   HARNESS-SKEW CRuby itself does not pass the extracted program -- the
#                extraction changed meaning; excluded from spinel's score.
#
# Output: one TSV row per example + a summary + the reject-reason ranking,
# which is the "what to implement next" list this harness exists to produce.
set -u
DIR="${1:?usage: run.sh EXTRACTED_DIR [out.tsv]}"
OUT="${2:-$DIR/results.tsv}"
SPINEL="${SPINEL:-bin/spinel}"
TDIR=$(mktemp -d /tmp/rubyspec-run.XXXXXX)
trap 'rm -rf "$TDIR"' EXIT
: > "$OUT"

pass=0; fail=0; reject=0; error=0; skew=0
for f in "$DIR"/*.rb; do
  bn=$(basename "$f" .rb)
  # CRuby oracle first: a skewed extraction must not count against spinel.
  cr=$(timeout 10 ruby "$f" 2>/dev/null | tail -1)
  if ! grep -q "fail=0" <<<"$cr"; then
    echo -e "$bn\tHARNESS-SKEW\t${cr:-crash}" >> "$OUT"; skew=$((skew+1)); continue
  fi
  diag=$("$SPINEL" "$f" -o "$TDIR/x" 2>&1 >/dev/null)
  if [ ! -x "$TDIR/x" ]; then
    reason=$(grep -oE "unsupported [^:]*|Parse errors|cannot [a-z ]*|error: [^(]*" <<<"$diag" | head -1)
    echo -e "$bn\tREJECT\t${reason:-unknown}" >> "$OUT"; reject=$((reject+1)); continue
  fi
  run=$(timeout 10 "$TDIR/x" 2>&1); rc=$?
  last=$(tail -1 <<<"$run")
  rm -f "$TDIR/x"
  if [ $rc -ne 0 ] || ! grep -q "MSPEC-DONE" <<<"$last"; then
    echo -e "$bn\tERROR\trc=$rc" >> "$OUT"; error=$((error+1))
  elif grep -q "fail=0" <<<"$last"; then
    echo -e "$bn\tPASS\t$last" >> "$OUT"; pass=$((pass+1))
  else
    echo -e "$bn\tFAIL\t$last" >> "$OUT"; fail=$((fail+1))
  fi
done

total=$((pass+fail+reject+error))
echo "rubyspec: $pass PASS / $fail FAIL / $reject REJECT / $error ERROR  (of $total; +$skew harness-skew excluded)"
echo "--- top reject reasons ---"
awk -F'\t' '$2=="REJECT"{print $3}' "$OUT" | sort | uniq -c | sort -rn | head -12
