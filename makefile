# =======================================
# COMPILADOR
# =======================================

CXX = g++
CXXFLAGS = -O3
BUILD_DIR = build
ENTRADA ?= instancia.txt


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
    SBRP.cpp \
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
	$(CXX) $(CXXFLAGS) -o $@ $^


# =======================================
# MODELO MATEMÁTICO
# =======================================

modelo: $(EXEC_MODELO)

$(EXEC_MODELO): $(OBJ_MODELO)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CPLEX_LPATH) $(CPLEX_LIBRARIES)


# =======================================
# COMPILAÇÃO DA METAHEURÍSTICA
# =======================================

$(BUILD_DIR)/SBRP.o: SBRP.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(FLAGS) -c $< -o $@

$(BUILD_DIR)/$(METAHEURISTICA_DIR)/%.o: $(METAHEURISTICA_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(FLAGS) -c $< -o $@


# =======================================
# COMPILAÇÃO DO MODELO
# =======================================

$(BUILD_DIR)/$(MODELO_MAT_DIR)/%.o: $(MODELO_MAT_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPLEX_INCLUDE) $(FLAGS) -c $< -o $@


# =======================================
# EXECUTAR METAHEURÍSTICA
# =======================================

runHeuristica: clean $(EXEC_HEURISTICA) heuristica 
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