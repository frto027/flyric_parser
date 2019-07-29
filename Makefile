
CC=cc
PROJECT_DIR=.
OBJ_DIR=$(PROJECT_DIR)/obj
SRC_DIR=$(PROJECT_DIR)/src
INCLUDE_DIR=$(PROJECT_DIR)/include
BIN_DIR=$(PROJECT_DIR)/bin

# compile options for test
libs+= -L$(OBJ_DIR) -lfparser -lm
incl+= -I$(INCLUDE_DIR)

all:
	[ -d $(INCLUDE_DIR) ] || mkdir $(INCLUDE_DIR)
	[ -d $(OBJ_DIR) ] || mkdir $(OBJ_DIR)
	cd $(SRC_DIR)&&make
test:$(BIN_DIR)/test.timeline $(BIN_DIR)/test.memcheck $(BIN_DIR)/test.normal
	

$(BIN_DIR)/test.timeline:test.c all
	[ -d $(BIN_DIR) ]||mkdir $(BIN_DIR)
	$(CC) -o $@ test.c -DTIMELINE_TEST $(incl) $(libs)
$(BIN_DIR)/test.memcheck:test.c all
	[ -d $(BIN_DIR) ]||mkdir $(BIN_DIR)
	$(CC) -o $@ test.c -DMEMORY_LEAK_TEST $(incl) $(libs)
$(BIN_DIR)/test.normal:test.c all
	[ -d $(BIN_DIR) ]||mkdir $(BIN_DIR)
	$(CC) -o $@ test.c -DNORMAL_TEST $(incl) $(libs)

cleanall:clean
	cd $(SRC_DIR)&&make clean

clean:
	rm -rf ./obj/* ./include/* ./bin/*
#	rm -r $(OBJ_DIR)/* $(INCLUDE_DIR)/* $(BIN_DIR)/*


