function compressFunc(){return!1}var MANIFEST=chrome.runtime.getManifest(),isDebug=(compressFunc+"").indexOf("false")!==-1,bv=window.external.LiebaoGetVersion&&window.external.LiebaoGetVersion()||/Chrome\/([\d\.]+)/i.exec(navigator.userAgent)[1],entry12306=!1,log=function(){var n=function(){return this.print=function(){isDebug&&console.log.apply(console,$.makeArray(arguments))},this};return new n}(),notification=function(){var n=function(){var n=this,t={};this.mapParam=function(n){return{type:n.type||"basic",title:n.title||"提示",message:n.message||"",iconUrl:n.iconUrl||"/icons/icon_n.png",priority:n.priority||0,eventTime:n.eventTime||null,buttons:_.map(n.buttons,function(n){return{title:n.title,iconUrl:n.iconUrl||"/icons/icon_16.png"}})||null,imageUrl:n.imageUrl||null,items:_.map(n.items,function(n){return{title:n.title,message:n.message}})||null}};n.create=function(i){var r=n.mapParam(i),u=i.id||"";chrome.notifications.create(u,r,function(n){t[n]=i;i.id=n});i.update=function(){n.update(i)};i.clear=function(){n.clear(i.id)}};n.update=function(t){var i=n.mapParam(t);chrome.notifications.update(t.id,i,function(){t.onUpdate&&t.onUpdate.apply(t,$.makeArray(arguments))})};n.clear=function(n){chrome.notifications.clear(n,function(){opts.onClear&&opts.onClear.apply(opts,$.makeArray(arguments))})};n.getAll=function(n){chrome.notifications.getAll(n)};n.getPermissionLevel=function(n){chrome.notifications.getPermissionLevel(n)};chrome.notifications.onClicked.addListener(function(n){var i=t[n];i!==undefined&&i!==null&&$.isFunction(i.onClick)&&i.onClick.apply(this)});chrome.notifications.onClosed.addListener(function(n,i){var r=t[n];r!==undefined&&r!==null&&$.isFunction(r.onClose)&&r.onClose.apply(this,[i])});chrome.notifications.onButtonClicked.addListener(function(n,i){var r=t[n],u;r!==undefined&&r!==null&&(u=r.buttons&&r.buttons[i],u&&$.isFunction(u.onClick))&&u.onClick.apply(r)});chrome.runtime.onMessage.addListener(function(t,i){t.action==="showNotification"?n.create({message:t.detail.message,title:t.detail.title,iconUrl:t.detail.iconUrl}):t.action==="notify"?n.create(t.detail):t.action==="attention"&&i.tab&&(chrome.tabs.update(t.detail&&t.detail.tabid||i.tab.id,{active:!0,highlighted:!0}),chrome.windows.update(t.detail&&t.detail.windowid,{drawAttention:!0,focused:!0,state:"maximized"}))});chrome.runtime.onMessage.addListener(function(n,t){n&&n.action&&n.action==="attention"&&t.tab&&(chrome.tabs.update(n.detail&&n.detail.tabid||t.tab.id,{active:!0,highlighted:!0}),chrome.windows.update(n.detail&&n.detail.windowid,{drawAttention:!0,focused:!0,state:"maximized"}))})};return new n}(),CFG_MANGER=function(){function i(){var n=this,u=!1,i=null,f=12e5,t=null,r;return this.baseConfig={appendCacheTagHeader:!0,showRealSeatCount:!0,autoSubmitAfterVerifyCode:!0,autoLoginAfterVerifyCode:!0,rememberLoginUser:!0,rememberLoginPwd:!0,enableSoundPrompt:!0,enablePopupPrompt:!0,musicUrl:"http://static.liebao.cn/resources/audio/music2.ogg",submitOrderDelay:5,refreshDelay:5,autoWaitToSell:!0,keepOnline:!0,blockDynamicJs:!1,enableRealTimeTicketQuery:!0,showTicketPrice:!0,otnAutoConfirmOrderDelay:0,dynamicAutoConfirmOrderDelay:1e3,dynamicAutoSubmitOrderDelay:5e3,otnAutoSubmitOrderDelay:0,enableSelfTrack:!0,enableServerAutoChange:!0,enableAutoCaptcha:!0&&typeof window.external.LieBaoSign=="function",autoCaptchaFailedLimit:5,captchaServerUrl:"http://api.12306.liebao.cn/code.php",appendPriceUnit:!0,blockVcVerify:!1,blockQueueCount:!1,chatServerApi:"http://srv.12306.liebao.cn/room/list"},this.sysConfig={},this.userConfig={enableAutoCaptcha:!1},localStorage.sysConfig&&(this.sysConfig=$.extend(this.sysConfig,JSON.parse(localStorage.sysConfig))),localStorage.userConfig&&(this.userConfig=JSON.parse(localStorage.userConfig)),this.refresh=function(){n.config=$.extend({},n.baseConfig,n.userConfig,n.sysConfig)},this.refresh(),this.refresh=function(){t&&(clearTimeout(t),t=null);(entry12306||!u)&&(i=new Date,$.getJSON("http://storage.fishlee.net/soft/files/44/rwticketconfig.json?"+Math.random(),function(t){(typeof t.enableAutoCaptcha=="undefined"||t.enableAutoCaptcha)&&(t.enableAutoCaptcha=t.enableAutoCaptcha&&typeof window.external.LieBaoSign=="function");localStorage.sysConfig=JSON.stringify(t);n.sysConfig=t;n.config=$.extend({},n.baseConfig,n.userConfig,n.sysConfig);chrome.runtime.sendMessage({action:"sysConfigUpdate",detail:n.config})}));var r=(new Date).getMinutes();t=r>=50||r<=15?setTimeout(n.refresh,6e5):setTimeout(n.refresh,f)},this.isEnabled=function(){},r=function(t){t&&t.action==="enter12306"&&(f=12e5,(i===null||new Date-i>=12e5)&&n.refresh());t&&t.action==="setUserConfig"&&(n.userConfig=$.extend({},n.userConfig,t.detail),n.config=$.extend({},n.baseConfig,n.userConfig,n.sysConfig),localStorage.userConfig=JSON.stringify(n.userConfig),chrome.runtime.sendMessage({action:"sysConfigUpdate",detail:n.config}))},chrome.runtime.onMessage.addListener(r),chrome.runtime.onMessageExternal.addListener(r),this.refresh(),u=!0,this}var n=new i,t=function(t,i,r){t.action==="getSysConfig"&&r({action:"responseSysConfig",detail:n.config});t.action==="getBaseSysConfig"&&r({action:"responseBaseSysConfig",detail:$.extend(n.baseConfig,n.sysConfig)})};return chrome.runtime.onMessage.addListener(t),chrome.runtime.onMessageExternal.addListener(t),n}();(function(){window.localStorage.cv!=MANIFEST.version&&(window.localStorage.cv=MANIFEST.version,notification.create({title:"更新成功",message:"您的订票助手已成功更新至 "+MANIFEST.version+" :-)"}))})(),function(){function t(n){var t={};return n.split("; ").forEach(function(n){var i=n.indexOf("=");t[n.substr(0,i)]=n.substr(i+1)}),t}function i(n){for(var a,o,h=n.url,c=!CFG_MANGER.config.blockDynamicJs,i={},e=null,l=null,f=0;f<n.requestHeaders.length;++f){var s=n.requestHeaders[f],r=s.name,u=s.value;r==="User-Agent"?i[r]="Mozilla/5.0 (MSIE 9.0; Windows NT 6.1; Trident/5.0;)":s.name=="TRefer"?i.Referer=u:s.name=="Referer"?i.Referer||u.indexOf("12306.cn")==-1||(i.Referer=u):r==="Cookie"?(l=u,i.Cookie=u):r.indexOf("Fish-")!==-1?(r=r.substr(5),r==="Cookie"?e=t(u):u?i[r]=u:i[r]&&delete i[r],c=!0):i[r]||(i[r]=u)}e&&(a=t(l),e=_.extend({},a,e),i.Cookie=_.filter(_.map(e,function(n,t){return n?t+"="+encodeURIComponent(n):""}),function(n){return n||!1}).join("; "),i.Cookie||delete i.Cookie);o=[];for(f in i)o.push({name:f,value:i[f]});return!c&&$.isFunction(window.cbl)&&window.cbl.call(n,h,i)?{cancel:!0}:(CFG_MANGER.config.appendCacheTagHeader&&h.indexOf("querySingleAction.do?method=queryLeftTicket")!=-1&&(o.push({name:"If-None-Match",value:Math.random()+""}),o.push({name:"Pragma",value:"No-cache"})),{requestHeaders:o})}var n={urls:["*://*.12306.cn/*"],types:["main_frame","sub_frame","stylesheet","script","image","object","xmlhttprequest","other"]},r=function(n){var e=/trainClass=([^&]+)&/g.exec(n)[1],u=e.replace(/%23/g,"#").split("#"),t,o,r,f,s,i,h;if(u[0]=="QB")return null;t=[0,0,0,0,0];for(o in u)r=u[o],r=="D"?t[0]=1:r=="Z"?t[1]=1:r=="T"?t[2]=1:r=="K"?t[3]=1:r=="QT"&&(t[4]=1);for(f=[],s=["D","Z","T","K","QT"],i=0;i<t.length;i++)t[i]||(t[i]=Math.random()>.5?1:0),t[i]&&f.push(s[i]);return h=f.join("#")+"#",n.replace(e,escape(h))};chrome.webRequest.onBeforeSendHeaders.addListener(i,n,["blocking","requestHeaders"]);chrome.webRequest.onBeforeRequest.addListener(function(){return null},n,["blocking"])}(),function(){var r=window.external.LieBaoLookupDnsAddress?1:0,e=typeof chrome.tabs.httpRequest=="function";if(chrome.runtime.onMessage.addListener(function(n,t,i){n&&n.action==="servervalid"&&i({valid:r})}),chrome.runtime.onMessageExternal.addListener(function(n,t,i){n&&n.action==="servervalid"&&i({valid:r})}),r){var p="http://www.fishlee.net/apps/cn12306/ipservice/getlist",w="http://www.fishlee.net/apps/cn12306/ipservice/update2",u=!1,n={count:0,valid:0,timeout:0,failed:0,lastUpdate:null,validList:[],status:0,enableDirectAccess:e},i=[],f=[],s={"kyfw.12306.cn":{ip:null,speed:null}},b=[{host:"kyfw.12306.cn",url:"https://kyfw.12306.cn/otn/",count:0}],t={};(function(){var t=function(t,r,u){t&&t.action&&(t.action==="getServerStatus"?u(n):t.action==="getServerList"?u(n.status==2?_.flatten([f,i]):[]):t.action==="getCurrentServer"&&u(s))};chrome.runtime.onMessage.addListener(t);chrome.runtime.onMessageExternal.addListener(t)})();var k=function(n,t){if(!n||n.length<=t)return n;for(var i=[],r;i.length<t;)r=Math.floor(Math.random()*n.length),i.push(n.splice(r,1));return i},o=function(){n.status=1;chrome.runtime.sendMessage({action:"serverStateChange",detail:{state:n.status}});chrome.runtime.sendMessage({action:"track",detail:{type:90}});$.getJSON(p).done(function(r){r=_.groupBy(r,function(n){return n.host});for(var u in r)r[u]=k(r[u],100);r=_.flatten(r);i=r;n.count=r.length;n.valid=n.timeout=n.failed=0;t&&_.each(t,function(n,t){_.each(n,function(n){i.push({host:t,ip:n})})});d()}).fail(function(){setTimeout(o,6e5)})},d=function(){f=[];n.status=2;chrome.runtime.sendMessage({action:"serverStateChange",detail:{state:n.status}});h()},h=function(){i.length?g(i.splice(0,1),h):nt()},c=function(n,t){var i=new $.Deferred;return chrome.tabs.httpRequest(n,"User-Agent: MSIE 9.0\r\nHost: "+t,3e3,function(n){var t=null;try{t=JSON.parse(n)}catch(e){}if(!t){i.reject(0,"unknown error.");return}var f=t[0],r=t[1],u=t[2];f?i.resolve(u,r):r===10001?i.reject(0,"timeout"):i.reject(r,u)}),i.promise()},g=function(t,i){t=t[0];t.host=t.host||"kyfw.12306.cn";var r="https://"+t.ip+"/otn/",e=new Date;c(r,t.host).done(function(n){t.speed=n&&n.indexOf("客运服务")===-1?-2:new Date-e}).fail(function(){t.speed=-1}).always(function(){t.status=t.speed==-1||t.speed==-2?-1:t.speed>1e3?-2:1;u&&(t.status===-1?n.failed++:t.status===-2?n.timeout++:t.status===0?n.timeout++:(n.valid++,n.validList.push(t)));f.push(t);setTimeout(function(){chrome.runtime.sendMessage({action:"serverTestResult",detail:t});i()},100)})},nt=function(){var t,i,r,e;if(n.status=3,chrome.runtime.sendMessage({action:"serverStateChange",detail:{state:n.status}}),n.validList=_.groupBy(n.validList,function(n){return n.host}),_.each(n.validList,function(n,t,i){n.sort(function(n,t){return n.speed===t.speed?0:n.speed===-1&&t.speed!==-1?1:n.speed!==-1&&t.speed===-1?-1:n.speed===0&&t.speed!==0?1:n.speed!==0&&t.speed===0?-1:n.speed-t.speed});i[t]=n.splice(0,20)}),n.status=0,n.lastUpdate=new Date,chrome.runtime.sendMessage({action:"serverStateChange",detail:{state:n.status}}),u){t=[];_.each(f,function(n){var i=_.pick(n,"ip","averageSpeed","host");i.averageSpeed=n.speed;t.push(i)});i=_.groupBy(t,function(n){return n.host});r={};for(e in i)r[e]=JSON.stringify(i[e]);chrome.runtime.sendMessage({action:"track",detail:{type:91}});$.post(w,r);localStorage.serverStorage=JSON.stringify(n)}else{if(_.flatten(n.validList).length<5){u=!0;n.validList=[];o();return}localStorage.serverStorage=JSON.stringify(n)}l()},tt=function(){_.each(s,function(n){n.ip&&window.external.LieBaoSetHostAddress(n.host,n.ip,1)})};setInterval(tt,2e4);var ut=function(n,i){chrome.runtime.sendMessage({action:"track",detail:{type:92,values:[i]}});i?window.external.LieBaoSetHostAddress(n,i,1):window.external.LieBaoSetHostAddress(n,t[n][0],0)},l=function(){return},ft=function(n,t){var i=new Date;$.ajax(n,{dataType:"text",method:"GET",timeout:3e3}).done(function(){t(new Date-i)}).fail(function(){t(999999)})},it=function(){localStorage.serverStorage&&(n=JSON.parse(localStorage.serverStorage),n.enableDirectAccess=e);!n.lastUpdate||(new Date).getTime()/36e5-new Date(n.lastUpdate).getTime()/36e5>=12||n.validList.length<5?(u=!0,n.validList=[],o()):(i=n.validList,l())},rt=function(){var n=0;if(_.each(b,function(i){t[i.host]=_.filter(window.external.LieBaoLookupDnsAddress(i.host,"false").split(";"),function(n){return n||!1});t[i.host]&&t[i.host].length||n++}),n){r=-1;return}c("https://"+t["kyfw.12306.cn"][0]+"/otn/","kyfw.12306.cn").done(function(){it()}).fail(function(){r=-1})},a=!1,v=function(){localStorage.lastUsed=Math.floor((new Date).getTime()/864e5);a||(a=!0,rt());entry12306=!0},y=function(n){try{return JSON.parse(n)}catch(t){return isDebug&&console.log("WARNING: unable to parse json response. content -> "+n),null}};chrome.runtime.onMessage.addListener(function(t,i,r){var l,h,a,s;if(t&&t.action)if(t.action==="triggerUpdate")v();else if(t.action==="directAccess"){var u=t.data,o=/^https?:\/\/([^\/]+)\//.exec(u.url)&&RegExp.$1,f=null,c=0;if(e&&o&&n.validList&&n.validList[o]&&n.validList[o].length&&(l=n.validList[o],c=Math.floor(Math.random()*l.length),f=l[c],u.url=u.url.replace("//"+o,"//"+f.ip)),f){if(h=[],u.headers)for(a in u.headers)s=a,s.indexOf("Fish-")===0&&(s=s.substr(5)),h.push(s+": "+u.headers[a]);h.push("Host: "+o);u.data&&(u.url+="?"+$.param(u.data));isDebug&&console.log("direct access to url "+u.url);chrome.tabs.httpRequest(u.url,h.join("\r\n"),2e4,function(t){isDebug&&console.log("direct access response received. data: ",t),function(t,i,u,e){t&&i===200?r([!0,i,i,u,e||"",y(u)]):(r([!1,i,i,u,e||"",null]),f.failedCount=(f.failedCount||0)+1,console.log("bad response for host ["+f.ip+"], increase failed counter. current at "+f.failedCount),f.failedCount>3&&(n.validList[o].splice(c,1),console.log("bad response for host ["+f.ip+"] failed too many times, removed from validlist."),localStorage.serverStorage=JSON.stringify(n)))}.apply(this,y(t))})}else $.ajax(u).done(function(n,t,i){r([!0,i.status,i.statusText,i.responseText,i.getAllResponseHeaders(),n])}).fail(function(n){r([!1,n.status,n.statusText,n.responseText,"",null])});return!0}});chrome.runtime.onMessageExternal.addListener(function(n){n&&n.action==="triggerUpdate"&&v()});setInterval(function(){entry12306=!1;chrome.tabs.query({url:"*://*.12306.cn/*"},function(n){entry12306|=n&&n.length>0});chrome.tabs.query({url:"*://12306.liebao.cn/*"},function(n){entry12306|=n&&n.length>0})},2e4)}}(),function(){var t="http://liebao.tjweb.ijinshan.com/click/__infoc.gif?actionname=liebao_80",i="http://service.fishlee.net/report/44/",n=function(n,r){var f,u;r=r||[];CFG_MANGER.config.enableSelfTrack&&$.post(i,{t:n,d:r.join(":")});f={type:n};for(u in r)r[u]&&(f["value"+(parseInt(u)+1)]=r[u]+"");$.get(t,f)};chrome.runtime.onMessage.addListener(function(t){t&&t.action&&t.action==="track"&&n(t.detail.type,t.detail.values)});chrome.runtime.onMessageExternal.addListener(function(t){t&&t.action&&t.action==="track"&&n(t.detail.type,t.detail.values)});n(99,[MANIFEST.version,bv])}(),function(){var u=parseInt(/Chrome\/(\d+)/i.exec(navigator.userAgent)[1]),f="http://www.fishlee.net/service/update2/44/"+(u<34?"44":"60")+"/version_v6.json?"+Math.random(),t=null,e=!1,o=null,s=null,n=null,h=function(n,t){for(var f,e,i=n.split("."),r=t.split("."),o=Math.min(i.length,r.length),u=0;u<o;u++){if(f=parseInt(i[u]),e=parseInt(r[u]),f<e)return-1;if(f>e)return 1}return i.length>r.length?1:i.length<r.length?-1:0},i=function(){$.getJSON(f,function(t){n=t;o=t.version;s=t.notify;n.hasUpdate=e=h(MANIFEST.version,t.version)<0;chrome.runtime.sendMessage({action:"updateInfoRefreshed",detail:n})});t=new Date},r=function(r,u,f){r&&r.action&&(r.action==="getUpdateInfo"?f(n):r.action==="getVersionInfo"?f({curVersion:MANIFEST.version,updateInfo:n}):r.action==="triggerUpdate"?(!t||new Date-t>=12e5)&&i():r.action==="getBv"&&f({bv:bv}))};chrome.runtime.onMessage.addListener(r);chrome.runtime.onMessageExternal.addListener(r);i()}(),function(){var n=function(n,t,i){if(n&&n.action==="captcha"&&CFG_MANGER.config.captchaServerUrl){var r;return $.ajax(CFG_MANGER.config.captchaServerUrl,{method:"POST",data:{pic:n.detail.base64,sign:window.external.LieBaoSign(n.detail.base64)},timeout:3e3}).done(function(n){r=n.success?n.code.replace(/\s/g,""):""}).fail(function(){r=""}).always(function(){i({code:r})}),!0}};chrome.runtime.onMessage.addListener(n);chrome.runtime.onMessageExternal.addListener(n)}(),function(){var n=function(n,t,i){if(n.action==="getStorage")i({action:"sendStorage",detail:localStorage});else if(n.action==="setStorage"&&n.detail)_.each(n.detail,function(n,t){n?localStorage.setItem(t,n):localStorage.removeItem(t)}),i({action:"sendStorage",detail:n.detail});else if(n.action==="notify"){var r=new Notification(n.title||"订票助手",{body:n.content||null,icon:"/icons/icon_n.png"});setTimeout(function(){r.close()},5e3)}};chrome.runtime.onMessageExternal.addListener(n)}(),function(){var n=[],i=[],t=[],r=null,o=function(){f.disconnect();t=[];i=[];r=null},s=function(n,r){if(n&&n.action)switch(n.action){case"getChatServerStatus":e.loadServers(function(n){r.postMessage({action:"responseServer",detail:n})});break;case"getCurrentRoomInfo":r.postMessage({action:"responseCurrentRoomInfo",detail:f.currentRoom});break;case"enterChatRoom":f.connect(n.detail);break;case"chatRoomSendMsg":f.sendMsg(n.detail);break;case"disconnectChatRoom":t=[];i=[];f.disconnect()}},h=function(t){var i=_.indexOf(n,t);i>-1&&n.splice(i,1);r||n.length!==0||(r=setTimeout(o,18e5))},u=function(r){if(r&&r.action==="chatRoomReceiveMsg"){if(!n.length){i.push(r);return}t.push(r);t.length>20&&t.splice(0,1)}n.forEach(function(n){n.postMessage(r)});r&&r.action==="chatRoomConnected"&&(t.length&&t.forEach(function(t){n.forEach(function(n){n.postMessage(t)})}),i.length&&(i.forEach(function(t){n.forEach(function(n){n.postMessage(t)})}),i=[]))},e,f;chrome.runtime.onConnectExternal.addListener(function(t){n.push(t);t.onMessage.addListener(s);t.onDisconnect.addListener(h);r&&(clearTimeout(r),r=null)});e=function(){var f=null,t=!1,n=null,i=[],r=null,o=function(t,i){if(n){var r=_.findWhere(n,{id:t});r&&(r.onlinecount=i)}},e=function(){for(var t;i.length;)(t=i.pop())&&t(n)},s=function(u){u&&(f&&r&&new Date-r<6e5?u(n):(i.push(u),t||(t=!0,$.get(CFG_MANGER.config.chatServerApi,null,null,"json").done(function(i){n=i;t=!1;f=!0;e()}).fail(function(){n=null;t=!1;e()}).always(function(){r=new Date}))))};return chrome.runtime.onMessage.addListener(function(n){n&&n.action&&(n.action==="serverStateChange"||n.action==="sysConfigUpdate")&&u(n)}),{loadServers:s,setRoomCount:o}}();f=function(){var t=null,n=null,s=this,i,e=function(f){t=f;var e=t.url;n!==null&&f.id!==i&&r();i=f.id;n===null?(n=new WebSocket(e,[]),n.binaryType="arraybuffer",n.onclose=function(){n=null;u({action:"chatRoomDisconnected"})},n.onopen=function(){u({action:"chatRoomConnected"})},n.onmessage=function(n){for(var i=new Uint8Array(n.data),r=[],t=0;t<i.length;t++)r.push(i[t]);u({action:"chatRoomReceiveMsg",detail:{buffer:r}})},n.onerror=function(){try{n.close()}catch(t){}},u({action:"chatRoomConnecting"})):u({action:"chatRoomConnected"})},o=function(t){n!==null&&n.send(new Uint8Array(t).buffer)},r=function(){n&&(n.close(),n=null);t=null;i=null},f={connect:e,disconnect:r,sendMsg:o};return Object.defineProperty(f,"currentRoom",{get:function(){return t}}),f}()}(),function(){var n=JSON.parse(localStorage.alarm||"[]"),f="/infobar/theme/plus_16.png",e="/icons/icon_n.png",r=null,t={},u=function(){localStorage.alarm=JSON.stringify(n)},o=function(n){if(t[n]){var i=t[n],r=i.data;delete t[n];chrome.tabs.create({active:!0,url:"http://12306.liebao.cn/#ALARM-"+encodeURIComponent(JSON.stringify(r))})}},i;chrome.notifications.onButtonClicked.addListener(o);i=function(){var o,s,h;for(r=null,o=!1,s={};n.length>0&&n[0].time<=(new Date).getTime();)h=n.shift(),s[h.group]=h,o=!0;o&&_.each(s,function(n){var i="ALARM-"+n.data.fromCode+"-"+n.data.toCode+(new Date).getTime();chrome.runtime.sendMessage({action:"track",detail:{type:132,values:[n.type||0,n.point||0]}});chrome.notifications.create(i,{type:"basic",iconUrl:e,title:n.group,message:n.text,buttons:[{title:"立刻打开订票页面",iconUrl:f}]},function(r){i=r;t[i]=n;setTimeout(function(){delete t[r];chrome.notifications.clear(r,function(){})},1e4)})});n.length&&(r=setTimeout(i,3e4));o&&u()};chrome.runtime.onMessageExternal.addListener(function(t){if(t&&t.action&&t.action==="setAlarmTask"){var f=t.detail;_.each(f,function(t){n.push(t)});n.sort(function(n,t){return n.time-t.time});u();r||i()}});i()}()