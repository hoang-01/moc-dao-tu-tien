"""Cấu hình ứng dụng Backend — đọc từ biến môi trường (.env)."""

from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """Cấu hình trung tâm cho toàn bộ Backend."""

    # --- Database ---
    database_url: str = (
        "postgresql+asyncpg://postgres:password@localhost:5432/moc_dao_tu_tien"
    )

    # --- JWT ---
    jwt_secret_key: str = "dev-secret-key-change-in-production"
    jwt_algorithm: str = "HS256"
    access_token_expire_minutes: int = 60
    refresh_token_expire_days: int = 30

    # --- Google OAuth ---
    google_client_id: str = ""
    google_client_secret: str = ""

    # --- MQTT ---
    mqtt_broker_host: str = "localhost"
    mqtt_broker_port: int = 1883
    mqtt_username: str = ""
    mqtt_password: str = ""

    # --- App ---
    app_env: str = "development"
    app_debug: bool = True
    cors_origins: list[str] = [
        "http://localhost:3000",
        "http://localhost:5173",
    ]

    model_config = {"env_file": ".env", "env_file_encoding": "utf-8"}


settings = Settings()
