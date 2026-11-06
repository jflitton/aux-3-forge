#!/usr/bin/env python3
"""Build a port into a pkg package, and optionally test it in A/UX.

  build-port.py [--test] NAME

Paths are under ports/, except tools/ and pkg/. From NAME/: source (the tarball's URL
and SHA-256 on one line), any *.patch (applied in name order with patch -p1), build.sh
and +PKG. The tarball is downloaded into dist/ once and checked against its SHA-256 every
time. A program of our own has its source in NAME/src/ instead of a source file. In
build/ports/NAME/ it's unpacked or copied (src/), built (build/, where build.sh runs) and
installed (stage/, laid out from /); build.sh gets SRC, STAGE and TESTDIR, with sdk/bin
first in PATH and CONFIG_SITE set, and puts any files its test needs in TESTDIR. Then
build/packages/ gets NAME-VERSION.pkg, from tools/pkg-build.py, and build/packages/source/
the tarball and patches it was built from (for a tarball).

--test then boots A/UX in QEMU (tools/aux-run.py) with pkg (pkg/build/aux/pkg, made if
it isn't there), the package, build/test/ptyrun (for typing at interactive programs) and
TESTDIR's files in /usr/local/bin, and runs NAME/aux-test.sh, which installs the package
with pkg. Run `make sdk` in ports/ first; its port and test-port targets do.
"""
import hashlib, os, shutil, subprocess, sys, tarfile, urllib.request

TOP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.join(TOP, 'ports')
TOOLS, PKG = os.path.join(TOP, 'tools'), os.path.join(TOP, 'pkg')


def die(msg):
    sys.exit('build-port: %s' % msg)


def run(cmd, **kw):
    if subprocess.run(cmd, **kw).returncode:
        die('failed: %s' % ' '.join(cmd))


def fetch(url, sha256):
    dist = os.path.join(ROOT, 'dist')
    path = os.path.join(dist, os.path.basename(url))
    if not os.path.exists(path):
        os.makedirs(dist, exist_ok=True)
        print('fetching %s' % url)
        with urllib.request.urlopen(url) as r, open(path + '.part', 'wb') as f:
            shutil.copyfileobj(r, f)
        os.replace(path + '.part', path)
    with open(path, 'rb') as f:
        got = hashlib.sha256(f.read()).hexdigest()
    if got != sha256:
        die('%s has SHA-256 %s, not %s' % (path, got, sha256))
    return path


def meta(path):
    keys = {}
    for line in open(path, encoding='ascii'):
        key, _, value = line.rstrip('\n').partition(' ')
        if key and not key.startswith('#'):
            keys[key] = value
    return keys


def main():
    args = sys.argv[1:]
    test = args[:1] == ['--test']
    if test:
        args = args[1:]
    if len(args) != 1:
        sys.exit(__doc__.strip().split('\n\n')[1])
    name = args[0]
    port = os.path.join(ROOT, name)
    if '/' in name or not os.path.isfile(os.path.join(port, '+PKG')):
        die('no port %s (a folder in ports/ with a +PKG)' % name)
    pkg = meta(os.path.join(port, '+PKG'))
    tarball = None
    if os.path.exists(os.path.join(port, 'source')):
        url, sha256 = open(os.path.join(port, 'source')).read().split()
        tarball = fetch(url, sha256)
    elif not os.path.isdir(os.path.join(port, 'src')):
        die('port %s has neither a source file nor a src directory' % name)
    patches = sorted(f for f in os.listdir(port) if f.endswith('.patch'))

    work = os.path.join(ROOT, 'build', 'ports', name)
    shutil.rmtree(work, ignore_errors=True)
    src, build = os.path.join(work, 'src'), os.path.join(work, 'build')
    stage, testdir = os.path.join(work, 'stage'), os.path.join(work, 'test')
    for d in (build, stage, testdir):
        os.makedirs(d)
    if tarball:
        os.makedirs(src)
        with tarfile.open(tarball) as t:
            t.extractall(src, filter='data')
        tops = os.listdir(src)
        if len(tops) == 1 and os.path.isdir(os.path.join(src, tops[0])):
            src = os.path.join(src, tops[0])
    else:
        shutil.copytree(os.path.join(port, 'src'), src)
    for p in patches:
        run(['patch', '-p1', '-s', '-i', os.path.join(port, p)], cwd=src)

    env = dict(os.environ, SRC=src, STAGE=stage, TESTDIR=testdir,
               PATH=os.path.join(ROOT, 'sdk', 'bin') + os.pathsep + os.environ['PATH'],
               CONFIG_SITE=os.path.join(ROOT, 'sdk', 'config.site'))
    run(['/bin/sh', '-e', os.path.join(port, 'build.sh')], cwd=build, env=env)

    shutil.copy(os.path.join(port, '+PKG'), os.path.join(stage, '+PKG'))
    packages = os.path.join(ROOT, 'build', 'packages')
    os.makedirs(os.path.join(packages, 'source'), exist_ok=True)
    out = subprocess.run([sys.executable, '-I', os.path.join(TOOLS, 'pkg-build.py'),
                          stage, packages], capture_output=True, text=True)
    if out.returncode:
        die('pkg-build: %s' % out.stderr.strip())
    package = out.stdout.strip()
    for f in ([tarball] if tarball else []) + [os.path.join(port, p) for p in patches]:
        shutil.copy(f, os.path.join(packages, 'source'))
    print(package)

    if test:
        aux_pkg = os.path.join(PKG, 'build', 'aux', 'pkg')
        if not os.path.exists(aux_pkg):
            run(['make', '-C', PKG, 'build/aux/pkg'])
        files = [aux_pkg, package, os.path.join(ROOT, 'build', 'test', 'ptyrun')] + \
            [os.path.join(testdir, f) for f in sorted(os.listdir(testdir))]
        os.chdir(ROOT)
        sys.exit(subprocess.run([sys.executable, '-I', os.path.join(TOOLS, 'aux-run.py'),
                                 os.path.join(port, 'aux-test.sh')] + files).returncode)


if __name__ == '__main__':
    main()
