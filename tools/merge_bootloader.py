"""Fusionne l'application compilee avec la section boot relevee au backup.

Le bootloader SysEx d'e-licktronic n'existe nulle part ailleurs : ni dans MightyCore,
ni dans le depot amont. Le projet etant abandonne, personne ne peut le refournir. Or un
televersement ISP fait un chip erase. On flashe donc un fichier unique qui porte les
deux, et le bootloader survit.

    python tools/merge_bootloader.py <app.hex> <yocto-backup.hex> <sortie.hex>

Le script refuse de tourner si l'application deborde sur la section boot, et relit sa
propre sortie pour verifier qu'elle est identique octet pour octet avant de rendre la
main.
"""
import sys
BOOT = 0x1F800   # debut de la section boot preservee, releve au backup

def read_hex(path):
    mem = {}; base = 0
    for line in open(path):
        line = line.strip()
        if not line.startswith(':'): continue
        b = bytes.fromhex(line[1:])
        assert (sum(b) & 0xFF) == 0, f'checksum invalide dans {path} : {line}'
        n, addr, typ, data = b[0], (b[1] << 8) | b[2], b[3], b[4:4+b[0]]
        if typ == 0x00:
            for i, v in enumerate(data): mem[base + addr + i] = v
        elif typ == 0x04: base = ((data[0] << 8) | data[1]) << 16
        elif typ == 0x02: base = ((data[0] << 8) | data[1]) << 4
        elif typ == 0x01: break
    return mem

def write_hex(mem, path):
    out = []; base = None
    def rec(b): return ':' + (b + bytes([(-sum(b)) & 0xFF])).hex().upper()
    addrs = sorted(mem)
    runs = []                                   # suites d'adresses contigues
    for a in addrs:
        if runs and a == runs[-1][-1] + 1: runs[-1].append(a)
        else: runs.append([a])
    for run in runs:
        i = 0
        while i < len(run):
            start = run[i]
            n = min(16, len(run) - i, 16 - (start & 0xF))   # ne franchit pas la frontiere de 16
            hi = start >> 16
            if hi != base:
                base = hi
                out.append(rec(bytes([2, 0, 0, 4, (hi >> 8) & 0xFF, hi & 0xFF])))
            data = bytes(mem[start + k] for k in range(n))
            out.append(rec(bytes([n, (start >> 8) & 0xFF, start & 0xFF, 0]) + data))
            i += n
    out.append(':00000001FF')
    open(path, 'w', newline='\n').write('\n'.join(out) + '\n')

app, backup, dest = sys.argv[1], sys.argv[2], sys.argv[3]
a, b = read_hex(app), read_hex(backup)
assert not [x for x in a if x >= BOOT], "l'application deborde sur la section boot"
# section boot conservee telle quelle, seule la queue effacee est retiree
hi = [k for k in b if k >= BOOT]
last = max(k for k in hi if b[k] != 0xFF)
boot = {k: b[k] for k in hi if k <= last}
merged = dict(a); merged.update(boot)
# La version affichee au boot est la seule chose qui dise, plus tard, ce qu'il y a
# vraiment dans la puce. Le 12/09/2026 un bump de FW_VERSION a ete perdu et la machine a
# tourne plusieurs jours en annoncant une version de retard : on l'imprime ici, pour que
# le chiffre passe sous les yeux juste avant l'ecriture.
import re as _re
_blob = bytes(a[k] for k in sorted(a))
_vers = sorted(set(_re.findall(rb"Yocto v[0-9.]+", _blob)))
print(f"  version     : {(b' / '.join(_vers) or b'AUCUNE TROUVEE').decode():>6}")
print(f"  application : {len(a):6} o   0x{min(a):04X}-0x{max(a):04X}")
print(f"  bootloader  : {len(boot):6} o   0x{min(boot):05X}-0x{max(boot):05X}  (conserve tel quel)")
print(f"  fusionne    : {len(merged):6} o")
write_hex(merged, dest)
c = read_hex(dest)
assert c == merged, 'le fichier relu ne correspond pas'
assert all(c[k] == v for k, v in a.items()), 'application alteree'
assert all(c[k] == b[k] for k in boot), 'bootloader altere'
print(f"  relu et verifie octet par octet : identique")
