CC = gcc
CFLAGS = -Wall -Wextra -Werror -I $(INCLUDE)

NAME = ft_ping
SRCS = src/ping.c
BONUS_SRCS = src/ping_bonus.c
OBJS = $(SRCS:.c=.o)
BONUS_OBJS = $(BONUS_SRCS:.c=.o)
INCLUDE = include/
BONUS_NAME=ft_ping_bonus
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

bonus: $(BONUS_OBJS)
	$(CC) $(CFLAGS) $(BONUS_OBJS) -o $(BONUS_NAME)

clean:
	rm -f $(OBJS) $(BONUS_OBJS)

fclean: clean
	rm -f $(NAME) $(BONUS_NAME)

re: fclean all
