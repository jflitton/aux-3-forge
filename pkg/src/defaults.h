/*
 * The server pkg uses unless /etc/pkg.conf or -s names another, and the public keys it
 * trusts to sign package lists (64 hex digits each): see docs/publishing.md. The build
 * for this machine sets DEFAULT_SERVER to "", so tests never reach the real one.
 */
#ifndef DEFAULT_SERVER
#define DEFAULT_SERVER	"http://auxpkg.zerobytesfree.org/"
#endif

static const char *const default_keys[] = {
	"17ed83cf368d5b5fa6dc6784fe9ed36a43602aa7d38cbfa809224c4ef4ac5943",
	NULL
};
