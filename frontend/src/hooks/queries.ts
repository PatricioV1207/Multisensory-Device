import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { api } from "../lib/api";

export const useDashboard = () =>
  useQuery({
    queryKey: ["dashboard"],
    queryFn: api.dashboard,
    refetchInterval: 30_000,
  });

export const useVehicles = () =>
  useQuery({ queryKey: ["vehicles"], queryFn: api.vehicles });

export const useVehicle = (vehicleId: string) =>
  useQuery({
    queryKey: ["vehicle", vehicleId],
    queryFn: () => api.vehicle(vehicleId),
    enabled: Boolean(vehicleId),
  });

export const useTelemetry = (vehicleId: string, hours = 24) =>
  useQuery({
    queryKey: ["telemetry", vehicleId, hours],
    queryFn: () => api.telemetry(vehicleId, hours),
    enabled: Boolean(vehicleId),
  });

export const useRoute = (vehicleId: string, hours = 6) =>
  useQuery({
    queryKey: ["route", vehicleId, hours],
    queryFn: () => api.route(vehicleId, hours),
    enabled: Boolean(vehicleId),
  });

export const useAcoustic = (vehicleId: string, hours = 24) =>
  useQuery({
    queryKey: ["acoustic", vehicleId, hours],
    queryFn: () => api.acoustic(vehicleId, hours),
    enabled: Boolean(vehicleId),
  });

export const useAlerts = (status?: string) =>
  useQuery({ queryKey: ["alerts", status], queryFn: () => api.alerts(status) });

export const useTrips = (vehicleId?: string) =>
  useQuery({
    queryKey: ["trips", vehicleId],
    queryFn: () => api.trips(vehicleId),
  });

export const useDevices = () =>
  useQuery({ queryKey: ["devices"], queryFn: api.devices });

export function useAlertAction() {
  const queryClient = useQueryClient();
  return useMutation({
    mutationFn: ({
      id,
      action,
    }: {
      id: string;
      action: "acknowledge" | "resolve";
    }) => api.updateAlert(id, action),
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: ["alerts"] });
      void queryClient.invalidateQueries({ queryKey: ["dashboard"] });
      void queryClient.invalidateQueries({ queryKey: ["vehicle"] });
    },
  });
}
