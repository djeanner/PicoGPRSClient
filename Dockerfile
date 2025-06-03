# Use official Node.js 18 LTS Alpine image for amd64 compatibility
FROM --platform=linux/amd64 node:18-alpine

# Set working directory inside the container
WORKDIR /app

# Copy only the necessary files for installing dependencies
COPY package*.json ./

# Install only production dependencies (faster and leaner)
RUN npm ci --omit=dev

# Copy the rest of the application files
COPY . .

# Ensure server starts only after all files are in place
# (Optional: RUN npm run build if using a build step)

# Expose the port your app listens on
EXPOSE 3000

# Define environment variables (optional, override in Jelastic UI)
# ENV NODE_ENV=production

# Run the application
CMD ["node", "server.js"]
