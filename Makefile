CC = gcc
CFLAGS = -Wall -Wextra -Werror -I $(INCLUDE)

NAME = ft_ping
SRCS = src/ping.c
OBJS = $(SRCS:.c=.o)
INCLUDE = include/

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
