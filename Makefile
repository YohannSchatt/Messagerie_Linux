# Variables
CC = gcc
CFLAGS := -Wall -Wextra -std=c99 -Wpedantic -fsanitize=address,undefined -g
DEPS = serveur.h
OBJ = serveur.o client.o file.o annexe_serveur.o communication_serveur.o salon.o

# Règle par défaut
all: $(OBJ)

# Règles pour les fichiers objets
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Règles pour les fichiers objets
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Règle de nettoyage
clean:
	rm -f $(OBJ)