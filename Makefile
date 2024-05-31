# Variables
CC = gcc
DEPS = serveur.h
CFLAGS := -Wall -Wextra -std=c99 -Wpedantic -fsanitize=address,undefined -g
OBJ = serveur.c client.c
TARGETS = $(OBJ:.c=)

# Règles pour les fichiers objets
%: %.c $(DEPS)
	$(CC) -o $@ $< $(CFLAGS)

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)