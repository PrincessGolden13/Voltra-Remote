#pragma once
namespace voltra {
// Self-contained phone page: no CDN, analytics, remote requests or credential
// echo. Submitted Wi-Fi passwords only go to the remote over its protected AP.
inline constexpr const char* wifiPortalPage=R"HTML(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>VOLTRA Remote Wi-Fi</title><style>
*{box-sizing:border-box}body{margin:0;background:#080f15;color:#f5f8ff;font:17px system-ui,sans-serif}
main{max-width:460px;margin:36px auto;padding:24px}h1{font-size:28px;margin:6px 0 12px}p{line-height:1.5;color:#b4c2ce}
label{display:block;margin-top:24px;font-weight:600}input,button{width:100%;padding:15px;margin-top:8px;border-radius:12px;font:inherit}
input{background:#14212c;color:#fff;border:1px solid #536879}button{background:#00d9e8;color:#062025;border:0;font-weight:700;cursor:pointer}
button:disabled{opacity:.55}#status{padding:16px;border-radius:12px;background:#152530;min-height:58px;margin-top:24px}
small{display:block;color:#b4c2ce;margin-top:16px;line-height:1.4}</style>
<main><h1>Connect your remote</h1><p>Choose your home Wi-Fi or enter its name. Use a 2.4 GHz network.</p>
<form id="wifi"><input type="hidden" name="token" value="%TOKEN%">
<label for="ssid">Wi-Fi name</label><input id="ssid" name="ssid" list="networks" maxlength="32" autocomplete="off" required><datalist id="networks"></datalist>
<label for="password">Wi-Fi password</label><input id="password" name="password" type="password" maxlength="64" autocomplete="new-password">
<small>Leave the password empty only for an open network. Your password stays on the remote.</small>
<button id="connect">Connect and save</button></form><div id="status" role="status" aria-live="polite">Ready to connect.</div>
<p>Keep this page open while connecting. Your phone may say this setup network has no internet.</p></main>
<script>
const form=document.querySelector('#wifi'),status=document.querySelector('#status'),button=document.querySelector('#connect');let submitted=false,finished=false;
async function networks(){try{const r=await fetch('/networks',{cache:'no-store'}),data=await r.json();const list=document.querySelector('#networks');list.replaceChildren();for(const name of data){const option=document.createElement('option');option.value=name;list.append(option);}}catch{}}
form.onsubmit=async e=>{e.preventDefault();button.disabled=true;submitted=true;status.textContent='Connecting…';try{const r=await fetch('/configure',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(new FormData(form))});const data=await r.json();status.textContent=data.message;if(!r.ok){submitted=false;button.disabled=false;}}catch{status.textContent='Waiting for the remote. Stay connected to its setup Wi-Fi.';}};
async function poll(){if(finished)return;try{const r=await fetch('/status',{cache:'no-store'}),data=await r.json();if(submitted){status.textContent=data.message;if(data.connected&&data.saved){finished=true;form.password.value='';status.textContent='Connected and saved. You can return to your usual Wi-Fi. The setup hotspot will close shortly.';}else if(!data.connecting){button.disabled=false;submitted=false;}}}catch{} }
networks();setTimeout(networks,4000);setInterval(poll,1000);
</script></html>)HTML";
}
