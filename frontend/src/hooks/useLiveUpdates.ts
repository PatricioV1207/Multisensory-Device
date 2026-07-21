import { useQueryClient } from "@tanstack/react-query";
import { useEffect, useRef, useState } from "react";
import { isDemoMode } from "../lib/api";
import { getAccessToken } from "../lib/session";
import type { LiveMessage } from "../types/api";

export type LiveConnectionState =
  "connecting" | "connected" | "reconnecting" | "offline" | "demo";

function websocketUrl(): string {
  const configured = import.meta.env.VITE_WS_BASE_URL?.replace(/\/$/, "");
  if (configured) return `${configured}/ws/v1/live`;
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}/ws/v1/live`;
}

export function useLiveUpdates(vehicleIds: string[] = []): LiveConnectionState {
  const queryClient = useQueryClient();
  const [state, setState] = useState<LiveConnectionState>(
    isDemoMode ? "demo" : getAccessToken() ? "connecting" : "offline",
  );
  const reconnectAttempt = useRef(0);
  const subscriptionKey = [...vehicleIds].sort().join(",");

  useEffect(() => {
    if (isDemoMode) return;
    if (!getAccessToken()) return;
    const subscriptionIds = subscriptionKey ? subscriptionKey.split(",") : [];
    let socket: WebSocket | null = null;
    let timer: number | null = null;
    let closedByEffect = false;

    const connect = () => {
      setState(reconnectAttempt.current ? "reconnecting" : "connecting");
      socket = new WebSocket(websocketUrl());
      socket.onopen = () => {
        const currentToken = getAccessToken();
        if (!currentToken) {
          socket?.close();
          return;
        }
        socket?.send(
          JSON.stringify({ action: "authenticate", token: currentToken }),
        );
      };
      socket.onmessage = (event) => {
        let message: LiveMessage;
        try {
          message = JSON.parse(event.data) as LiveMessage;
        } catch {
          return;
        }
        if (message.type === "connection.ready") {
          reconnectAttempt.current = 0;
          setState("connected");
          socket?.send(
            JSON.stringify({
              action: "subscribe",
              vehicle_ids: subscriptionIds,
            }),
          );
          return;
        }
        if (
          ["telemetry.received", "device.status", "alert.changed"].includes(
            message.type,
          )
        ) {
          void queryClient.invalidateQueries({ queryKey: ["dashboard"] });
          void queryClient.invalidateQueries({ queryKey: ["vehicles"] });
        }
        if (message.vehicle_id) {
          void queryClient.invalidateQueries({
            queryKey: ["vehicle", message.vehicle_id],
          });
          void queryClient.invalidateQueries({
            queryKey: ["telemetry", message.vehicle_id],
          });
          void queryClient.invalidateQueries({
            queryKey: ["route", message.vehicle_id],
          });
          void queryClient.invalidateQueries({
            queryKey: ["acoustic", message.vehicle_id],
          });
        }
        if (message.type === "alert.changed") {
          void queryClient.invalidateQueries({ queryKey: ["alerts"] });
        }
      };
      socket.onclose = () => {
        if (closedByEffect) return;
        reconnectAttempt.current += 1;
        setState("reconnecting");
        const delay = Math.min(
          30_000,
          1_000 * 2 ** Math.min(reconnectAttempt.current, 5),
        );
        timer = window.setTimeout(connect, delay);
      };
      socket.onerror = () => socket?.close();
    };

    connect();
    return () => {
      closedByEffect = true;
      if (timer != null) window.clearTimeout(timer);
      socket?.close();
    };
  }, [queryClient, subscriptionKey]);

  return state;
}
