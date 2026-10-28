#pragma once
#include <Arduino.h>

const char kNetworkPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="es"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bricohams - CW</title>
<style>body{font:16px system-ui;background:#101725;color:#eef4ff;max-width:760px;margin:auto;padding:20px}
h1,h2{color:#ff9933}button,input,textarea,select{font:inherit;padding:10px;box-sizing:border-box;max-width:100%}
textarea{width:100%;height:100px}button{background:#ff9933;border:0;border-radius:6px;margin:8px 0}
article,details{background:#213047;margin:10px 0;padding:14px;overflow-wrap:anywhere}pre{white-space:pre-wrap}
.error{color:#ffb5a5}label{display:block;margin:12px 0}table{width:100%;text-align:left}summary{cursor:pointer}</style>
<h1>Bricohams · <span data-tr="Network"></span></h1>
<label><span data-tr="UiLanguage"></span>
<select id="language"><option value="0">Español</option><option value="1">English</option><option value="2">Deutsch</option><option value="3">Français</option><option value="4">Italiano</option></select></label>
<p id="network"></p><p data-tr="WebIntro"></p><p data-tr="MorseNote"></p>
<form id="send"><label><span data-tr="Callsign"></span>
<input id="call" value="NOCALL" maxlength="9" required pattern="[A-Za-z0-9\/]{1,9}"></label>
<label><span data-tr="Message"></span><textarea id="text" maxlength="120" required placeholder="CQ CQ DE NOCALL K"></textarea></label>
<button data-tr="SendAll"></button></form>
<label><input id="decode" type="checkbox"> <span data-tr="DecodeLabel"></span></label>
<p id="status" role="status" aria-live="polite"></p>
<details><summary data-tr="Config"></summary>
<form id="trainer">
<label><span data-tr="WordLanguage"></span><select id="vocabulary">
<option value="0">Español</option><option value="1">English</option><option value="2">Deutsch</option><option value="3">Français</option><option value="4">Italiano</option></select></label>
<label><span data-tr="Sequence"></span><select id="sequence">
<option value="0" data-tr="SequenceBrico"></option><option value="1" data-tr="SequenceLetters"></option>
<option value="2" data-tr="SequenceDigits"></option><option value="3" data-tr="CustomSet"></option></select></label>
<label><span data-tr="CustomSet"></span><input id="custom" minlength="2" maxlength="40" required></label>
<label><span data-tr="WordGap"></span><select id="wordGap"><option>1</option><option>2</option><option>3</option><option>4</option><option>5</option></select> × 7</label>
<button data-tr="Save"></button></form></details>
<details><summary data-tr="Memories"></summary>
<form id="memory"><label><span data-tr="Memories"></span><select id="memoryIndex">
<option value="0">1</option><option value="1">2</option><option value="2">3</option><option value="3">4</option>
<option value="4">5</option><option value="5">6</option><option value="6">7</option><option value="7">8</option></select></label>
<label><span data-tr="Message"></span><textarea id="memoryText" maxlength="120"></textarea></label>
<button data-tr="Save"></button></form></details>
<details><summary data-tr="Statistics"></summary><p id="totals"></p>
<table><thead><tr><th>CW</th><th data-tr="Attempts"></th><th data-tr="Errors"></th></tr></thead><tbody id="scores"></tbody></table></details>
<div id="messages"></div>
<script>
const token='@TOKEN@',catalog=@CATALOG@;
let language=@LANGUAGE@,updating=false,lastError='',lastMessages='',lastScores='',state=null,trainerDirty=false,memoryDirty=false;
const el=id=>document.getElementById(id),tr=id=>catalog[id][language];
function localize(){
document.documentElement.lang=['es','en','de','fr','it'][language];
document.querySelectorAll('[data-tr]').forEach(node=>node.textContent=tr(node.dataset.tr));
document.title='Bricohams - '+tr('Network');el('language').value=language;lastMessages='';lastScores='';
}
function status(text,error=false){el('status').textContent=text;el('status').className=error?'error':'';}
async function post(path,body,form=false){
const r=await fetch(path,{method:'POST',headers:{'Content-Type':form?'application/x-www-form-urlencoded':'text/plain','X-Bricohams-Token':token},body});
const text=await r.text();if(!r.ok)throw Error(text);return text;
}
el('language').onchange=async e=>{const previous=language;language=Number(e.target.value);localize();
try{await post('/api/language',String(language));status(tr('Saved'));await refresh();}
catch(error){language=previous;localize();status(error.message,true);}};
el('send').onsubmit=async e=>{e.preventDefault();try{
status(await post('/api/send',el('call').value+'\n'+el('text').value));el('text').value='';await refresh();
}catch(error){status(error.message,true);}};
el('decode').onchange=async e=>{try{await post('/api/decode',e.target.checked?'1':'0');
status(tr('DecodeSaved'));lastMessages='';await refresh();
}catch(error){status(error.message,true);await refresh();}};
el('trainer').oninput=()=>{trainerDirty=true;};
el('trainer').onsubmit=async e=>{e.preventDefault();try{
const body=new URLSearchParams();for(const id of ['vocabulary','sequence','wordGap','custom'])body.set(id,el(id).value);
status(await post('/api/trainer',body.toString(),true));trainerDirty=false;await refresh();
}catch(error){status(error.message,true);}};
el('memoryText').oninput=()=>{memoryDirty=true;};
el('memoryIndex').onchange=()=>{el('memoryText').value=state?.memories[Number(el('memoryIndex').value)]??'';memoryDirty=false;};
el('memory').onsubmit=async e=>{e.preventDefault();try{
status(await post('/api/memory',el('memoryIndex').value+'\n'+el('memoryText').value));memoryDirty=false;await refresh();
}catch(error){status(error.message,true);}};
function render(data){
if(language!==data.language){language=data.language;localize();}
el('network').textContent=data.ssid+' · '+tr('Pending')+': '+data.pending+' · '+tr('Drops')+': '+data.drops+
' · '+tr('InvalidPackets')+': '+data.invalid+' · '+tr('TxFailures')+': '+data.sendFailures;
el('decode').checked=data.decode;
if(!trainerDirty)for(const id of ['vocabulary','sequence','wordGap','custom'])el(id).value=data[id];
if(!memoryDirty)el('memoryText').value=data.memories[Number(el('memoryIndex').value)];
el('totals').textContent=tr('Attempts')+': '+data.groups+' · '+tr('Accuracy')+': '+(data.groups?Math.floor(data.correct*100/data.groups):0)+'%';
const scores=JSON.stringify(data.scores);
if(scores!==lastScores){lastScores=scores;el('scores').replaceChildren();
for(const row of data.scores){if(!row[1])continue;const line=document.createElement('tr');
for(const value of row){const cell=document.createElement('td');cell.textContent=value;line.append(cell);}el('scores').append(line);}}
const serialized=JSON.stringify(data.messages);
if(serialized!==lastMessages){lastMessages=serialized;el('messages').replaceChildren();
for(const m of [...data.messages].reverse()){
const card=document.createElement('article'),heading=document.createElement('strong'),code=document.createElement('pre'),text=document.createElement('p');
heading.textContent=(m.tx?'TX ':'RX ')+m.from;code.textContent=m.morse;text.textContent=m.text??tr('Hidden');
card.append(heading,code,text);el('messages').append(card);}}
}
async function refresh(){if(updating)return;updating=true;try{
const r=await fetch('/api/state',{cache:'no-store'});if(!r.ok)throw Error(tr('Offline'));
state=await r.json();render(state);if(lastError)status(tr('Recovered'));lastError='';
}catch(error){if(lastError!==error.message){status(error.message,true);lastError=error.message;}}
finally{updating=false;}}
localize();refresh();setInterval(refresh,1500);
</script></html>)HTML";
