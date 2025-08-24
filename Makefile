CC = gcc
CFLAGS = -Wall -Wextra -Werror -I $(INCLUDE)

NAME = ft_ping
SRCS = src/ping.c
BONUS_SRCS = src/ping_bonus.c
OBJS = $(SRCS:.c=.o)
BONUS_OBJS = $(BONUS_SRCS:.c=.o)
INCLUDE = include/
HEADERS = include/ping.h
BONUS_NAME=ft_ping_bonus

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

bonus: $(BONUS_NAME)

$(BONUS_NAME): $(BONUS_OBJS)
	$(CC) $(CFLAGS) $(BONUS_OBJS) -o $(BONUS_NAME)

clean:
	rm -f $(OBJS) $(BONUS_OBJS)

fclean: clean
	rm -f $(NAME) $(BONUS_NAME)

re: fclean all

docker: docker-clean docker-build docker-run

docker-build:
	@echo $(CYAN)"Building Docker image..."$(RESET)
	@docker build -t $(NAME)_image .

docker-run:
	@echo $(CYAN)"Running Docker container..."$(RESET)
	@docker run -it -v $(shell pwd):/$(NAME) $(NAME)_image

docker-clean:
	@docker rm $$(docker ps -a -q) 2>/dev/null || true
	@docker volume rm $$(docker volume ls -q) 2>/dev/null || true
	@docker network rm $$(docker network ls -q) 2>/dev/null || true
	@docker rmi $(NAME)_image 2>/dev/null || true
	@echo $(MAGENTA)"Docker cleanup complete!"$(RESET)

.PHONY: all clean fclean re docker docker-clean docker-build docker-run