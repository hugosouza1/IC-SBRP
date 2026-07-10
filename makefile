INCLUDE=-I/opt/ibm/ILOG/CPLEX_Studio128/cplex/include -I/opt/ibm/ILOG/CPLEX_Studio128/concert/include

FLAGS=-DIL_STD -fPIC -fno-strict-aliasing -fexceptions -DNDEBUG -w

LPATH=-L/opt/ibm/ILOG/CPLEX_Studio128/concert/lib/x86-64_linux/static_pic -L//opt/ibm/ILOG/CPLEX_Studio128/cplex/lib/x86-64_linux/static_pic

LIBRARIES=-lconcert -lilocplex -lcplex -lpthread -ldl

# =======================================

CXX = g++
# CXXFLAGS = -Wall -Wextra -O3
CXXFLAGS = -O3

BUILD_DIR = build

METAHEURISTICA_DIR = metaheuristica
MODELO_MAT_DIR = modelo_matematico

SRC = \
	main.cpp \
	SBRP.cpp \
	$(METAHEURISTICA_DIR)/buscaTabu.cpp \
	$(METAHEURISTICA_DIR)/genetico.cpp



OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)

EXEC = execMaster
ENTRADA ?= entrada.txt

all: build run

build: $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ENTRADA)

# Linkagem
$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilação dos .cpp
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(EXEC)

rebuild: clean all

# =================================================================================================================
# se queiser so o modelo. temporario kk

SO_MODELO = modelo_matematico/modelo

soModeloMat: $(SO_MODELO).o
	$(CXX) -O3 $(SO_MODELO).o -o SO_MODELO.exe $(LPATH) $(LIBRARIES)
	./SO_MODELO.exe $(ENTRADA)

$(SO_MODELO).o: $(SO_MODELO).cpp
	$(CXX) -DIL_STD -O3 -c $(SO_MODELO).cpp -o $(SO_MODELO).o $(INCLUDE) $(FLAGS)

cleanMM:
	rm -f *.exe $(SO_MODELO).o