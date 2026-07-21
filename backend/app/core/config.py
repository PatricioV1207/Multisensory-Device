from __future__ import annotations

from functools import lru_cache
from pathlib import Path
from typing import Literal

from pydantic import Field, SecretStr, field_validator
from pydantic_settings import BaseSettings, SettingsConfigDict

REPOSITORY_ROOT = Path(__file__).resolve().parents[3]


class Settings(BaseSettings):
    model_config = SettingsConfigDict(
        env_file=".env",
        env_prefix="VEHICLESENSE_",
        case_sensitive=False,
        extra="ignore",
    )

    app_name: str = "VehicleSense API"
    environment: Literal["development", "test", "production"] = "development"
    api_prefix: str = "/api/v1"
    database_url: str = "postgresql+asyncpg://vehiclesense:vehiclesense@localhost/vehiclesense"
    auto_create_schema: bool = False
    sql_echo: bool = False
    cors_origins: list[str] = Field(default_factory=lambda: ["http://localhost:5173"])
    contracts_directory: Path = REPOSITORY_ROOT / "contracts" / "schemas"

    jwt_secret: SecretStr = SecretStr("development-only-change-me-32-characters")
    jwt_algorithm: str = "HS256"
    access_token_minutes: int = 15
    refresh_token_days: int = 7
    bootstrap_admin_email: str | None = None
    bootstrap_admin_password: SecretStr | None = None

    mqtt_enabled: bool = False
    mqtt_host: str = ""
    mqtt_port: int = 8883
    mqtt_username: str = ""
    mqtt_password: SecretStr = SecretStr("")
    mqtt_client_id: str = "vehiclesense-backend-development"
    mqtt_topic_prefix: str = "vehiclesense/v1"
    mqtt_ca_file: Path | None = None
    mqtt_keepalive_seconds: int = 30

    stale_after_seconds: int = 45
    offline_after_seconds: int = 120
    status_scan_seconds: int = 15
    auto_register_devices: bool = False
    maximum_mqtt_payload_bytes: int = 65_536

    trip_start_speed_kmh: float = 5.0
    trip_start_duration_seconds: int = 20
    trip_stop_speed_kmh: float = 2.0
    trip_stop_duration_seconds: int = 120
    trip_max_jump_speed_kmh: float = 250.0

    @field_validator("jwt_secret")
    @classmethod
    def secure_production_secret(cls, value: SecretStr, info):
        environment = info.data.get("environment", "development")
        if environment == "production" and len(value.get_secret_value()) < 32:
            raise ValueError("jwt_secret must contain at least 32 characters in production")
        return value

    @field_validator("mqtt_port")
    @classmethod
    def valid_mqtt_port(cls, value: int) -> int:
        if value < 1 or value > 65_535:
            raise ValueError("mqtt_port must be a valid TCP port")
        return value


@lru_cache
def get_settings() -> Settings:
    return Settings()
