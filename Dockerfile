# Multi-Stage Lightweight Container for ISHA AI Cognitive Core & Studios
FROM python:3.11-slim AS builder

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir --prefix=/install -r requirements.txt

# Final Runtime Image
FROM python:3.11-slim AS runner

WORKDIR /app

COPY --from=builder /install /usr/local
COPY backend/ ./backend/
COPY frontend/ ./frontend/
COPY run_isha.py run_desktop.py SYSTEM_DESIGN.md README.md ./

EXPOSE 8000
ENV PORT=8000
ENV HOST=0.0.0.0

HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://localhost:8000/api/health')" || exit 1

CMD ["python", "run_isha.py"]
