#pragma once

#include <Arduino.h>

namespace WebAssets {

static const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bus IoT Monitor</title><style>
:root{color-scheme:dark;--bg:#08111f;--panel:#111d2e;--line:#243550;--text:#e9f1ff;--muted:#8fa4c2;--ok:#35d07f;--bad:#ff6b6b;--accent:#57a6ff}
*{box-sizing:border-box}body{margin:0;font:15px system-ui,sans-serif;background:linear-gradient(135deg,#08111f,#10223a);color:var(--text);min-height:100vh}
header{padding:24px max(20px,5vw);display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--line)}
h1{font-size:clamp(20px,4vw,32px);margin:0}a{color:var(--accent)}main{padding:24px max(20px,5vw)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:14px}.card{background:rgba(17,29,46,.9);border:1px solid var(--line);border-radius:16px;padding:18px;box-shadow:0 12px 30px #0004}
.label{color:var(--muted);font-size:13px;text-transform:uppercase;letter-spacing:.08em}.value{font-size:25px;font-weight:700;margin-top:8px}.status{font-size:14px}.ok{color:var(--ok)}.bad{color:var(--bad)}
footer{padding:22px max(20px,5vw);color:var(--muted)}
</style></head><body><header><div><h1>Bus IoT Monitor</h1><div id="device" class="label">Conectando…</div></div><a href="/admin">Administrador</a></header>
<main><div class="grid">
<section class="card"><div class="label">Temperatura</div><div id="temp" class="value">—</div></section>
<section class="card"><div class="label">Humedad</div><div id="hum" class="value">—</div></section>
<section class="card"><div class="label">Luz</div><div id="lux" class="value">—</div></section>
<section class="card"><div class="label">Presión</div><div id="pressure" class="value">—</div></section>
<section class="card"><div class="label">Altitud barométrica</div><div id="alt" class="value">—</div></section>
<section class="card"><div class="label">Resumen IMU</div><div id="imu" class="value">—</div></section>
<section class="card"><div class="label">microSD</div><div id="sd" class="value status">—</div></section>
<section class="card"><div class="label">SIM800L / GPRS</div><div id="sim" class="value status">—</div></section>
<section class="card"><div class="label">MQTT</div><div id="mqtt" class="value status">—</div></section>
<section class="card"><div class="label">Uptime</div><div id="uptime" class="value">—</div></section>
<section class="card"><div class="label">Último envío</div><div id="last" class="value">—</div></section>
</div></main><footer>Actualización local cada 2 segundos</footer><script>
const el=id=>document.getElementById(id), num=(v,u,d=1)=>Number.isFinite(v)?v.toFixed(d)+' '+u:'—';
const state=(ok,label)=>`<span class="${ok?'ok':'bad'}">${label}</span>`;
async function refresh(){try{const [t,s]=await Promise.all([fetch('/api/telemetry/basic').then(r=>r.json()),fetch('/api/status').then(r=>r.json())]);
el('device').textContent=s.device_id+' · firmware '+s.firmware_version;el('temp').textContent=num(t.temperature_c,'°C');el('hum').textContent=num(t.humidity_percent,'%');el('lux').textContent=num(t.light_lux,'lx',0);el('pressure').textContent=num(t.pressure_hpa,'hPa',2);el('alt').textContent=num(t.baro_altitude_m,'m',1);
const mags=[num(t.accel_magnitude_mps2,'m/s²',2),num(t.gyro_magnitude_rad_s,'rad/s',2),num(t.mag_magnitude_ut,'µT',1)];el('imu').textContent=t.imu_valid?mags.join(' · '):'Parcial / inválida';
el('sd').innerHTML=state(s.sd.mounted,s.sd.mounted?(s.sd.last_write_ok?'Montada · escritura OK':'Montada · esperando'):'No disponible');el('sim').innerHTML=state(s.sim800l.gprs_connected,s.sim800l.gprs_connected?'GPRS conectado':(s.sim800l.present?'Sin GPRS':'No detectado'));el('mqtt').innerHTML=state(s.mqtt.connected,s.mqtt.connected?'Conectado':'Desconectado');
el('uptime').textContent=Math.floor(s.uptime_ms/1000)+' s';el('last').textContent=s.mqtt.last_publish_ms?Math.floor(s.mqtt.last_publish_ms/1000)+' s':'Nunca';}catch(e){el('device').textContent='Sin respuesta del dispositivo';}}
refresh();setInterval(refresh,2000);
</script></body></html>)HTML";

static const char ADMIN_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Administración Bus IoT</title><style>
body{font:16px system-ui,sans-serif;max-width:680px;margin:50px auto;padding:20px;background:#08111f;color:#edf4ff}section{background:#111d2e;padding:25px;border-radius:16px;border:1px solid #263955}input,button{width:100%;margin-top:14px;padding:12px}button{background:#287ddf;color:white;border:0;border-radius:8px;font-weight:700}small{color:#9fb0c9}a{color:#68adff}</style></head><body><section><h1>Actualización OTA</h1><p>Selecciona el <code>firmware.bin</code> compilado para <code>full_prototype_cellular</code>.</p><form method="POST" action="/admin/update" enctype="multipart/form-data"><input type="file" name="firmware" accept=".bin,application/octet-stream" required><button type="submit">Actualizar firmware</button></form><p><small>No desconectes la alimentación durante el proceso.</small></p><a href="/">Volver al monitor</a></section></body></html>)HTML";

}  // namespace WebAssets
