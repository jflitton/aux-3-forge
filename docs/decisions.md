# Decisions

What we've settled about the repository as a whole, and why. Newest last. When a decision
changes, edit it here and say what replaced it, rather than leaving two answers. pkg's own
decisions are in [pkg/docs/decisions.md](../pkg/docs/decisions.md), the SDK's and the
ports' in [ports/docs/decisions.md](../ports/docs/decisions.md); what we've found out about
A/UX and the Macs is in [findings.md](findings.md).

## 2026-11-06: one repository

### aux-3-patches, aux-3-pkg and aux-3-ports became aux-3-forge

From 2026-10-28 to 2026-11-06 these were three repositories: the patches (aux-3-patches),
pkg (aux-3-pkg), and the SDK and ports (aux-3-ports), split the way Homebrew keeps its
formulae out of `brew` and FreeBSD its ports tree out of `pkg`. That suits big projects
with many contributors and their own release teams. It didn't suit this one: nearly every
change crossed them (netsetup's boot line was a pkg release, a port and a patch, in three
repositories), and what held them together was work of its own: checkouts side by side
(`AUX_PATCHES`, `AUX_PKG`), scripts that built in another repository and recorded its
commit, two copies of `local.mk` and of the signing key, and a map of which repository
held what.

So they're one, each part in a folder of its own:

- The patches stay at the top, so `make-disk.sh`, the README its users read and the
  "Download ZIP" they take are where they were; aux-3-patches was renamed aux-3-forge
  (below), and GitHub sends its old address here.
- pkg is in `pkg/`, the SDK and ports in `ports/` (each port in `ports/NAME/`), each with
  its own Makefile, guide and decisions.
- Every tool is in `tools/`, whichever part uses it; only the SDK keeps its own
  (`ports/sdk/bin`, `ports/sdk/tools`), as programs' builds use them as a whole.
- One `local.mk` and one `.env`, at the top.

Their histories came with them, under `pkg/` and `ports/` (git filter-repo), and pkg's
tags `vVERSION` became `pkg-vVERSION`. aux-3-pkg and aux-3-ports stay on GitHub, archived,
with a pointer here: pkgs already installed, and packages already published, link to them.

What separate repositories gave, one keeps:

- **pkg stands alone.** It still uses nothing from the patches or the ports: `pkg/` builds
  from its own source and `tools/`, with no Apple code.
- **Licenses.** Everything is MIT, as each repository was; a port's patches to a GPL
  program are under that program's license, as before.
- **Releases.** pkg's are tags (`pkg-vVERSION`), a port's are the version in its `+PKG`;
  neither needed a repository to itself.

### Its name: aux-3-forge

For a few hours it was aux-3, which reads as A/UX 3 itself and is hard to search for.
aux-3-forge keeps "aux-3", so people looking for A/UX 3 still find it, and says it's where
things for A/UX 3 are made: patched disks, pkg and its packages, ports. GitHub sends aux-3
and aux-3-patches here, and pkg 0.6's record (`url`) still names aux-3, so no repository
may take those names again.
