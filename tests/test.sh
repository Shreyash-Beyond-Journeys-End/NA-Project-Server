#!/bin/bash
# needs nc and xxd. frames are built by hand from SPEC.md, no client involved
PORT=9100
cd "$(dirname "$0")/.."
./bserve ./www $PORT > server.log 2>&1 &
SERVER=$!
sleep 1
trap "kill $SERVER 2>/dev/null" EXIT

pass=0
fail=0
check() {
    if [ "$2" = "0" ]; then echo "PASS  $1"; pass=$((pass+1)); else echo "FAIL  $1"; fail=$((fail+1)); fi
}

hexof() { printf '%s' "$1" | xxd -p | tr -d '\n'; }
h16() { printf '%04x' "$1"; }
filehex() { xxd -p "$1" | tr -d '\n'; }

rawreq() {
    local payload="81$(h16 ${#1})$(hexof "$1")82$(h16 ${#2})$(hexof "$2")"
    printf '%06x0101%08x%s' $(( ${#payload} / 2 )) "$3" "$payload"
}
send() { echo -n "$1" | xxd -r -p | nc -w 1 localhost $PORT | xxd -p | tr -d '\n'; }
statuses() { echo "$1" | grep -oE '830003[0-9a-f]{6}' | sed 's/830003//' | tr '\n' ' '; }

out=$(send "$(rawreq GET /index.html 1)")
[ "$(statuses "$out")" = "323030 " ]; check "GET /index.html -> 200" $?
echo "$out" | grep -q "$(filehex www/index.html)"; check "body bytes are the file" $?

out=$(send "$(rawreq GET / 1)")
echo "$out" | grep -q "$(filehex www/index.html)"; check "/ gives index.html" $?

out=$(send "$(rawreq GET /sub/page.txt 1)")
echo "$out" | grep -q "$(filehex www/sub/page.txt)"; check "file in sub folder" $?

out=$(send "$(rawreq GET /big.bin 1)")
echo "$out" | grep -q "$(head -c 16384 www/big.bin | xxd -p | tr -d '\n')"; check "big file, first 16384 bytes in one DATA frame" $?
echo "$out" | grep -q "0006a0000100000001"; check "last DATA frame (1696 bytes) has END_STREAM" $?

out=$(send "$(rawreq GET /empty.txt 1)")
[ "$(statuses "$out")" = "323030 " ] && echo "$out" | grep -q '8a000a6b6565702d616c697665$'; check "empty file: HEADERS only, nothing after it" $?

out=$(send "$(rawreq GET /index.html 7)")
echo "$out" | grep -q '^0000[0-9a-f]\{2\}010000000007'; check "response echoes stream id" $?

out=$(send "$(rawreq GET /nope 1)")
[ "$(statuses "$out")" = "343034 " ]; check "missing file -> 404" $?

out=$(send "$(rawreq POST /index.html 1)")
[ "$(statuses "$out")" = "343035 " ]; check "POST -> 405" $?

out=$(send "$(rawreq GET /../secret 1)")
[ "$(statuses "$out")" = "343033 " ]; check "dot dot path -> 403" $?

out=$(send "$(rawreq GET index.html 1)")
[ "$(statuses "$out")" = "343030 " ]; check "path without leading slash -> 400" $?

out=$(send "$(rawreq GET /index.html 1)$(rawreq GET /nope 3)$(rawreq GET /about.html 5)")
[ "$(statuses "$out")" = "323030 343034 323030 " ]; check "3 requests on one connection" $?

bad="000001010100000001""05"
out=$(send "$bad$(rawreq GET /index.html 3)")
[ "$(statuses "$out")" = "343030 323030 " ]; check "malformed header block -> 400, connection stays open" $?

unk="000003""7f""00""00000000""aabbcc"
out=$(send "$unk$(rawreq GET /index.html 1)")
[ "$(statuses "$out")" = "323030 " ]; check "unknown frame type skipped, request still served" $?

out=$(send "ffff0001010000000100")
[ "$(statuses "$out")" = "343030 " ]; check "oversized length -> 400 then close" $?

echo "$pass passed, $fail failed"
rm -f server.log
[ $fail -eq 0 ]
