import re
from pathlib import Path

import matplotlib.pyplot as plt


# ============================================================
# CONFIGURAÇÃO
# ============================================================

arquivo = Path("instancia.txt")
saida = "grafo_sbrp.png"

texto = arquivo.read_text(encoding="utf-8")


# ============================================================
# LEITURA DAS SEÇÕES
# ============================================================

def ler_secao(texto, nome_secao, proxima_secao=None):

    padrao_inicio = rf"(?m)^{re.escape(nome_secao)}(?:\s.*)?$"

    inicio = re.search(padrao_inicio, texto)

    if not inicio:
        return ""

    pos_inicio = inicio.end()

    if proxima_secao is None:
        return texto[pos_inicio:]

    padrao_fim = rf"(?m)^{re.escape(proxima_secao)}(?:\s.*)?$"

    fim = re.search(
        padrao_fim,
        texto[pos_inicio:]
    )

    if not fim:
        return texto[pos_inicio:]

    return texto[
        pos_inicio:
        pos_inicio + fim.start()
    ]


# ============================================================
# ESCOLA
# ============================================================

sec_escola = ler_secao(
    texto,
    "ESCOLA",
    "PARADAS"
)

escola = None

for linha in sec_escola.strip().splitlines():

    partes = linha.split()

    if len(partes) < 3:
        continue

    try:
        identificador = int(partes[0])
        x = float(partes[1])
        y = float(partes[2])

    except ValueError:
        continue

    if identificador == 0:
        escola = (x, y)
        break


if escola is None:
    raise ValueError(
        "Não foi possível encontrar a escola."
    )


# ============================================================
# PARADAS
# ============================================================

sec_paradas = ler_secao(
    texto,
    "PARADAS",
    "ALUNOS"
)

paradas = {}

for linha in sec_paradas.strip().splitlines():

    partes = linha.split()

    if len(partes) < 3:
        continue

    try:
        pid = int(partes[0])
        x = float(partes[1])
        y = float(partes[2])

    except ValueError:
        continue

    paradas[pid] = (x, y)


# Vértice 0 = escola
paradas[0] = escola


# ============================================================
# ALUNOS
# ============================================================

sec_alunos = ler_secao(
    texto,
    "ALUNOS",
    "ATRIBUICOES"
)

alunos = {}

for linha in sec_alunos.strip().splitlines():

    partes = linha.split()

    if len(partes) < 3:
        continue

    try:
        aid = int(partes[0])
        x = float(partes[1])
        y = float(partes[2])

    except ValueError:
        continue

    alunos[aid] = (x, y)


# ============================================================
# ATRIBUIÇÕES
# ============================================================

sec_atribuicoes = ler_secao(
    texto,
    "ATRIBUICOES",
    "ONIBUS"
)

atribuicoes = {}

for linha in sec_atribuicoes.strip().splitlines():

    partes = linha.split()

    if len(partes) < 3:
        continue

    try:
        aluno = int(partes[0])
        parada = int(partes[1])
        distancia = float(partes[2])

    except ValueError:
        continue

    if aluno not in atribuicoes:
        atribuicoes[aluno] = []

    atribuicoes[aluno].append(
        (parada, distancia)
    )


# Ordena pelas distâncias
for aluno in atribuicoes:

    atribuicoes[aluno].sort(
        key=lambda x: x[1]
    )


# ============================================================
# ÔNIBUS
# ============================================================

sec_onibus = ler_secao(
    texto,
    "ONIBUS",
    "ARESTAS"
)

quantidade_onibus = None
capacidade_onibus = None

for linha in sec_onibus.strip().splitlines():

    partes = linha.split()

    if len(partes) < 2:
        continue

    try:
        quantidade_onibus = int(partes[0])
        capacidade_onibus = int(partes[1])

    except ValueError:
        continue

    break


# ============================================================
# ARESTAS
# ============================================================

sec_arestas = ler_secao(
    texto,
    "ARESTAS"
)

arestas = []

for linha in sec_arestas.strip().splitlines():

    partes = linha.split()

    if len(partes) < 3:
        continue

    try:
        u = int(partes[0])
        v = int(partes[1])
        custo = float(partes[2])

    except ValueError:
        continue

    arestas.append(
        (u, v, custo)
    )


# ============================================================
# AGRUPAR ALUNOS
# ============================================================
#
# Os alunos estão ordenados no arquivo.
#
# Exemplo:
#
# 50 10.0 20.0
# 51 10.0 20.0
# 52 10.0 20.0
#
# vira:
#
# 50-52
#
# Se houver somente:
#
# 53 30.0 40.0
#
# fica:
#
# 53
#
# ============================================================

grupos_alunos = []

alunos_ordenados = list(alunos.items())

i = 0

while i < len(alunos_ordenados):

    primeiro_id, (x, y) = alunos_ordenados[i]

    ultimo_id = primeiro_id

    j = i + 1

    while j < len(alunos_ordenados):

        aid2, (x2, y2) = alunos_ordenados[j]

        # Mesma coordenada
        if x2 == x and y2 == y:

            ultimo_id = aid2
            j += 1

        else:
            break

    grupos_alunos.append({
        "primeiro": primeiro_id,
        "ultimo": ultimo_id,
        "x": x,
        "y": y
    })

    i = j


# ============================================================
# INFORMAÇÕES
# ============================================================

print()
print("=" * 60)
print("DADOS LIDOS")
print("=" * 60)

print(f"Escola:                {escola}")
print(f"Paradas:               {len(paradas) - 1}")
print(f"Alunos:                {len(alunos)}")
print(f"Grupos de alunos:      {len(grupos_alunos)}")
print(f"Arestas:               {len(arestas)}")
print(f"Alunos com atribuição: {len(atribuicoes)}")

print(
    f"Ligações aluno-parada: "
    f"{sum(len(x) for x in atribuicoes.values())}"
)

print(f"Ônibus:                {quantidade_onibus}")
print(f"Capacidade:            {capacidade_onibus}")

print()
print("GRUPOS DE ALUNOS")

for grupo in grupos_alunos:

    primeiro = grupo["primeiro"]
    ultimo = grupo["ultimo"]

    if primeiro == ultimo:
        nome = str(primeiro)
    else:
        nome = f"{primeiro}-{ultimo}"

    print(
        f"  {nome}: "
        f"({grupo['x']:.3f}, {grupo['y']:.3f})"
    )


# ============================================================
# PLOT
# ============================================================

fig, ax = plt.subplots(
    figsize=(14, 11)
)


# ============================================================
# 1. ARESTAS ENTRE PARADAS
# ============================================================

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
        alpha=0.55,
        zorder=1
    )

    # Custo da aresta
    xm = (x1 + x2) / 2
    ym = (y1 + y2) / 2

    ax.annotate(
        f"{custo:.1f}",
        (xm, ym),
        fontsize=7,
        ha="center",
        va="center",
        alpha=0.8,
        zorder=2
    )


# ============================================================
# 2. LIGAÇÕES ALUNO -> PARADA
# ============================================================
#
# Para cada grupo de alunos:
#
# 50
# 51
# 52
#
# se todos estão no mesmo ponto, não desenhamos
# 3 linhas iguais.
#
# Pegamos a união das paradas possíveis dos alunos
# pertencentes ao grupo.
#
# ============================================================

for grupo in grupos_alunos:

    primeiro = grupo["primeiro"]
    ultimo = grupo["ultimo"]

    xa = grupo["x"]
    ya = grupo["y"]

    paradas_grupo = set()

    for aid in range(primeiro, ultimo + 1):

        for parada, distancia in atribuicoes.get(aid, []):

            paradas_grupo.add(parada)


    for parada in paradas_grupo:

        if parada not in paradas:
            continue

        xp, yp = paradas[parada]

        ax.plot(
            [xa, xp],
            [ya, yp],
            linestyle="--",
            linewidth=0.8,
            alpha=0.25,
            zorder=2
        )


# ============================================================
# 3. DESENHAR PARADAS
# ============================================================
#
# IMPORTANTE:
# Antes as paradas não estavam sendo desenhadas
# explicitamente.
#
# Agora cada parada aparece como quadrado.
#
# ============================================================

paradas_sem_escola = {
    pid: ponto
    for pid, ponto in paradas.items()
    if pid != 0
}


if paradas_sem_escola:

    ax.scatter(
        [ponto[0] for ponto in paradas_sem_escola.values()],
        [ponto[1] for ponto in paradas_sem_escola.values()],
        marker="s",
        s=90,
        facecolors="white",
        edgecolors="black",
        linewidths=1.4,
        label="Parada",
        zorder=5
    )


# ============================================================
# 4. ID DAS PARADAS
# ============================================================

for pid, (x, y) in paradas_sem_escola.items():

    ax.annotate(
        str(pid),
        (x, y),
        xytext=(5, 5),
        textcoords="offset points",
        fontsize=8,
        fontweight="bold",
        zorder=7
    )


# ============================================================
# 5. DESENHAR ALUNOS
# ============================================================

if grupos_alunos:

    ax.scatter(
        [grupo["x"] for grupo in grupos_alunos],
        [grupo["y"] for grupo in grupos_alunos],
        marker="o",
        s=55,
        facecolors="white",
        edgecolors="black",
        linewidths=1.2,
        label="Aluno(s)",
        zorder=6
    )


# ============================================================
# 6. IDENTIFICADORES DOS GRUPOS
# ============================================================
#
# Aqui NÃO desenhamos os alunos individualmente.
#
# Só desenhamos:
#
# 22
# 22-24
# 38-42
# 40
#
# dependendo dos grupos reais.
#
# ============================================================

for grupo in grupos_alunos:

    primeiro = grupo["primeiro"]
    ultimo = grupo["ultimo"]

    x = grupo["x"]
    y = grupo["y"]

    if primeiro == ultimo:

        texto = str(primeiro)

    else:

        texto = f"{primeiro}-{ultimo}"

    ax.annotate(
        texto,
        (x, y),
        xytext=(7, 7),
        textcoords="offset points",
        fontsize=8,
        fontweight="bold",
        zorder=8
    )


# ============================================================
# 7. ESCOLA
# ============================================================

ax.scatter(
    [escola[0]],
    [escola[1]],
    marker="*",
    s=350,
    facecolors="white",
    edgecolors="black",
    linewidths=1.5,
    label="Escola (0)",
    zorder=9
)

ax.annotate(
    "0 - Escola",
    escola,
    xytext=(9, 9),
    textcoords="offset points",
    fontsize=11,
    fontweight="bold",
    zorder=10
)


# ============================================================
# ESTÉTICA
# ============================================================

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


# ============================================================
# SALVAR
# ============================================================

plt.savefig(
    saida,
    dpi=300,
    bbox_inches="tight"
)

plt.show()


# ============================================================
# ATRIBUIÇÕES
# ============================================================

print()
print("=" * 60)
print("SUBPARADAS POR ALUNO")
print("=" * 60)

for aluno in sorted(atribuicoes):

    subparadas = [
        parada
        for parada, distancia
        in atribuicoes[aluno]
    ]

    print(
        f"Aluno {aluno}: {subparadas}"
    )