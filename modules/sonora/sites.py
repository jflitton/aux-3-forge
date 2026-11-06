"""The Sonora module's patch sites (modules/sonora/sonora.S), for a group's kernel-patch.py.

The module is linked over orwellsize and memcdjsize in pstart, which is identity-mapped, so
every hook is entered at its link address, MMU on or off. The call sites:

  _start: jsr boardinit               jsr sonora_board
  boardinit: move.w machineID,d0;     jmp sonora_switch; 3 nops
    ext.l d0; bra.w <switch>
  memsize: tst.l rbv_exists           jsr sonora_rbv (the bne.w after it stays)
  memsize (one bank):                 clr.l -(sp); jsr sonora_banksize
    moveq #0,d0; move.l d0,-(sp);
    jsr banksize
  vadrspace: jsr setup_autovectors    jsr sonora_vectors
  aEgretDispatch: jsr cEgretDispatch  jsr sonora_egret
  cEgretDispatch: link.l a6,#-12      jmp sonora_cegret (the DFAC filter, which goes on to
                                      the Egret code, or to the Cuda transport linked
                                      after this jmp)

A checkpoint build also redirects, in /unix only (autoconfig writes these tables),
init_first's egret_init, video_init and fdb_init entries, main's jsr startup, and
initfunc's iinit and nvram_init entries to the ck_* trampolines.
"""
import struct

from kpatch import CLR_SP, JMP, JSR, NOP

LINK_A6_12 = b'\x48\x0e\xff\xff\xff\xf4'


def sites(k, esym, blob, what):
    """The linked code (blob, linked at orwellsize) and the redirected call sites."""
    if esym['ourmac'] != k.sym['orwellsize']:
        raise SystemExit('the code is not linked at orwellsize')
    if k.section(k.sym['orwellsize'])['name'] != 'pstart':
        raise SystemExit('orwellsize is not in pstart')
    ptr = lambda name: struct.pack('>I', esym[name])
    switch_site, _ = k.boardinit_switch()
    rbv_site, banksize_site = k.memsize_rbv()
    egret = k.sym['aEgretDispatch'] + 2
    if k.read(egret, 6) != JSR + struct.pack('>I', k.sym['cEgretDispatch']):
        raise SystemExit('aEgretDispatch+2 is not jsr cEgretDispatch')
    if k.read(k.sym['cEgretDispatch'], 6) != LINK_A6_12:
        raise SystemExit('cEgretDispatch does not start with link.l a6,#-12')
    return [
        (k.sym['orwellsize'], blob, '%s (%d bytes)' % (what, len(blob))),
        (k.find_call('pstart', 'boardinit'), JSR + ptr('sonora_board'),
         'jsr sonora_board, was boardinit'),
        (switch_site, JMP + ptr('sonora_switch') + NOP * 3, 'jmp sonora_switch'),
        (rbv_site, JSR + ptr('sonora_rbv'), 'jsr sonora_rbv, was tst.l rbv_exists'),
        (banksize_site, CLR_SP + JSR + ptr('sonora_banksize'),
         'clr.l -(sp); jsr sonora_banksize, was banksize'),
        (k.find_call('pstart', 'setup_autovectors'), JSR + ptr('sonora_vectors'),
         'jsr sonora_vectors, was setup_autovectors'),
        (egret, JSR + ptr('sonora_egret'), 'jsr sonora_egret, was cEgretDispatch'),
        (k.sym['cEgretDispatch'], JMP + ptr('sonora_cegret'),
         'jmp sonora_cegret, was link.l a6,#-12'),
    ]


def ckpt_sites(k, esym):
    """A checkpoint build's trampolines, for /unix only (empty for a normal build)."""
    if 'ck_startup' not in esym:
        return []
    ptr = lambda name: struct.pack('>I', esym[name])
    return [(k.table_entry('init_first', f), ptr('ck_' + f), 'init_first: ck_' + f)
            for f in ('egret_init', 'video_init', 'fdb_init')] + [
        (k.find_call('.text', 'startup'), JSR + ptr('ck_startup'),
         'main: jsr ck_startup, was startup')] + [
        (k.table_entry('initfunc', f), ptr('ck_' + f), 'initfunc: ck_' + f)
        for f in ('iinit', 'nvram_init')]
