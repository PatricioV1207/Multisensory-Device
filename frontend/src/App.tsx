import { lazy, Suspense } from "react";
import { Navigate, Route, Routes } from "react-router-dom";
import { ProtectedRoute } from "./auth/ProtectedRoute";
import { PageLoader } from "./components/ui/AsyncState";
import { AppShell } from "./layout/AppShell";
import { LoginPage } from "./pages/LoginPage";

const DashboardPage = lazy(async () => ({
  default: (await import("./pages/DashboardPage")).DashboardPage,
}));
const LiveMapPage = lazy(async () => ({
  default: (await import("./pages/LiveMapPage")).LiveMapPage,
}));
const VehiclesPage = lazy(async () => ({
  default: (await import("./pages/VehiclesPage")).VehiclesPage,
}));
const VehicleDetailPage = lazy(async () => ({
  default: (await import("./pages/VehicleDetailPage")).VehicleDetailPage,
}));
const AlertsPage = lazy(async () => ({
  default: (await import("./pages/AlertsPage")).AlertsPage,
}));
const TripsPage = lazy(async () => ({
  default: (await import("./pages/TripsPage")).TripsPage,
}));
const AnalyticsPage = lazy(async () => ({
  default: (await import("./pages/AnalyticsPage")).AnalyticsPage,
}));
const ReportsPage = lazy(async () => ({
  default: (await import("./pages/ReportsPage")).ReportsPage,
}));
const DevicesPage = lazy(async () => ({
  default: (await import("./pages/DevicesPage")).DevicesPage,
}));
const SettingsPage = lazy(async () => ({
  default: (await import("./pages/SettingsPage")).SettingsPage,
}));

export function App() {
  return (
    <Suspense fallback={<PageLoader />}>
      <Routes>
        <Route path="/login" element={<LoginPage />} />
        <Route
          path="/"
          element={
            <ProtectedRoute>
              <AppShell />
            </ProtectedRoute>
          }
        >
          <Route index element={<DashboardPage />} />
          <Route path="map" element={<LiveMapPage />} />
          <Route path="vehicles" element={<VehiclesPage />} />
          <Route path="vehicles/:vehicleId" element={<VehicleDetailPage />} />
          <Route path="alerts" element={<AlertsPage />} />
          <Route path="trips" element={<TripsPage />} />
          <Route path="analytics" element={<AnalyticsPage />} />
          <Route path="reports" element={<ReportsPage />} />
          <Route path="devices" element={<DevicesPage />} />
          <Route path="settings" element={<SettingsPage />} />
        </Route>
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </Suspense>
  );
}
