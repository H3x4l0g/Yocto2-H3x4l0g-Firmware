"""Envoie un .syx au bootloader, un message a la fois, avec la pause qu'il exige.

    python tools/send_syx.py                       liste les sorties MIDI
    python tools/send_syx.py <fichier.syx> <port>  envoie (port : numero ou bout de nom)
    python tools/send_syx.py <fichier.syx> <port> <pause_ms>

Ecrit pour remplacer MIDI-OX, qui veut tamponner tout le fichier d'avance et n'y arrive
pas a 88 Ko. Ici chaque message part seul, on attend que le pilote l'ait fini, puis on
laisse au bootloader le temps d'effacer et d'ecrire sa page (~9 ms, interruptions
masquees) : 50 ms par defaut. Cf tools/hex2syx.py pour le protocole.

Windows seulement, par winmm via ctypes : rien a installer.
"""
import ctypes, sys, time
from ctypes import wintypes as wt

winmm = ctypes.WinDLL('winmm')
MHDR_DONE, MIDIERR_STILLPLAYING = 1, 65

class MIDIOUTCAPSW(ctypes.Structure):
    _fields_ = [('wMid', wt.WORD), ('wPid', wt.WORD), ('vDriverVersion', wt.UINT),
                ('szPname', wt.WCHAR * 32), ('wTechnology', wt.WORD), ('wVoices', wt.WORD),
                ('wNotes', wt.WORD), ('wChannelMask', wt.WORD), ('dwSupport', wt.DWORD)]

class MIDIHDR(ctypes.Structure):
    pass
MIDIHDR._fields_ = [('lpData', ctypes.c_char_p), ('dwBufferLength', wt.DWORD),
                    ('dwBytesRecorded', wt.DWORD), ('dwUser', ctypes.c_size_t),
                    ('dwFlags', wt.DWORD), ('lpNext', ctypes.POINTER(MIDIHDR)),
                    ('reserved', ctypes.c_size_t), ('dwOffset', wt.DWORD),
                    ('dwReserved', ctypes.c_size_t * 8)]

winmm.midiOutGetDevCapsW.argtypes = [ctypes.c_size_t, ctypes.POINTER(MIDIOUTCAPSW), wt.UINT]
winmm.midiOutOpen.argtypes = [ctypes.POINTER(ctypes.c_void_p), wt.UINT,
                              ctypes.c_size_t, ctypes.c_size_t, wt.DWORD]
for f in ('midiOutPrepareHeader', 'midiOutLongMsg', 'midiOutUnprepareHeader'):
    getattr(winmm, f).argtypes = [ctypes.c_void_p, ctypes.POINTER(MIDIHDR), wt.UINT]
winmm.midiOutClose.argtypes = [ctypes.c_void_p]

def ports():
    out = []
    for i in range(winmm.midiOutGetNumDevs()):
        caps = MIDIOUTCAPSW()
        winmm.midiOutGetDevCapsW(i, ctypes.byref(caps), ctypes.sizeof(caps))
        out.append(caps.szPname)
    return out

def check(err, what):
    if err: sys.exit(f'{what} : erreur winmm {err}')

if len(sys.argv) < 3:
    for i, name in enumerate(ports()): print(f'{i} : {name}')
    sys.exit(0)

path, want = sys.argv[1], sys.argv[2]
pause = int(sys.argv[3]) / 1000 if len(sys.argv) > 3 else 0.05
names = ports()
hits = [i for i, n in enumerate(names) if want == str(i) or want.lower() in n.lower()]
if len(hits) != 1: sys.exit(f'port "{want}" introuvable ou ambigu : {names}')

data = open(path, 'rb').read()
msgs, start = [], None
for i, v in enumerate(data):
    if v == 0xF0: start = i
    elif v == 0xF7 and start is not None: msgs.append(data[start:i+1]); start = None
assert msgs and b''.join(msgs) == data, f'{path} ne se decoupe pas en messages SysEx complets'

h = ctypes.c_void_p()
check(winmm.midiOutOpen(ctypes.byref(h), hits[0], 0, 0, 0), 'ouverture')
print(f'{names[hits[0]]} : {len(msgs)} messages, pause {pause * 1000:.0f} ms')
try:
    for n, m in enumerate(msgs, 1):
        buf = ctypes.create_string_buffer(m, len(m))
        hdr = MIDIHDR(lpData=ctypes.cast(buf, ctypes.c_char_p), dwBufferLength=len(m))
        check(winmm.midiOutPrepareHeader(h, ctypes.byref(hdr), ctypes.sizeof(hdr)), 'preparation')
        check(winmm.midiOutLongMsg(h, ctypes.byref(hdr), ctypes.sizeof(hdr)), 'envoi')
        while not hdr.dwFlags & MHDR_DONE: time.sleep(0.001)
        while winmm.midiOutUnprepareHeader(h, ctypes.byref(hdr), ctypes.sizeof(hdr)) == MIDIERR_STILLPLAYING:
            time.sleep(0.001)
        #Un pilote USB peut rendre MHDR_DONE des que le message est dans SA file, pas
        #sur le fil : on ajoute le temps de ligne a 31250 bauds, 10 bits par octet.
        time.sleep(len(m) * 10 / 31250 + pause)
        print(f'\r{n}/{len(msgs)}', end='', flush=True)
finally:
    winmm.midiOutClose(h)
print('\ntermine')
