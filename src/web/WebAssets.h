#pragma once

#include <Arduino.h>

namespace WebAssets {

static const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>VehicleSense Local</title><style>
:root{--bg:#f5f9ff;--panel:#fff;--line:#dce7f4;--text:#10213d;--muted:#65758e;--blue:#0878f9;--cyan:#0aa8b5;--ok:#10a85b;--warn:#ed9700;--bad:#df3d45;--soft:#edf6ff}
*{box-sizing:border-box}body{margin:0;font:15px system-ui,-apple-system,sans-serif;background:radial-gradient(circle at 80% 0,#e7f7ff 0,transparent 35%),var(--bg);color:var(--text);min-height:100vh}
header{background:#fff;border-bottom:1px solid var(--line);padding:18px max(18px,4vw);display:flex;justify-content:space-between;gap:18px;align-items:center;position:sticky;top:0;z-index:2}
.brand{display:flex;align-items:center;gap:12px}.logo{width:42px;height:42px;border-radius:13px;background:linear-gradient(135deg,var(--cyan),var(--blue));color:#fff;display:grid;place-items:center;font-weight:800}.brand b{font-size:21px}.brand b span{color:var(--cyan)}
.identity{font-size:12px;color:var(--muted);margin-top:3px}.admin{padding:10px 14px;border:1px solid var(--line);border-radius:10px;color:var(--text);text-decoration:none;background:#fff;font-weight:650}
main{max-width:1400px;margin:auto;padding:26px max(18px,4vw) 40px}.intro{display:flex;justify-content:space-between;align-items:flex-end;gap:16px;margin-bottom:18px}.intro h1{margin:0;font-size:clamp(23px,4vw,34px)}.intro p{margin:5px 0 0;color:var(--muted)}
.live{border-radius:999px;padding:7px 11px;background:#e9f9f0;color:var(--ok);font-size:12px;font-weight:750}.live.off{background:#fff1f1;color:var(--bad)}.live.warn{background:#fff7e5;color:var(--warn)}
.grid{display:grid;grid-template-columns:repeat(12,1fr);gap:14px}.card{grid-column:span 3;background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:17px;box-shadow:0 8px 28px #274d7510;min-height:118px}.wide{grid-column:span 6}.full{grid-column:1/-1}
.label{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.065em;font-weight:700}.value{font-size:24px;font-weight:780;margin-top:9px;overflow-wrap:anywhere}.detail{font-size:12px;color:var(--muted);margin-top:7px;line-height:1.45}.ok{color:var(--ok)}.warn{color:var(--warn)}.bad{color:var(--bad)}.blue{color:var(--blue)}
.pills{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}.pill{background:var(--soft);color:#36516f;border-radius:999px;padding:6px 9px;font-size:12px}.notice{border-left:4px solid var(--warn);background:#fff9e9;padding:12px 14px;border-radius:10px;color:#735319}
footer{padding:0 max(18px,4vw) 28px;text-align:center;color:var(--muted);font-size:12px}
@media(max-width:980px){.card{grid-column:span 6}}@media(max-width:620px){header{position:static}.brand b{font-size:18px}.admin{padding:8px 10px}.intro{align-items:flex-start;flex-direction:column}.card,.wide{grid-column:1/-1}.value{font-size:21px}}
</style></head><body><header><div class="brand"><div class="logo">VS</div><div><b>Vehicle<span>Sense</span></b><div id="identity" class="identity">Conectando con el dispositivo…</div></div></div><a class="admin" href="/admin">Administrar</a></header>
<main><div class="intro"><div><h1>Monitor local</h1><p>Diagnóstico directo del vehículo, disponible sin Internet.</p></div><div id="live" class="live off">Sin respuesta</div></div>
<div class="grid">
<section class="card"><div class="label">Temperatura</div><div id="temp" class="value">—</div><div id="dhtState" class="detail">Sin lectura</div></section>
<section class="card"><div class="label">Humedad</div><div id="hum" class="value">—</div><div class="detail">DHT11 · porcentaje relativo</div></section>
<section class="card"><div class="label">Iluminación</div><div id="lux" class="value">—</div><div class="detail">BH1750 · lux</div></section>
<section class="card"><div class="label">Presión local</div><div id="pressure" class="value">—</div><div id="alt" class="detail">Altitud barométrica: —</div></section>
<section class="card wide"><div class="label">GPS · texto local</div><div id="gps" class="value">—</div><div id="gpsDetail" class="detail">Sin datos UART</div></section>
<section class="card wide"><div class="label">Conectividad</div><div id="wifi" class="value">—</div><div id="wifiDetail" class="detail">—</div></section>
<section class="card wide"><div class="label">Resumen IMU</div><div id="imu" class="value">—</div><div id="imuDetail" class="detail">Acelerómetro · giroscopio · magnetómetro</div></section>
<section class="card"><div class="label">microSD</div><div id="sd" class="value">—</div><div id="queue" class="detail">Cola offline: —</div></section>
<section class="card"><div class="label">HiveMQ / MQTT</div><div id="mqtt" class="value">—</div><div id="last" class="detail">Último PUBACK: nunca</div></section>
<section class="card"><div class="label">INMP441</div><div id="audio" class="value">—</div><div id="audioDetail" class="detail">Nivel relativo, no dB SPL</div></section>
<section class="card"><div class="label">Uptime</div><div id="uptime" class="value">—</div><div id="time" class="detail">Hora UTC: no sincronizada</div></section>
<section class="card full"><div class="label">Alertas del dispositivo</div><div id="alerts" class="notice">Motor de alertas aún no disponible en este firmware.</div></section>
</div></main><footer>Actualización local cada 2 segundos · sin recursos externos · sin mapa GPS</footer><script>
const el=id=>document.getElementById(id),num=(v,u,d=1)=>Number.isFinite(v)?v.toFixed(d)+' '+u:'—';
const age=(now,then)=>then&&now>=then?Math.floor((now-then)/1000)+' s atrás':'Nunca';
const health=(valid,good,bad='No disponible')=>valid?`<span class="ok">${good}</span>`:`<span class="bad">${bad}</span>`;
async function refresh(){try{const responses=await Promise.all([fetch('/api/telemetry/basic',{cache:'no-store'}),fetch('/api/status',{cache:'no-store'})]);if(!responses[0].ok||!responses[1].ok)throw Error('http');const t=await responses[0].json(),s=await responses[1].json();
el('identity').textContent=`${s.vehicle_id||'vehículo sin ID'} · ${s.device_id||'dispositivo sin ID'} · firmware ${s.firmware_version||'—'}`;el('live').textContent=s.simulated?'Datos simulados de diagnóstico':'Dispositivo local activo';el('live').className=s.simulated?'live warn':'live';
el('temp').textContent=num(t.temperature_c,'°C');el('hum').textContent=num(t.humidity_percent,'%');el('lux').textContent=num(t.light_lux,'lx',0);el('pressure').textContent=num(t.pressure_hpa,'hPa',2);el('alt').textContent='Altitud barométrica: '+num(t.baro_altitude_m,'m',1);el('dhtState').innerHTML=health(t.dht_valid,'Lectura válida','DHT11 inválido');
if(t.gps_valid){el('gps').textContent=`${t.latitude.toFixed(6)}, ${t.longitude.toFixed(6)}`;el('gpsDetail').textContent=`Fix válido · ${t.satellites??'—'} sat · HDOP ${Number.isFinite(t.gps_hdop)?t.gps_hdop.toFixed(1):'—'} · ${num(t.speed_kmh,'km/h',1)}`;}else{el('gps').textContent=t.gps_stream_seen?'NMEA recibida, sin fix':'Sin datos GPS';el('gpsDetail').textContent=t.gps_stream_seen?`${t.satellites??'—'} satélites visibles`:'Revisar alimentación, UART y cielo visible';}
const mags=[num(t.accel_magnitude_mps2,'m/s²',2),num(t.gyro_magnitude_rad_s,'rad/s',2),num(t.mag_magnitude_ut,'µT',1)];el('imu').textContent=t.imu_valid?mags.join(' · '):'Parcial / inválida';
const w=s.wifi||{};el('wifi').innerHTML=health(w.connected,'WiFi conectado',w.ap_running?'Solo Access Point local':'WiFi desconectado');const parts=[];if(w.station_ip)parts.push('STA '+w.station_ip);if(w.ap_ip)parts.push('AP '+w.ap_ip);if(Number.isFinite(w.rssi_dbm))parts.push(w.rssi_dbm+' dBm');if(s.sim800l&&s.sim800l.present)parts.push(s.sim800l.gprs_connected?'GPRS conectado':'SIM800L sin GPRS');el('wifiDetail').textContent=parts.join(' · ')||'Sin dirección de red';
el('sd').innerHTML=health(s.sd.mounted,s.sd.last_write_ok?'Montada · escritura OK':'Montada · esperando','No disponible');el('queue').textContent=`Cola offline: ${s.offline.queued} · replay ${s.offline.replayed} · pérdidas ${s.offline.dropped}`;
el('mqtt').innerHTML=health(s.mqtt.connected,'Conectado',s.mqtt.reconnecting?'Reconectando':'Desconectado');el('last').textContent='Último PUBACK: '+age(s.uptime_ms,s.mqtt.last_puback_ms);
if(t.acoustic_valid){el('audio').textContent=`${t.acoustic_category} · ${num(t.acoustic_relative_level_dbfs,'dBFS',1)}`;el('audioDetail').textContent=`Confianza ${(t.acoustic_confidence*100).toFixed(0)}% · ${t.acoustic_clipping?'clipping detectado':'sin clipping'}`;}else{el('audio').textContent=t.mic_valid?'Análisis no válido':'No disponible';el('audioDetail').textContent='Nivel relativo, no dB SPL';}
el('uptime').textContent=Math.floor(s.uptime_ms/1000)+' s';el('time').textContent='Fuente UTC: '+(s.time_source||'none');el('alerts').textContent=s.alerts_available?'Consulta las alertas activas del dispositivo.':'Motor de alertas aún no disponible en este firmware.';
}catch(e){el('live').textContent='Sin respuesta local';el('live').className='live off';el('identity').textContent='No se pudo consultar la API del ESP32';}}
refresh();setInterval(refresh,2000);
</script></body></html>)HTML";

static const char ADMIN_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Administración VehicleSense</title><style>
body{font:16px system-ui,sans-serif;max-width:680px;margin:50px auto;padding:20px;background:#f4f9ff;color:#10213d}section{background:#fff;padding:28px;border-radius:16px;border:1px solid #dce7f4;box-shadow:0 12px 35px #315c8715}input,button{width:100%;margin-top:14px;padding:12px}button{background:#0878f9;color:white;border:0;border-radius:9px;font-weight:700}small{color:#65758e}a{color:#0878f9}</style></head><body><section><h1>Actualización OTA local</h1><p>Selecciona el <code>firmware.bin</code> compilado para el mismo environment instalado en el dispositivo. Para VehicleSense usa <code>vehiclesense_wifi</code>.</p><form method="POST" action="/admin/update" enctype="multipart/form-data"><input type="file" name="firmware" accept=".bin,application/octet-stream" required><button type="submit">Actualizar firmware</button></form><p><small>No desconectes la alimentación durante el proceso. USB permanece como vía de recuperación.</small></p><a href="/">Volver al monitor</a></section></body></html>)HTML";

}  // namespace WebAssets
