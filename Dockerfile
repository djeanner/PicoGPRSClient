# Use official Node LTS image for amd64 compatibility
FROM --platform=linux/amd64 node:18-alpine

# Set working directory
WORKDIR /app

# Copy package files
COPY package*.json ./

# Install dependencies
RUN npm install --production

# Copy source
COPY . .

# Expose port
EXPOSE 3000

# Start the server
CMD ["node", "server.js"]
