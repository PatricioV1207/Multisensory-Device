from __future__ import annotations

import logging
import logging.config


def configure_logging(environment: str) -> None:
    level = "DEBUG" if environment == "development" else "INFO"
    logging.config.dictConfig(
        {
            "version": 1,
            "disable_existing_loggers": False,
            "formatters": {
                "default": {
                    "format": "%(asctime)s %(levelname)s %(name)s %(message)s",
                }
            },
            "handlers": {
                "console": {
                    "class": "logging.StreamHandler",
                    "formatter": "default",
                }
            },
            "root": {"handlers": ["console"], "level": level},
        }
    )


def redact_connection_url(url: str) -> str:
    if "://" not in url or "@" not in url:
        return url
    scheme, remainder = url.split("://", 1)
    _, host = remainder.rsplit("@", 1)
    return f"{scheme}://***:***@{host}"
