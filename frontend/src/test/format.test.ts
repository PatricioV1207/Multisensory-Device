import { describe, expect, it } from "vitest";
import { numberOrDash, valueWithUnit, vectorMagnitude } from "../lib/format";

describe("telemetry formatting", () => {
  it("never substitutes zero for missing or non-finite values", () => {
    expect(numberOrDash(null)).toBe("—");
    expect(numberOrDash(Number.NaN)).toBe("—");
    expect(valueWithUnit(undefined, "km/h")).toBe("—");
    expect(valueWithUnit(0, "km/h", 0)).toBe("0 km/h");
  });

  it("computes vector magnitude only for complete finite vectors", () => {
    expect(vectorMagnitude([3, 4, 0])).toBe(5);
    expect(vectorMagnitude(null)).toBeNull();
    expect(vectorMagnitude([1, Number.NaN, 2])).toBeNull();
  });
});
