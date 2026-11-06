"""The Cuda transport's patch sites (modules/cuda/cuda.c), for a group's kernel-patch.py.

The transport is linked over cEgretDispatch and the Egret byte-level helpers after it
(`send_packet` ... `get_urgent_response`), which nothing reaches once cEgretDispatch and
the three entry points below are redirected: shift_reg_intr (the VIA1 shift register
interrupt), egret_reset and abort_pending_transactions become jmps to cuda_intr, cuda_reset
and cuda_abort. Callers of cEgretDispatch then reach cuda_dispatch, directly or through a
filter the group puts at cEgretDispatch (the Sonora module's DFAC setting).
"""
import struct

from kpatch import JMP

STUBS = [('shift_reg_intr', 'cuda_intr'), ('egret_reset', 'cuda_reset'),
         ('abort_pending_transactions', 'cuda_abort')]


def code(k, esym, blob, at, what):
    """The linked code, which must start with cuda_dispatch, linked at at (cEgretDispatch,
    or after the group's filter there)."""
    if esym['cuda_dispatch'] != at:
        raise SystemExit('cuda_dispatch is not at %#x' % at)
    return (at, blob, '%s (%d bytes)' % (what, len(blob)))


def stubs(k, esym):
    """jmps from the Egret driver's entry points to the transport's."""
    return [(k.sym[at], JMP + struct.pack('>I', esym[to]), 'jmp %s' % to) for at, to in STUBS]
