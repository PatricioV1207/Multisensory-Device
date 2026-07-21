import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
  type PropsWithChildren,
} from "react";
import { api, isDemoMode } from "../lib/api";
import { clearTokens, getAccessToken, saveTokens } from "../lib/session";
import type { UserProfile } from "../types/api";

interface AuthValue {
  user: UserProfile | null;
  loading: boolean;
  authenticated: boolean;
  login(email: string, password: string): Promise<void>;
  enterDemo(): Promise<void>;
  logout(): void;
}

const AuthContext = createContext<AuthValue | null>(null);

export function AuthProvider({ children }: PropsWithChildren) {
  const [user, setUser] = useState<UserProfile | null>(null);
  const [loading, setLoading] = useState(Boolean(getAccessToken()));

  const logout = useCallback(() => {
    clearTokens();
    setUser(null);
  }, []);

  const loadProfile = useCallback(async () => {
    try {
      setUser(await api.me());
    } catch {
      logout();
    } finally {
      setLoading(false);
    }
  }, [logout]);

  useEffect(() => {
    if (getAccessToken()) queueMicrotask(() => void loadProfile());
    const expired = () => logout();
    window.addEventListener("vehiclesense:auth-expired", expired);
    return () =>
      window.removeEventListener("vehiclesense:auth-expired", expired);
  }, [loadProfile, logout]);

  const login = useCallback(async (email: string, password: string) => {
    const tokens = await api.login(email, password);
    saveTokens(tokens);
    setUser(await api.me());
  }, []);

  const enterDemo = useCallback(async () => {
    if (!isDemoMode) return;
    await login("demo@vehiclesense.local", "demonstration");
  }, [login]);

  const value = useMemo(
    () => ({
      user,
      loading,
      authenticated: Boolean(user),
      login,
      enterDemo,
      logout,
    }),
    [user, loading, login, enterDemo, logout],
  );
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

// eslint-disable-next-line react-refresh/only-export-components
export function useAuth(): AuthValue {
  const value = useContext(AuthContext);
  if (!value) throw new Error("useAuth must be used within AuthProvider");
  return value;
}
