#pragma once
#include <Arduino.h>

const char kNetworkPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="es"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bricohams - Red CW</title>
<style>body{font:16px system-ui;background:#101725;color:#eef4ff;max-width:760px;margin:auto;padding:20px}
h1{color:#ff9933}button,input,textarea{font:inherit;padding:12px;box-sizing:border-box}
textarea{width:100%;height:100px}button{background:#ff9933;border:0;border-radius:6px;margin:8px 0}
article{background:#213047;margin:10px 0;padding:14px;overflow-wrap:anywhere}pre{white-space:pre-wrap}
.error{color:#ffb5a5}label{display:block;margin:12px 0}</style>
<h1>Bricohams · Red CW</h1>
<p id="network"></p><p>Red local de práctica por WiFi/ESP-NOW, no radioafición por RF ni Internet.
Todos los tutores con este firmware, en el canal 1 y dentro del alcance directo, pueden recibir.
La difusión no garantiza entrega ni confidencialidad.</p>
<form id="send"><label>Indicativo <input id="call" value="NOCALL" maxlength="9" required pattern="[A-Za-z0-9\/]{1,9}"></label>
<label>Mensaje (hasta 120 caracteres Morse/ASCII)
<textarea id="text" maxlength="120" required placeholder="CQ CQ DE NOCALL K"></textarea></label>
<button>Enviar a todos</button></form>
<label><input id="decode" type="checkbox"> Mostrar texto decodificado automáticamente (también en pantalla)</label>
<p id="status" role="status"></p><div id="messages"></div>
<script>
const token='@TOKEN@';let updating=false,lastError='',lastMessages='';
const status=document.getElementById('status');
async function post(path,body){const r=await fetch(path,{method:'POST',headers:{'Content-Type':'text/plain','X-Bricohams-Token':token},body});
const text=await r.text();if(!r.ok)throw Error(text);return text;}
document.getElementById('send').onsubmit=async e=>{e.preventDefault();try{
status.textContent=await post('/api/send',document.getElementById('call').value+'\n'+document.getElementById('text').value);
status.className='';document.getElementById('text').value='';await refresh();
}catch(error){status.textContent=error.message;status.className='error';}};
document.getElementById('decode').onchange=async e=>{try{await post('/api/decode',e.target.checked?'1':'0');
status.textContent='Modo RX guardado';status.className='';lastMessages='';await refresh();}
catch(error){status.textContent=error.message;status.className='error';await refresh();}};
async function refresh(){if(updating)return;updating=true;try{
const r=await fetch('/api/state',{cache:'no-store'});if(!r.ok)throw Error('No se puede leer el estado de la red');
const data=await r.json();document.getElementById('network').textContent=data.ssid+' · Pendientes: '+data.pending+
' · Descartes: '+data.drops+' · Paquetes inválidos: '+data.invalid+' · Fallos TX: '+data.sendFailures;
document.getElementById('decode').checked=data.decode;
const serialized=JSON.stringify(data.messages);if(serialized!==lastMessages){lastMessages=serialized;
const list=document.getElementById('messages');list.replaceChildren();
for(const m of [...data.messages].reverse()){const card=document.createElement('article');
const heading=document.createElement('strong');heading.textContent=(m.tx?'TX ':'RX ')+m.from;card.append(heading);
const code=document.createElement('pre');code.textContent=m.morse;card.append(code);
const text=document.createElement('p');text.textContent=m.text??'Texto oculto para practicar. Activa la decodificación para revelarlo.';card.append(text);
list.append(card);}}if(lastError){status.textContent='Conexión recuperada';status.className='';}lastError='';
}catch(error){if(lastError!==error.message){status.textContent=error.message;status.className='error';lastError=error.message;}}
finally{updating=false;}}
refresh();setInterval(refresh,1500);
</script></html>)HTML";
