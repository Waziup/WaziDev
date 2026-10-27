FROM python:3.12-slim

WORKDIR /app
COPY wqi.py main.py objectives.json ./
COPY ui ./ui

# Installing from the dashboard pulls this image and unpacks /index.zip
# (docker-compose.yml + package.json) into /var/lib/wazigate/apps/<app id>/.
COPY docker-compose.yml package.json /tmp/pkg/
RUN python -c "import zipfile; z = zipfile.ZipFile('/index.zip', 'w'); \
[z.write('/tmp/pkg/' + f, f) for f in ('docker-compose.yml', 'package.json')]; z.close()"

CMD ["python", "-u", "main.py"]
