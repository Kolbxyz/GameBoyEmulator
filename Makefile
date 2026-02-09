##
## EPITECH PROJECT, 2026
## G-PSU-100-MLH-1-1-mytop-1
## File description:
## Makefile
##

SRC =  	src/utils/throw_error.c	\
		src/memory/read_rom.c	\
		src/cpu/cpu_add.c		\
		src/cpu/cpu_dec.c		\
		src/cpu/cpu_sub.c		\
		src/cpu/cpu_inc.c		\
		src/cpu/cpu_cp.c		\
		src/cpu/cpu_logical.c	\
		src/cpu/cpu_srl.c		\
		src/cpu/cpu_cb_funcs.c	\
		src/cpu/stack.c			\
		src/cpu/execute.c		\
		src/vram.c				\
		src/ppu.c				\
		src/timer.c				\
		src/apu.c				\
		src/save.c				\
		src/utils/memory_ops.c	\

NAME = emulator

MAIN = src/main.c

OBJ = $(SRC:.c=.o) $(MAIN:.c=.o)

LIBS = -lSDL2

CC = clang

OPTIONS = -I./include

all: $(OBJ)
	@echo "📂 Compiling..."
	@$(CC) $(OPTIONS) $(OBJ) -o $(NAME) $(LIBS)
	@echo "🥬 Done! ./$(NAME) to execute!"

%.o: %.c
	@$(CC) $(OPTIONS) -c $< -o $@

scan:
	@gcc -fanalyzer -Wanalyzer-possible-null-dereference $(OPTIONS) -c $(SRC) $(MAIN)

debug: OPTIONS += -g
debug: all

clean:
	@echo "🧹 Cleaning up..."
	@rm -f $(OBJ)

fclean: clean
	@echo "🗑️ Removing binary..."
	@rm -f $(NAME)
	@echo "🚮 Removed!"

leaks: OPTIONS += -g -fsanitize=address
leaks: all

style-check: CC = epiclang
style-check: re

dev: CC = epiclang
dev: OPTIONS += -g
dev: re

re: fclean all

.PHONY: all clean fclean leaks style-check dev debug re
