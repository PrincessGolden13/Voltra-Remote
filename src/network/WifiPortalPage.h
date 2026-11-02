#pragma once
namespace voltra {
// Self-contained phone page: no CDN, analytics, remote requests or credential
// echo. User-approved open temporary AP; no hotspot password or page PIN.
inline constexpr const char* wifiPortalPage=R"HTML(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>VOLTRA Remote Wi-Fi</title><style>
*{box-sizing:border-box}body{margin:0;background:#080f15;color:#f5f8ff;font:17px system-ui,sans-serif}
main{max-width:460px;margin:24px auto;padding:24px}h1{font-size:28px;margin:6px 0 12px}p{line-height:1.5;color:#b4c2ce}
label{display:block;margin-top:24px;font-weight:600}input,select,button{width:100%;padding:15px;margin-top:8px;border-radius:12px;font:inherit}
input,select{background:#14212c;color:#fff;border:1px solid #536879}select{min-height:54px}
button{background:#00d9e8;color:#062025;border:0;font-weight:700;cursor:pointer}
button:disabled{opacity:.55}#refresh{background:transparent;color:#00d9e8;border:1px solid #536879;padding:10px}
#status{padding:16px;border-radius:12px;background:#152530;min-height:58px;margin-top:24px}
small{display:block;color:#b4c2ce;margin-top:12px;line-height:1.4}[hidden]{display:none!important}</style>
<main><h1>Connect your remote</h1><p>Pick your home Wi-Fi from the nearby 2.4 GHz networks.</p>
<form id="wifi"><input type="hidden" name="token" value="%TOKEN%">
<label for="network">Wi-Fi network</label>
<select id="network" required><option value="">Choose your Wi-Fi</option><option value="manual">Hidden network / enter name…</option></select>
<small id="network-status" role="status">Scanning for networks…</small>
<button id="refresh" type="button">Refresh networks</button>
<div id="manual" hidden><label for="ssid">Hidden network name</label><input id="ssid" maxlength="32" autocomplete="off"></div>
<label for="password">Wi-Fi password</label><input id="password" type="password" maxlength="64" autocomplete="new-password">
<small>Enter this network's password. Leave it empty only for an open network.</small>
<button id="connect">Connect and save</button></form><div id="status" role="status" aria-live="polite">Choose a network to continue.</div>
<p>Stay on the remote's setup Wi-Fi until connected. Your phone may say it has no internet.</p></main>
<script>
const form=document.querySelector('#wifi'),status=document.querySelector('#status'),button=document.querySelector('#connect');
const selector=document.querySelector('#network'),manual=document.querySelector('#manual'),ssid=document.querySelector('#ssid');
const password=document.querySelector('#password'),refresh=document.querySelector('#refresh'),networkStatus=document.querySelector('#network-status');
let submitted=false,accepted=false,finished=false,scanning=false,readingNetworks=false,lastNetworks='';
function controls(){button.disabled=submitted||finished;refresh.disabled=submitted||finished||scanning;selector.disabled=submitted||finished;ssid.disabled=submitted||finished;password.disabled=submitted||finished;}
function selection(){manual.hidden=selector.value!=='manual';ssid.required=!manual.hidden;}
selector.onchange=selection;
async function json(url,options){const r=await fetch(url,options),data=await r.json();if(!r.ok)throw new Error(data.message||'Reopen the setup page.');return data;}
async function networks(){
    if(readingNetworks||finished)return;readingNetworks=true;
    try{
        const data=await json('/networks',{cache:'no-store'});
        if(!Array.isArray(data.networks))throw new Error('Could not read networks. Try Refresh networks.');
        const names=data.networks.filter(name=>typeof name==='string'),signature=JSON.stringify(names);
        if(signature!==lastNetworks){
            const previous=selector.value;selector.replaceChildren(new Option('Choose your Wi-Fi',''));
            for(const name of names)selector.add(new Option(name,'n:'+name));
            if(previous.startsWith('n:')&&!names.includes(previous.slice(2)))selector.add(new Option(previous.slice(2)+' (previous selection)',previous));
            selector.add(new Option('Hidden network / enter name…','manual'));selector.value=previous;selection();lastNetworks=signature;
        }
        scanning=!!data.scanning;
        networkStatus.textContent=scanning?'Scanning for networks…':data.failed?'Scan failed. Try Refresh networks.':names.length?'Select your network above.':'No networks found. Refresh or enter a hidden network name.';
    }catch(error){scanning=false;networkStatus.textContent=error.message||'Reconnect to the remote setup Wi-Fi.';}
    readingNetworks=false;controls();
}
refresh.onclick=async()=>{
    scanning=true;controls();networkStatus.textContent='Scanning for networks…';
    try{await json('/scan',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({token:form.elements.token.value})});await networks();}
    catch(error){scanning=false;networkStatus.textContent=error.message||'Could not refresh networks.';controls();}
};
form.onsubmit=async e=>{
    e.preventDefault();const name=selector.value==='manual'?ssid.value:selector.value.slice(2);
    const body=new URLSearchParams({token:form.elements.token.value,ssid:name,password:password.value});
    submitted=true;accepted=false;controls();status.textContent='Connecting…';
    try{const data=await json('/configure',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});accepted=true;status.textContent=data.message;}
    catch(error){submitted=false;status.textContent=error.message||'Stay connected to the remote setup Wi-Fi and try again.';controls();}
};
async function poll(){
    if(finished||!accepted)return;
    try{
        const data=await json('/status',{cache:'no-store'});status.textContent=data.message||'Waiting for the remote…';
        if(data.connected&&data.saved){finished=true;password.value='';status.textContent='Connected and saved. You can return to your usual Wi-Fi. The setup hotspot will close shortly.';}
        else if(!data.connecting){submitted=false;accepted=false;}
        controls();
    }catch{status.textContent='Waiting for the remote. Stay on its setup Wi-Fi.';}
}
networks();setInterval(networks,2000);setInterval(poll,1000);
</script></html>)HTML";
}
