# -*- coding: utf-8 -*-
# Verification exhaustive de l'identite qui remplace la division 16 bits :
#   ancien : (p + swing) % scale == 0   avec swing = -retard
#   nouveau : (p % scale) == retard
# Le C tronque vers zero sur les negatifs, d'ou la simulation explicite.

def c_mod(a, b):
    q = int(a / b)          # troncature vers zero, comme le C
    return a - q * b

table = {
    12: [0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5],
    16: [0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 7],
    24: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    32: [0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13],
}

PPQN = 96
ecarts = 0
cas = 0
for scale, retards in table.items():
    for cran, d in enumerate(retards, 1):
        assert d < scale, (scale, cran, d)
        for p in range(0, PPQN + 1):          # p va de 0 a 96
            ancien = c_mod(p - d, scale) == 0
            nouveau = c_mod(p, scale) == d
            cas += 1
            if ancien != nouveau:
                ecarts += 1
                if ecarts <= 8:
                    print('  ECART scale=%d cran=%d retard=%d p=%d : ancien=%s nouveau=%s'
                          % (scale, cran, d, p, ancien, nouveau))

print('  cas testes :', cas)
print('  ecarts     :', ecarts)
print('  ->', 'IDENTIQUE' if ecarts == 0 else 'DIVERGENCE')

# et la borne qui rend l'identite vraie : aucun p - retard ne tombe sur un multiple negatif
pire = min(0 - max(r) for r in table.values())
print('  p - retard le plus negatif possible :', pire)
for scale in table:
    print('    scale %2d : premier multiple negatif %4d, retard max %2d -> p-retard >= %d'
          % (scale, -scale, max(table[scale]), 0 - max(table[scale])))
