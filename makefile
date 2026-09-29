# =======================================
# COMPILADOR E FLAGS
# =======================================

CXX = g++
CXXFLAGS = -O3
SRC_DIR = src
BUILD_DIR = build
ENTRADA ?= Instancias/instancia.txt


# =======================================
# CPLEX
# =======================================

CPLEX_INCLUDE = -I/opt/ibm/ILOG/CPLEX_Studio128/cplex/include \
                -I/opt/ibm/ILOG/CPLEX_Studio128/concert/include

CPLEX_LPATH = -L/opt/ibm/ILOG/CPLEX_Studio128/concert/lib/x86-64_linux/static_pic \
              -L/opt/ibm/ILOG/CPLEX_Studio128/cplex/lib/x86-64_linux/static_pic

CPLEX_LIBRARIES = -lconcert -lilocplex -lcplex -lpthread -ldl

FLAGS = -DIL_STD -fPIC -fno-strict-aliasing -fexceptions -DNDEBUG -w


# =======================================
# DIRETÓRIOS DE CÓDIGO
# =======================================

METAHEURISTICA_DIR = $(SRC_DIR)/metaheuristica
MODELO_MAT_DIR = $(SRC_DIR)/modelo_matematico


# =======================================
# EXECUTÁVEIS
# =======================================

EXEC_HEURISTICA = heuristica.exe
EXEC_MODELO = modelo.exe


# =======================================
# ARQUIVOS DA METAHEURÍSTICA
# =======================================

SRC_HEURISTICA = \
    $(SRC_DIR)/SBRP.cpp \
    $(METAHEURISTICA_DIR)/buscaTabu.cpp \
    $(METAHEURISTICA_DIR)/genetico.cpp \
    $(METAHEURISTICA_DIR)/metaheuristica.cpp

# Mapeia: src/pasta/arquivo.cpp -> build/pasta/arquivo.o
OBJ_HEURISTICA = $(SRC_HEURISTICA:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)


# =======================================
# ARQUIVOS DO MODELO
# =======================================

SRC_MODELO = \
    $(SRC_DIR)/SBRP.cpp \
    $(MODELO_MAT_DIR)/modeloMain.cpp \
    $(MODELO_MAT_DIR)/modelo.cpp

OBJ_MODELO = $(SRC_MODELO:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)


# =======================================
# ALVOS PRINCIPAIS
# =======================================

all: heuristica modelo

heuristica: $(EXEC_HEURISTICA)

$(EXEC_HEURISTICA): $(OBJ_HEURISTICA)
	$(CXX) $(CXXFLAGS) -o $@ $^

modelo: $(EXEC_MODELO)

$(EXEC_MODELO): $(OBJ_MODELO)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CPLEX_LPATH) $(CPLEX_LIBRARIES)


# =======================================
# REGRA UNIFICADA DE COMPILAÇÃO (.cpp -> .o)
# =======================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPLEX_INCLUDE) $(FLAGS) -c $< -o $@


# =======================================
# EXECUTAR
# =======================================

runHeuristica: clean $(EXEC_HEURISTICA)
	./$(EXEC_HEURISTICA) $(ENTRADA)

runModelo: clean $(EXEC_MODELO)
	./$(EXEC_MODELO) $(ENTRADA)


# =======================================
# LIMPEZA
# =======================================

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXEC_HEURISTICA) $(EXEC_MODELO)

rebuild: clean all