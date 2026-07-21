import type { PropsWithChildren } from "react";
import { Navigate, useLocation } from "react-router-dom";
import { useAuth } from "./AuthContext";
import { PageLoader } from "../components/ui/AsyncState";

export function ProtectedRoute({ children }: PropsWithChildren) {
  const { authenticated, loading } = useAuth();
  const location = useLocation();
  if (loading) return <PageLoader label="Verificando sesión…" />;
  if (!authenticated)
    return <Navigate to="/login" replace state={{ from: location }} />;
  return children;
}
