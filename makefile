# =======================================
# CONFIGURAÇÃO CPLEX
# =======================================

INCLUDE = -I/opt/ibm/ILOG/CPLEX_Studio128/cplex/include \
          -I/opt/ibm/ILOG/CPLEX_Studio128/concert/include

FLAGS = -DIL_STD -fPIC -fno-strict-aliasing -fexceptions -DNDEBUG -w

LPATH = -L/opt/ibm/ILOG/CPLEX_Studio128/concert/lib/x86-64_linux/static_pic \
        -L/opt/ibm/ILOG/CPLEX_Studio128/cplex/lib/x86-64_linux/static_pic

LIBRARIES = -lconcert -lilocplex -lcplex -lpthread -ldl


# =======================================
# COMPILADOR
# =======================================

CXX = g++
CXXFLAGS = -O3

BUILD_DIR = build

ENTRADA ?= entrada.txt


# =======================================
# DIRETÓRIOS
# =======================================

METAHEURISTICA_DIR = metaheuristica
MODELO_MAT_DIR = modelo_matematico


# =======================================
# EXECUTÁVEIS
# =======================================

EXEC_HEURISTICA = heuristica.exe
EXEC_MODELO = modelo.exe


# =======================================
# ARQUIVOS DA METAHEURÍSTICA
# =======================================

SRC_HEURISTICA = \
    SBRP.cpp \
    $(METAHEURISTICA_DIR)/buscaTabu.cpp \
    $(METAHEURISTICA_DIR)/genetico.cpp \
    $(METAHEURISTICA_DIR)/metaheuristica.cpp

OBJ_HEURISTICA = $(SRC_HEURISTICA:%.cpp=$(BUILD_DIR)/%.o)


# =======================================
# ARQUIVOS DO MODELO
# =======================================

SRC_MODELO = \
    $(MODELO_MAT_DIR)/modeloMain.cpp \
    $(MODELO_MAT_DIR)/modelo.cpp

OBJ_MODELO = $(SRC_MODELO:%.cpp=$(BUILD_DIR)/%.o)


# =======================================
# ALVO PADRÃO
# =======================================

all: heuristica modelo


# =======================================
# METAHEURÍSTICA
# =======================================

heuristica: $(EXEC_HEURISTICA)

$(EXEC_HEURISTICA): $(OBJ_HEURISTICA)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LPATH) $(LIBRARIES)


# =======================================
# MODELO MATEMÁTICO
# =======================================

modelo: $(EXEC_MODELO)

$(EXEC_MODELO): $(OBJ_MODELO)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LPATH) $(LIBRARIES)


# =======================================
# COMPILAÇÃO DOS .CPP
# =======================================

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(FLAGS) -c $< -o $@


# =======================================
# EXECUTAR METAHEURÍSTICA
# =======================================

runHeuristica: $(EXEC_HEURISTICA)
	./$(EXEC_HEURISTICA) $(ENTRADA)


# =======================================
# EXECUTAR MODELO
# =======================================

runModelo: $(EXEC_MODELO)
	./$(EXEC_MODELO) $(ENTRADA)


# =======================================
# LIMPEZA
# =======================================

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXEC_HEURISTICA) $(EXEC_MODELO)


# =======================================
# RECOMPILAR TUDO
# =======================================

rebuild: clean all