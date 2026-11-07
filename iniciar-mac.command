#!/bin/zsh
set -e
cd "$(dirname "$0")/web"
python3 -m http.server --bind 127.0.0.1 8765 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' INT TERM EXIT
sleep 1
open "http://127.0.0.1:8765/"
echo "Demo abierta. Deja esta ventana abierta mientras la uses."
wait "$server_pid"
