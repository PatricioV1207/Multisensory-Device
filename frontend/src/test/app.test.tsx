import { screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it } from "vitest";
import { resetDemoState } from "../lib/api";
import { clearTokens } from "../lib/session";
import { authenticateDemo, renderApp } from "./render";

describe("VehicleSense application", () => {
  beforeEach(() => {
    clearTokens();
    resetDemoState();
  });

  it("enters the explicitly labelled demo and renders fleet data", async () => {
    const user = userEvent.setup();
    renderApp("/login");
    expect(
      screen.getByText(/datos sintéticos identificados/i),
    ).toBeInTheDocument();
    await user.click(
      screen.getByRole("button", { name: /entrar a la demostración/i }),
    );
    expect(
      await screen.findByRole("heading", { name: /dashboard de flota/i }),
    ).toBeInTheDocument();
    expect(
      screen.getByText(
        /todos los vehículos y mediciones visibles son simulados/i,
      ),
    ).toBeInTheDocument();
    expect(
      screen.getAllByText("3", { selector: ".metric-card__value" }).length,
    ).toBeGreaterThan(0);
  });

  it("shows invalid GPS and microphone as unavailable instead of zero", async () => {
    authenticateDemo();
    renderApp("/vehicles/vehicle-03");
    expect(
      await screen.findByRole("heading", { name: "Vehículo 03" }),
    ).toBeInTheDocument();
    expect(screen.getAllByText(/sin fix gps/i).length).toBeGreaterThan(0);
    expect(screen.getAllByText("No válido").length).toBeGreaterThan(0);
    expect(screen.queryByText("0 km/h")).not.toBeInTheDocument();
  });

  it("acknowledges an alert through the role-protected workflow", async () => {
    const user = userEvent.setup();
    authenticateDemo();
    renderApp("/alerts");
    const acknowledge = await screen.findAllByRole("button", {
      name: /reconocer/i,
    });
    await user.click(acknowledge[0]);
    await waitFor(() =>
      expect(screen.getAllByText("Reconocida").length).toBeGreaterThan(0),
    );
  });

  it("opens and closes the responsive navigation", async () => {
    const user = userEvent.setup();
    authenticateDemo();
    const { container } = renderApp("/");
    const open = await screen.findByRole("button", {
      name: /abrir navegación/i,
    });
    await user.click(open);
    expect(container.querySelector(".sidebar")).toHaveClass("sidebar--open");
    await user.click(
      screen.getByRole("button", { name: /^cerrar navegación$/i }),
    );
    expect(container.querySelector(".sidebar")).not.toHaveClass(
      "sidebar--open",
    );
  });
});
