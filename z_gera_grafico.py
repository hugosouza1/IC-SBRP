import re
import math
from pathlib import Path

import matplotlib.pyplot as plt


arquivo = Path("instancia.txt")
texto = arquivo.read_text(encoding="utf-8")


# ------------------------------------------------------------
# Leitura das seções
# ------------------------------------------------------------

def ler_secao(texto, inicio, fim=None):
    if fim is not None:
        padrao = (
            re.escape(inicio)
            + r"\n(.*?)(?=\n"
            + re.escape(fim)
            + r"|\Z)"
        )
    else:
        padrao = re.escape(inicio) + r"\n(.*?)(?:\Z)"

    m = re.search(padrao, texto, flags=re.S)

    return m.group(1) if m else ""


# ------------------------------------------------------------
# Escola
# ------------------------------------------------------------

m = re.search(
    r"^0\s+([-\d.]+)\s+([-\d.]+)\s*$",
    ler_secao(texto, "ESCOLA", "PARADAS id x y"),
    re.M
)

if not m:
    raise ValueError("Não foi possível encontrar a escola.")

escola = (
    float(m.group(1)),
    float(m.group(2))
)


# ------------------------------------------------------------
# Paradas: id x y
# ------------------------------------------------------------

sec_paradas = ler_secao(
    texto,
    "PARADAS id x y",
    "ALUNOS id x y"
)

paradas = {}

for linha in sec_paradas.strip().splitlines():

    partes = linha.split()

    if len(partes) == 3:

        pid = int(partes[0])

        x = float(partes[1])
        y = float(partes[2])

        paradas[pid] = (x, y)


# O vértice 0 representa a escola
paradas[0] = escola


# ------------------------------------------------------------
# Alunos: id x y
# ------------------------------------------------------------

sec_alunos = ler_secao(
    texto,
    "ALUNOS id x y",
    "ATRIBUICOES aluno_id parada_id distancia dentro_raio"
)

alunos = {}

for linha in sec_alunos.strip().splitlines():

    partes = linha.split()

    if len(partes) == 3:

        aid = int(partes[0])

        x = float(partes[1])
        y = float(partes[2])

        alunos[aid] = (x, y)


# ------------------------------------------------------------
# Arestas: origem destino custo
# ------------------------------------------------------------

m = re.search(
    r"ARESTAS\s+(\d+)\s*\n(.*?)(?:\Z)",
    texto,
    flags=re.S
)

arestas = []

if m:

    for linha in m.group(2).strip().splitlines():

        partes = linha.split()

        if len(partes) >= 3:

            u = int(partes[0])
            v = int(partes[1])
            custo = float(partes[2])

            arestas.append((u, v, custo))



# ------------------------------------------------------------

sec_atr = ler_secao(
    texto,
    "ATRIBUICOES aluno_id parada_id distancia dentro_raio",
    "ALUNOS_ATRIBUICAO_FORCADA"
)


# aluno -> lista de (parada, distancia)
atribuicoes = {}

for linha in sec_atr.strip().splitlines():

    partes = linha.split()

    if len(partes) != 4:
        continue

    aluno = int(partes[0])
    parada = int(partes[1])
    distancia = float(partes[2])
    dentro = int(partes[3])

    # Só considera subparadas dentro do raio
    if dentro != 1:
        continue

    if aluno not in atribuicoes:
        atribuicoes[aluno] = []

    atribuicoes[aluno].append(
        (parada, distancia)
    )


# ------------------------------------------------------------
# Ordena as subparadas de cada aluno pela distância
# ------------------------------------------------------------

for aluno in atribuicoes:

    atribuicoes[aluno].sort(
        key=lambda x: x[1]
    )


# ------------------------------------------------------------
# Plot
# ------------------------------------------------------------

fig, ax = plt.subplots(figsize=(12, 10))


# ------------------------------------------------------------
# 1. Arestas entre paradas
# ------------------------------------------------------------

for u, v, custo in arestas:

    if u not in paradas or v not in paradas:
        continue

    x1, y1 = paradas[u]
    x2, y2 = paradas[v]

    ax.plot(
        [x1, x2],
        [y1, y2],
        linestyle="-",
        linewidth=1.0,
        alpha=0.45,
        zorder=1
    )


# ------------------------------------------------------------
# 2. Ligações aluno -> TODAS as subparadas
# ------------------------------------------------------------

for aluno, lista_paradas in atribuicoes.items():

    if aluno not in alunos:
        continue

    xa, ya = alunos[aluno]

    for parada, distancia in lista_paradas:

        if parada not in paradas:
            continue

        xp, yp = paradas[parada]

        ax.plot(
            [xa, xp],
            [ya, yp],
            linestyle="--",
            linewidth=0.7,
            alpha=0.18,
            zorder=2
        )


# ------------------------------------------------------------
# 3. Alunos
# ------------------------------------------------------------

if alunos:

    ax.scatter(
        [p[0] for p in alunos.values()],
        [p[1] for p in alunos.values()],
        marker="o",
        s=24,
        alpha=0.65,
        label="Aluno",
        zorder=4
    )


# ------------------------------------------------------------
# 4. Paradas
#
# O vértice 0 não é desenhado como quadrado,
# pois representa a escola.
# ------------------------------------------------------------

paradas_normais = {
    pid: ponto
    for pid, ponto in paradas.items()
    if pid != 0
}


if paradas_normais:

    ax.scatter(
        [p[0] for p in paradas_normais.values()],
        [p[1] for p in paradas_normais.values()],
        marker="s",
        s=85,
        facecolors="white",
        edgecolors="black",
        linewidths=1.5,
        label="Parada",
        zorder=5
    )


# ------------------------------------------------------------
# 5. Números das paradas
# ------------------------------------------------------------

for pid, (x, y) in paradas_normais.items():

    ax.annotate(
        str(pid),
        (x, y),
        xytext=(5, 5),
        textcoords="offset points",
        fontsize=9,
        fontweight="bold",
        zorder=6
    )


# ------------------------------------------------------------
# 6. Identificadores dos alunos
# ------------------------------------------------------------

for aid, (x, y) in alunos.items():

    ax.annotate(
        str(aid),
        (x, y),
        xytext=(4, 4),
        textcoords="offset points",
        fontsize=8,
        alpha=0.8,
        zorder=6
    )


# ------------------------------------------------------------
# 7. Escola / vértice 0
# ------------------------------------------------------------

ax.scatter(
    [escola[0]],
    [escola[1]],
    marker="*",
    s=280,
    label="Escola (0)",
    zorder=7
)


ax.annotate(
    "0 - Escola",
    escola,
    xytext=(8, 8),
    textcoords="offset points",
    fontsize=11,
    fontweight="bold",
    zorder=8
)


# ------------------------------------------------------------
# Estética
# ------------------------------------------------------------

ax.set_title(
    "Representação espacial do grafo de transporte escolar",
    fontsize=15
)

ax.set_xlabel("X")
ax.set_ylabel("Y")

ax.grid(
    True,
    alpha=0.25
)

ax.set_aspect(
    "equal",
    adjustable="box"
)

ax.legend()

plt.tight_layout()


# ------------------------------------------------------------
# Salvar
# ------------------------------------------------------------

saida = "grafo_sbrp.png"

plt.savefig(
    saida,
    dpi=300,
    bbox_inches="tight"
)

plt.show()


# ------------------------------------------------------------
# Informações
# ------------------------------------------------------------

total_atribuicoes = sum(
    len(lista)
    for lista in atribuicoes.values()
)

print(f"Imagem salva em: {saida}")
print(f"Paradas: {len(paradas) - 1}")
print(f"Alunos: {len(alunos)}")
print(f"Arestas: {len(arestas)}")
print(f"Alunos com atribuições: {len(atribuicoes)}")
print(f"Ligações aluno -> subparada: {total_atribuicoes}")


# ------------------------------------------------------------
# Mostra as subparadas de cada aluno
# ------------------------------------------------------------

print("\nSubparadas por aluno:")

for aluno in sorted(atribuicoes):

    subparadas = [
        parada
        for parada, distancia in atribuicoes[aluno]
    ]

    print(
        f"Aluno {aluno}: {subparadas}"
    )