# Variables
CC = gcc
DEPS = serveur.h
OBJ = serveur.c client.c
TARGETS = $(OBJ:.c=)

# Règles pour les fichiers objets
%: %.c $(DEPS)
	$(CC) -o $@ $<

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)