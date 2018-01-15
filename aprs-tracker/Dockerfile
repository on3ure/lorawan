FROM wp-alpha-registry/mojolicious:latest

# Your App
ADD app /app

# Carton install
RUN (cd /app && carton install --deployment)

# Make it executable
RUN chmod a+rx /app/run.sh
