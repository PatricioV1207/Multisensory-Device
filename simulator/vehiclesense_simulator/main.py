from __future__ import annotations

import argparse
import copy
import json
import logging
import math
import signal
import sys
import threading
import time
from datetime import UTC, datetime

from dotenv import load_dotenv

from vehiclesense_simulator.config import SimulatorSettings
from vehiclesense_simulator.contracts import ContractRegistry
from vehiclesense_simulator.generator import Envelope, VehicleGenerator
from vehiclesense_simulator.publisher import DevicePublisher

logger = logging.getLogger(__name__)


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Publish contract-valid simulated VehicleSense devices"
    )
    parser.add_argument("--dry-run", action="store_true", help="validate without connecting")
    parser.add_argument("--cycles", type=int, default=0, help="0 runs until interrupted")
    parser.add_argument("--vehicles", type=int, help="override environment vehicle count")
    parser.add_argument("--interval", type=float, help="override seconds between cycles")
    parser.add_argument("--scenario", choices=("normal", "mixed", "stress"), default="mixed")
    parser.add_argument("--duplicate-every", type=int, default=0)
    parser.add_argument(
        "--inject-invalid-every",
        type=int,
        default=0,
        help="explicit negative test; publishes a schema-invalid copy",
    )
    parser.add_argument("--print-payloads", action="store_true")
    return parser.parse_args(argv)


def _validate_cli(args: argparse.Namespace, settings: SimulatorSettings) -> tuple[int, float]:
    vehicle_count = settings.vehicle_count if args.vehicles is None else args.vehicles
    interval = settings.interval_seconds if args.interval is None else args.interval
    if vehicle_count < 1 or vehicle_count > 100:
        raise ValueError("--vehicles must be between 1 and 100")
    if not math.isfinite(interval) or interval < 0.1 or interval > 3600:
        raise ValueError("--interval must be between 0.1 and 3600 seconds")
    if args.cycles < 0:
        raise ValueError("--cycles cannot be negative")
    if args.duplicate_every < 0 or args.inject_invalid_every < 0:
        raise ValueError("injection intervals cannot be negative")
    return vehicle_count, interval


def _summary(envelope: Envelope) -> str:
    payload = envelope.payload
    validity = "invalid-injection" if not envelope.validate else "valid"
    return (
        f"{payload.get('vehicle_id')}/{payload.get('device_id')} "
        f"{payload.get('message_type')} seq={payload.get('sequence', '-')} {validity}"
    )


def run(argv: list[str] | None = None) -> int:
    load_dotenv()
    args = arguments(argv)
    try:
        settings = SimulatorSettings.from_environment()
        vehicle_count, interval = _validate_cli(args, settings)
        if not args.dry_run:
            settings.require_broker()
    except ValueError as error:
        print(f"CONFIG ERROR: {error}", file=sys.stderr)
        return 2

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )
    registry = ContractRegistry()
    generators = [
        VehicleGenerator(index, settings.topic_prefix, settings.seed, registry)
        for index in range(1, vehicle_count + 1)
    ]
    publishers: list[DevicePublisher] = []
    if not args.dry_run:
        try:
            publishers = [DevicePublisher(settings, generator) for generator in generators]
            for publisher in publishers:
                publisher.start()
        except Exception as error:
            logger.error("Could not start HiveMQ simulator: %s", error)
            for publisher in publishers:
                publisher.stop()
            return 1

    stop = threading.Event()

    def request_stop(signum, frame) -> None:
        del signum, frame
        stop.set()

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    cycle = 0
    next_deadline = time.monotonic()
    try:
        while not stop.is_set() and (args.cycles == 0 or cycle < args.cycles):
            cycle += 1
            now = datetime.now(UTC)
            invalid_cycle = bool(
                args.inject_invalid_every and cycle % args.inject_invalid_every == 0
            )
            for index, generator in enumerate(generators):
                envelopes = generator.next_cycle(
                    now,
                    scenario=args.scenario,
                    inject_invalid=invalid_cycle and index == 0,
                )
                duplicate = None
                if args.duplicate_every and cycle % args.duplicate_every == 0:
                    duplicate = next(
                        (
                            copy.deepcopy(envelope)
                            for envelope in envelopes
                            if envelope.payload.get("message_type") == "telemetry"
                            and envelope.validate
                        ),
                        None,
                    )
                if duplicate is not None:
                    envelopes.append(duplicate)
                for envelope in envelopes:
                    if args.print_payloads:
                        print(
                            json.dumps(
                                {"topic": envelope.topic, "payload": envelope.payload},
                                separators=(",", ":"),
                                allow_nan=False,
                            )
                        )
                    else:
                        print(_summary(envelope))
                    if publishers:
                        publisher = publishers[index]
                        if (
                            envelope.payload.get("message_type") == "telemetry"
                            and envelope.validate
                        ):
                            envelope.payload["offline_queued"] = publisher.queued
                            envelope.payload["offline_replayed"] = publisher.replayed
                            envelope.payload["offline_dropped"] = publisher.dropped
                            registry.validate(envelope.payload)
                        publisher.publish(envelope)
                if publishers:
                    publishers[index].flush()
            if args.dry_run and args.cycles == 0:
                break
            next_deadline += interval
            stop.wait(max(0.0, next_deadline - time.monotonic()))
    finally:
        for publisher in publishers:
            publisher.stop()
    return 0


def main() -> None:
    raise SystemExit(run())


if __name__ == "__main__":
    main()
