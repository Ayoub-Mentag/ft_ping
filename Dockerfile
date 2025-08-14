FROM debian:bullseye

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    zsh \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install Oh My Zsh (non-interactive)
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

# Set default shell to zsh
SHELL ["/bin/zsh", "-c"]

# Set work directory
WORKDIR /app

# Copy project sources
COPY . .

# Optional: build your project
# RUN make

# Default command when container starts
CMD ["zsh"]
