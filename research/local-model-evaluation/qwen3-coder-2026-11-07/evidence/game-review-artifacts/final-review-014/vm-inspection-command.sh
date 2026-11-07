set -eu
uname -n
for path in /packages/luanti-5.17.0-1/.self/apps/Luanti /packages/luanti-5.17.0-1/.self/apps/Luanti/Luanti /packages/luanti-5.17.0-1/.self/apps/Luanti/launch.sh /home/config/luanti/games/voxe-libre/game.conf; do
 if test -e "$path"; then ls -ld "$path"; else printf 'MISSING %s
' "$path"; fi
done
if test -f /packages/luanti-5.17.0-1/.self/apps/Luanti/Luanti; then
 sha256sum /packages/luanti-5.17.0-1/.self/apps/Luanti/Luanti
 readelf -h /packages/luanti-5.17.0-1/.self/apps/Luanti/Luanti
fi
