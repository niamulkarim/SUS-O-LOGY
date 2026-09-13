SOURCES = core_version_4.c game_version_4.c
OUTPUT  = test_version_4.exe

$(OUTPUT): $(SOURCES)
	gcc $(SOURCES) -o $(OUTPUT) -L . -I . -lraylib -lopengl32 -lgdi32 -lwinmm

clean:
	del $(OUTPUT)