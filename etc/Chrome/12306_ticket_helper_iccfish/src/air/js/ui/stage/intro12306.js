(new Gallery).addStage("intro12306",{header:{title:"抢火车票"},init:function(){$("#cn12306Otn").attr("data-targeturl","https://kyfw.12306.cn/otn/");$("#cn12306About").attr("data-targeturl",chrome.extension.getURL("/pages/about.html"));$("#cn12306Sppeding").attr("data-targeturl",chrome.extension.getURL("/pages/serverstatus.html"));$("div.intro12306 li[data-targeturl]").click(function(){var n=this.dataset.targeturl;chrome.tabs.create({url:n});window.close()})},prepare:function(){}})