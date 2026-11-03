"""Convertit l'application compilee en fichier SysEx pour le bootloader d'e-licktronic.

    python tools/hex2syx.py <app.hex> <sortie.syx>

C'est la voie de mise a jour sans programmateur : allumer en tenant les pas 1, 3 et 5,
les LED clignotent deux fois, envoyer le .syx. Pas d'ISP, donc pas de chip erase, et le
bootloader ne touche jamais a sa propre section : un transfert rate se refait.

LE PROTOCOLE, RELU DANS LE BOOTLOADER LUI-MEME (section boot 0x1F800, aucune doc ne
l'a jamais decrit). Un message par page de 256 octets :

    F0 7D 08 08 02   en-tete, compare octet par octet (7D = ID non commercial)
    7E 00            commande "ecrire la page suivante"
    512 octets       les 256 octets de la page, en quartets : poids fort puis faible
    2 octets         somme des 256 octets modulo 256, en quartets elle aussi
    F7

et pour finir F0 7D 08 08 02 7F 00 F7, qui rend la main a l'application.

LES PAGES NE PORTENT PAS D'ADRESSE. Le bootloader ecrit a partir de zero et n'avance
son pointeur que sur une page acceptee ; une page refusee (somme fausse, octet perdu)
est ignoree EN SILENCE, et toutes les suivantes tombent une page trop bas. D'ou deux
regles cote emetteur :
  - une PAUSE APRES CHAQUE F7 : l'effacement et l'ecriture de la page masquent les
    interruptions ~9 ms, et l'USART ne tamponne que trois octets. 50 ms suffisent
    largement ; au-dela de ~8 s sans octet, le bootloader abandonne et lance l'appli ;
  - si le compteur des LED de pas ne finit pas sur le nombre de pages annonce par ce
    script, le transfert est faux : recommencer, le bootloader est intact.

Le script refuse une application qui deborderait sur la section boot, puis rejoue sa
propre sortie dans une transcription de l'automate du bootloader et verifie que la
flash reconstituee est identique, octet pour octet, a l'application.
"""
import sys
BOOT = 0x1F800                      # debut de la section boot, cf merge_bootloader.py
PAGE = 256                          # page SPM de l'ATmega1284P
HEADER = bytes([0xF0, 0x7D, 0x08, 0x08, 0x02])
WRITE, LEAVE = bytes([0x7E, 0x00]), bytes([0x7F, 0x00])

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

def nibbles(data):
    return bytes(x for v in data for x in (v >> 4, v & 0x0F))

def encode(image):
    out = bytearray()
    for p in range(0, len(image), PAGE):
        page = image[p:p+PAGE]
        out += HEADER + WRITE + nibbles(page) + nibbles([sum(page) & 0xFF]) + b'\xF7'
    return bytes(out + HEADER + LEAVE + b'\xF7')

def replay(stream):
    """L'automate du bootloader (0x1FB2C), transcrit etat pour etat."""
    flash = bytearray(); state = n = 0; cmd = []; buf = bytearray(); total = 0
    for v in stream:
        if v >= 0xF1 and v != 0xF7: continue                  # temps reel : ignore
        if state == 0:                                        # en-tete
            if v == HEADER[n]:
                n += 1
                if n == len(HEADER): state, n, cmd = 2, 0, []
            else: n = 0
        elif state == 2:                                      # deux octets de commande
            if v & 0x80: state, n = 0, 0; continue
            cmd.append(v)
            if len(cmd) == 2: state, n, buf, total = 3, 0, bytearray(), 0
        elif v & 0x80:                                        # state 3 : fin de message
            if v != 0xF7: continue
            if bytes(cmd) == LEAVE and n == 0: return flash
            if bytes(cmd) == WRITE and len(buf) == PAGE + 1 and buf[PAGE] == total & 0xFF:
                flash += buf[:PAGE]
            state, n = 0, 0
        else:                                                 # quartets de donnees
            if n & 1:
                buf[-1] |= v & 0x0F
                if len(buf) <= PAGE: total += buf[-1]
            else: buf.append((v << 4) & 0xF0)
            n += 1
    raise AssertionError('pas de message de sortie : le bootloader attendrait encore')

app, dest = sys.argv[1], sys.argv[2]
mem = read_hex(app)
assert mem, f'{app} est vide'
assert max(mem) < BOOT, "l'application deborde sur la section boot"
size = (max(mem) // PAGE + 1) * PAGE
image = bytes(mem.get(a, 0xFF) for a in range(size))
syx = encode(image)
assert replay(syx) == image, 'la relecture ne redonne pas l application'
open(dest, 'wb').write(syx)
print(f'{dest} : {size // PAGE} pages, {len(syx)} octets, relecture identique')
