import {
  ArrowRight,
  LockKeyhole,
  Mail,
  ShieldCheck,
  Truck,
  Wifi,
} from "lucide-react";
import { useState } from "react";
import { Navigate, useLocation, useNavigate } from "react-router-dom";
import { useAuth } from "../auth/AuthContext";
import { isDemoMode } from "../lib/api";

export function LoginPage() {
  const { authenticated, login, enterDemo } = useAuth();
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const navigate = useNavigate();
  const location = useLocation();
  const destination =
    (location.state as { from?: { pathname?: string } } | null)?.from
      ?.pathname ?? "/";

  if (authenticated) return <Navigate to={destination} replace />;

  const submit = async (event: React.FormEvent) => {
    event.preventDefault();
    setSubmitting(true);
    setError("");
    try {
      await login(email, password);
      navigate(destination, { replace: true });
    } catch (reason) {
      setError(
        reason instanceof Error
          ? reason.message
          : "No fue posible iniciar sesión",
      );
    } finally {
      setSubmitting(false);
    }
  };

  const demo = async () => {
    setSubmitting(true);
    await enterDemo();
    navigate(destination, { replace: true });
  };

  return (
    <main className="login-page">
      <section className="login-visual">
        <div className="login-brand">
          <span>
            <Truck size={25} />
          </span>{" "}
          Vehicle<strong>Sense</strong>
        </div>
        <div className="login-visual__content">
          <span className="login-kicker">
            <Wifi size={15} /> Telemetría segura en tiempo real
          </span>
          <h1>Conoce el estado de cada vehículo, sin perder el contexto.</h1>
          <p>
            Supervisa ubicación, sensores, conectividad, viajes y eventos
            acústicos desde una sola plataforma.
          </p>
          <div className="login-trust">
            <ShieldCheck size={22} />
            <div>
              <strong>Arquitectura protegida</strong>
              <span>El navegador nunca recibe credenciales del broker.</span>
            </div>
          </div>
        </div>
      </section>
      <section className="login-form-wrap">
        <form className="login-card" onSubmit={submit}>
          <div className="login-card__mark">
            <Truck size={24} />
          </div>
          <h2>Bienvenido</h2>
          <p>Accede al centro de monitoreo VehicleSense.</p>
          {error && (
            <div className="form-error" role="alert">
              {error}
            </div>
          )}
          {!isDemoMode && (
            <>
              <label>
                Correo electrónico
                <span className="input-wrap">
                  <Mail size={17} />
                  <input
                    type="email"
                    value={email}
                    onChange={(event) => setEmail(event.target.value)}
                    required
                    autoComplete="email"
                  />
                </span>
              </label>
              <label>
                Contraseña
                <span className="input-wrap">
                  <LockKeyhole size={17} />
                  <input
                    type="password"
                    value={password}
                    onChange={(event) => setPassword(event.target.value)}
                    required
                    autoComplete="current-password"
                    minLength={8}
                  />
                </span>
              </label>
              <button
                className="button button--primary button--wide"
                disabled={submitting}
                type="submit"
              >
                Entrar <ArrowRight size={17} />
              </button>
            </>
          )}
          {isDemoMode && (
            <>
              <div className="demo-login-note">
                Este build usa datos sintéticos identificados. No reemplaza una
                conexión con el backend.
              </div>
              <button
                className="button button--primary button--wide"
                disabled={submitting}
                type="button"
                onClick={demo}
              >
                Entrar a la demostración <ArrowRight size={17} />
              </button>
            </>
          )}
        </form>
      </section>
    </main>
  );
}
