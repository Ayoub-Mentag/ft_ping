CC = gcc
CFLAGS = -Wall -Wextra -Werror

NAME = ft_ping
SRCS = src/main.c src/ping.c
OBJS = $(SRCS:.c=.o)
INCLUDE = include/

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -I $(INCLUDE) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
