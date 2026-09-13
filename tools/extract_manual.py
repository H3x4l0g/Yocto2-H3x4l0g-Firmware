import io, re, sys
from pypdf import PdfReader

base = ("C:/Users/fauvert/AppData/Local/Temp/claude/"
        "C--Users-fauvert-Documents-Arduino-yocto2/"
        "1f5e64bb-ed3b-49b3-a8fa-6e72500cdb2d/scratchpad/")

r = PdfReader(base + "upstream/Yocto v2.002 user manual.pdf")
print('pages :', len(r.pages), file=sys.stderr)

out = []
for i, p in enumerate(r.pages, 1):
    t = p.extract_text() or ''
    t = re.sub(r'[ \t]+', ' ', t)
    t = re.sub(r'\n{3,}', '\n\n', t).strip()
    out.append(f'\n======== PAGE {i} ========\n{t}')

txt = '\n'.join(out)
io.open(base + 'manual.txt', 'w', encoding='utf-8').write(txt)
print('caracteres :', len(txt), file=sys.stderr)
