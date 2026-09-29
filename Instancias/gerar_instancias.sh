# Uso: ./rodar_instancias.sh [arquivo_de_parametros]

g++ y_gerador_entrada.cpp -o geradorEntrada.exe
# ./geradorEntrada.exe $( < parametros.txt)

mkdir -p Instancias_120

ARQ="${1:-parametros_120_instancias.txt}"
BIN="./geradorEntrada.exe"

if [ ! -f "$ARQ" ]; then
    echo "arquivo nao encontrado: $ARQ"
    exit 1
fi

if [ ! -x "$BIN" ]; then
    echo "executavel nao encontrado: $BIN"
    exit 1
fi

n=0
while IFS= read -r linha || [ -n "$linha" ]; do
    linha="${linha%$'\r'}"
    [ -z "$linha" ] && continue
    n=$((n + 1))
    echo "[$n] $BIN $linha"
    $BIN $linha
done < "$ARQ"

echo "Total de execucoes: $n"