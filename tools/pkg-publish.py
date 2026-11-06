#!/usr/bin/env python3
"""Add packages to a pkg server whose files are in S3: fetch what it has, add these, sign
the new list and upload it.

  pkg-publish.py [-n] KEYFILE s3://BUCKET[/DIR] http://SERVER[/DIR]/ PACKAGE...

Reads the server's index and index.sig and checks the list is signed by KEYFILE's key
(a key file, or a .env with PKG_KEY, as pkg-repo.py takes). Downloads every package the
list names into build/site/ (under the current directory), checking each against the
list, adds PACKAGE... under packages/, lists and signs them all with pkg-repo.py, and
uploads with publish.sh, which puts up only what's new and reads it back. So the new list
names everything the old one did, and nothing is taken off the server.

A package already on the server is fine if it's the same file, and refused if it isn't: a
new build needs a new version. If every PACKAGE is up already, nothing is uploaded. With
-n, it stops before uploading and leaves build/site/ to look at.
"""
import hashlib, importlib.util, os, shutil, subprocess, sys, tarfile, urllib.error, urllib.request

TOOLS = os.path.dirname(os.path.abspath(__file__))
SITE = os.path.join('build', 'site')

sys.path.insert(0, TOOLS)
import ed25519  # noqa: E402

spec = importlib.util.spec_from_file_location('pkg_repo', os.path.join(TOOLS, 'pkg-repo.py'))
pkg_repo = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pkg_repo)


def die(msg):
    sys.exit('pkg-publish: %s' % msg)


def fetch(url):
    try:
        with urllib.request.urlopen(url, timeout=60) as r:
            return r.read()
    except urllib.error.HTTPError as e:
        die('%s: %s %s' % (url, e.code, e.reason))
    except urllib.error.URLError as e:
        die('%s: %s' % (url, e.reason))


def signed_by(index, sig, pub):
    for line in sig.decode('ascii', 'replace').splitlines():
        f = line.split('\t')
        if len(f) == 3 and f[0] == 'ed25519' and f[1] == pub.hex() and len(f[2]) == 128:
            try:
                return ed25519.verify(pub, index, bytes.fromhex(f[2]))
            except ValueError:
                return False
    return False


def entries(index):
    lines = index.decode('ascii').splitlines()
    if not lines or lines[0] != 'aux-pkg-index 1':
        die('the server\'s index isn\'t a package list (aux-pkg-index 1)')
    out = []
    for line in lines[1:]:
        if not line or line.startswith('#'):
            continue
        f = line.split('\t')
        if len(f) != 6:
            die('the server\'s index has a bad line: %s' % line)
        name, version, size, sha, path = f[:5]
        parts = path.split('/')
        if path.startswith('/') or '..' in parts or '' in parts or '.' in parts:
            die('the server\'s index has a bad path: %s' % path)
        out.append((name, version, int(size), sha, path))
    return out


def main():
    args = sys.argv[1:]
    dry = args[:1] == ['-n']
    if dry:
        args = args[1:]
    if len(args) < 4:
        sys.exit(__doc__.strip().split('\n\n')[1])
    keyfile, dest, url, packages = args[0], args[1], args[2], args[3:]
    if not dest.startswith('s3://'):
        die('%s: not s3://BUCKET[/DIR]' % dest)
    if not url.startswith('http://'):
        die('%s: not an http:// URL (pkg can\'t use HTTPS)' % url)
    url = url.rstrip('/') + '/'
    pub = ed25519.public_key(pkg_repo.read_key(keyfile))

    index, sig = fetch(url + 'index'), fetch(url + 'index.sig')
    if not signed_by(index, sig, pub):
        die('%sindex isn\'t signed by %s\'s key' % (url, keyfile))
    have = entries(index)

    shutil.rmtree(SITE, ignore_errors=True)
    os.makedirs(os.path.join(SITE, 'packages'))
    for name, version, size, sha, path in have:
        data = fetch(url + path)
        if len(data) != size or hashlib.sha256(data).hexdigest() != sha:
            die('%s%s isn\'t the file the list names' % (url, path))
        os.makedirs(os.path.dirname(os.path.join(SITE, path)), exist_ok=True)
        with open(os.path.join(SITE, path), 'wb') as f:
            f.write(data)
    print('%s: %d package(s), all as listed' % (url, len(have)))

    new = []
    for p in packages:
        try:
            data = open(p, 'rb').read()
            meta = pkg_repo.package_meta(p)
        except OSError as e:   # (gzip's errors are OSErrors too)
            die('%s: not a package (%s)' % (p, e.strerror or e))
        except (tarfile.ReadError, EOFError) as e:
            die('%s: not a package (%s)' % (p, e or 'cut short'))
        path = 'packages/' + os.path.basename(p)
        same = [e for e in have if (e[0], e[1]) == (meta['name'], meta['version'])
                or e[4] == path]
        if same:
            if any(e[3] != hashlib.sha256(data).hexdigest() or e[4] != path for e in same):
                die('%s %s is on the server already as a different file (%s): a new build '
                    'needs a new version' % (meta['name'], meta['version'], same[0][4]))
            print('%s %s is on the server already' % (meta['name'], meta['version']))
            continue
        with open(os.path.join(SITE, path), 'wb') as f:
            f.write(data)
        new.append('%s %s' % (meta['name'], meta['version']))
    if not new:
        print('pkg-publish: nothing new')
        return

    subprocess.run([sys.executable, '-I', os.path.join(TOOLS, 'pkg-repo.py'), 'index', SITE,
                    keyfile], check=True)
    listed = {(e[0], e[1]) for e in entries(open(os.path.join(SITE, 'index'), 'rb').read())}
    missing = {(e[0], e[1]) for e in have} - listed
    if missing:
        die('the new list leaves out %s' % ', '.join(' '.join(m) for m in sorted(missing)))
    if dry:
        print('pkg-publish: would add %s; %s is ready, not uploaded (-n)' %
              (', '.join(new), SITE))
        return
    r = subprocess.run(['sh', os.path.join(TOOLS, 'publish.sh'), SITE, dest, url])
    if r.returncode:
        sys.exit(r.returncode)
    print('pkg-publish: added %s' % ', '.join(new))


if __name__ == '__main__':
    main()
