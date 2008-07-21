(function(){var each=tinymce.each;tinymce.create('tinymce.plugins.MediaPlugin',{init:function(ed,url){var t=this;t.editor=ed;t.url=url;function isMediaElm(n){return/^(mceItemFlash|mceItemShockWave|mceItemWindowsMedia|mceItemQuickTime|mceItemRealMedia)$/.test(n.className);};ed.onPreInit.add(function(){ed.serializer.addRules('param[name|value|_value]');});ed.addCommand('mceMedia',function(){ed.windowManager.open({file:url+'/media.htm',width:430+parseInt(ed.getLang('media.delta_width',0)),height:470+parseInt(ed.getLang('media.delta_height',0)),inline:1},{plugin_url:url});});ed.addButton('media',{title:'media.desc',cmd:'mceMedia'});ed.onNodeChange.add(function(ed,cm,n){cm.setActive('media',n.nodeName=='IMG'&&isMediaElm(n));});ed.onInit.add(function(){var lo={mceItemFlash:'flash',mceItemShockWave:'shockwave',mceItemWindowsMedia:'windowsmedia',mceItemQuickTime:'quicktime',mceItemRealMedia:'realmedia'};if(ed.settings.content_css!==false)ed.dom.loadCSS(url+"/css/content.css");if(ed.theme.onResolveName){ed.theme.onResolveName.add(function(th,o){if(o.name=='img'){each(lo,function(v,k){if(ed.dom.hasClass(o.node,k)){o.name=v;o.title=ed.dom.getAttrib(o.node,'title');return false;}});}});}if(ed&&ed.plugins.contextmenu){ed.plugins.contextmenu.onContextMenu.add(function(th,m,e){if(e.nodeName=='IMG'&&/mceItem(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)/.test(e.className)){m.add({title:'media.edit',icon:'media',cmd:'mceMedia'});}});}});ed.onBeforeSetContent.add(function(ed,o){var h=o.content;h=h.replace(/<script[^>]*>\s*write(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)\(\{([^\)]*)\}\);\s*<\/script>/gi,function(a,b,c){var o=t._parse(c);return'<img class="mceItem'+b+'" title="'+ed.dom.encode(c)+'" src="'+url+'/img/trans.gif" width="'+o.width+'" height="'+o.height+'" />'});h=h.replace(/<object([^>]*)>/gi,'<span class="mceItemObject" $1>');h=h.replace(/<embed([^>]*)\/?>/gi,'<span class="mceItemEmbed" $1></span>');h=h.replace(/<embed([^>]*)>/gi,'<span class="mceItemEmbed" $1>');h=h.replace(/<\/(object)([^>]*)>/gi,'</span>');h=h.replace(/<\/embed>/gi,'');h=h.replace(/<param([^>]*)>/gi,function(a,b){return'<span '+b.replace(/value=/gi,'_value=')+' class="mceItemParam"></span>'});h=h.replace(/\/ class=\"mceItemParam\"><\/span>/gi,'class="mceItemParam"></span>');o.content=h;});ed.onSetContent.add(function(){t._spansToImgs(ed.getBody());});ed.onPreProcess.add(function(ed,o){var dom=ed.dom;if(o.set){t._spansToImgs(o.node);each(dom.select('IMG',o.node),function(n){var p;if(isMediaElm(n)){p=t._parse(n.title);dom.setAttrib(n,'width',dom.getAttrib(n,'width',p.width||100));dom.setAttrib(n,'height',dom.getAttrib(n,'height',p.height||100));}});}if(o.get){each(dom.select('IMG',o.node),function(n){var ci,cb,mt;if(ed.getParam('media_use_script')){if(isMediaElm(n))n.className=n.className.replace(/mceItem/g,'mceTemp');return;}switch(n.className){case'mceItemFlash':ci='d27cdb6e-ae6d-11cf-96b8-444553540000';cb='http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0';mt='application/x-shockwave-flash';break;case'mceItemShockWave':ci='166b1bca-3f9c-11cf-8075-444553540000';cb='http://download.macromedia.com/pub/shockwave/cabs/director/sw.cab#version=8,5,1,0';mt='application/x-director';break;case'mceItemWindowsMedia':ci=ed.getParam('media_wmp6_compatible')?'05589fa1-c356-11ce-bf01-00aa0055595a':'6bf52a52-394a-11d3-b153-00c04f79faa6';cb='http://activex.microsoft.com/activex/controls/mplayer/en/nsmp2inf.cab#Version=5,1,52,701';mt='application/x-mplayer2';break;case'mceItemQuickTime':ci='02bf25d5-8c17-4b23-bc80-d3488abddc6b';cb='http://www.apple.com/qtactivex/qtplugin.cab#version=6,0,2,0';mt='video/quicktime';break;case'mceItemRealMedia':ci='cfcdaa03-8be4-11cf-b84b-0020afbbccfa';cb='http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0';mt='audio/x-pn-realaudio-plugin';break;}if(ci){dom.replace(t._buildObj({classid:ci,codebase:cb,type:mt},n),n);}});}});ed.onPostProcess.add(function(ed,o){o.content=o.content.replace(/_value=/g,'value=');});if(ed.getParam('media_use_script')){function getAttr(s,n){n=new RegExp(n+'=\"([^\"]+)\"','g').exec(s);return n?ed.dom.decode(n[1]):'';};ed.onPostProcess.add(function(ed,o){o.content=o.content.replace(/<img[^>]+>/g,function(im){var cl=getAttr(im,'class');if(/^(mceTempFlash|mceTempShockWave|mceTempWindowsMedia|mceTempQuickTime|mceTempRealMedia)$/.test(cl)){at=t._parse(getAttr(im,'title'));at.width=getAttr(im,'width');at.height=getAttr(im,'height');im='<script type="text/javascript">write'+cl.substring(7)+'({'+t._serialize(at)+'});</script>';}return im;});});}},getInfo:function(){return{longname:'Media',author:'Moxiecode Systems AB',authorurl:'http://tinymce.moxiecode.com',infourl:'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/media',version:tinymce.majorVersion+"."+tinymce.minorVersion};},_buildObj:function(o,n){var ob,ed=this.editor,dom=ed.dom,p=this._parse(n.title);p.width=o.width=dom.getAttrib(n,'width')||100;p.height=o.height=dom.getAttrib(n,'height')||100;ob=dom.create('span',{mce_name:'object',classid:"clsid:"+o.classid,codebase:o.codebase,width:o.width,height:o.height});if(p.src)p.src=ed.convertURL(p.src,'src',n);each(p,function(v,k){if(!/^(width|height|codebase|classid)$/.test(k)){if(o.type=='application/x-mplayer2'&&k=='src')k='url';dom.add(ob,'span',{mce_name:'param',name:k,'_value':v});}});dom.add(ob,'span',tinymce.extend({mce_name:'embed',type:o.type},p));return ob;},_spansToImgs:function(p){var t=this,dom=t.editor.dom,im,ci;each(dom.select('span',p),function(n){if(dom.getAttrib(n,'class')=='mceItemObject'){ci=dom.getAttrib(n,"classid").toLowerCase().replace(/\s+/g,'');switch(ci){case'clsid:d27cdb6e-ae6d-11cf-96b8-444553540000':dom.replace(t._createImg('mceItemFlash',n),n);break;case'clsid:166b1bca-3f9c-11cf-8075-444553540000':dom.replace(t._createImg('mceItemShockWave',n),n);break;case'clsid:6bf52a52-394a-11d3-b153-00c04f79faa6':case'clsid:22d6f312-b0f6-11d0-94ab-0080c74c7e95':case'clsid:05589fa1-c356-11ce-bf01-00aa0055595a':dom.replace(t._createImg('mceItemWindowsMedia',n),n);break;case'clsid:02bf25d5-8c17-4b23-bc80-d3488abddc6b':dom.replace(t._createImg('mceItemQuickTime',n),n);break;case'clsid:cfcdaa03-8be4-11cf-b84b-0020afbbccfa':dom.replace(t._createImg('mceItemRealMedia',n),n);break;default:dom.replace(t._createImg('mceItemFlash',n),n);}return;}if(dom.getAttrib(n,'class')=='mceItemEmbed'){switch(dom.getAttrib(n,'type')){case'application/x-shockwave-flash':dom.replace(t._createImg('mceItemFlash',n),n);break;case'application/x-director':dom.replace(t._createImg('mceItemShockWave',n),n);break;case'application/x-mplayer2':dom.replace(t._createImg('mceItemWindowsMedia',n),n);break;case'video/quicktime':dom.replace(t._createImg('mceItemQuickTime',n),n);break;case'audio/x-pn-realaudio-plugin':dom.replace(t._createImg('mceItemRealMedia',n),n);break;default:dom.replace(t._createImg('mceItemFlash',n),n);}}});},_createImg:function(cl,n){var im,dom=this.editor.dom,pa={},ti='';im=dom.create('img',{src:this.url+'/img/trans.gif',width:dom.getAttrib(n,'width')||100,height:dom.getAttrib(n,'height')||100,'class':cl});each(['id','name','width','height','bgcolor','align','flashvars','src','wmode'],function(na){var v=dom.getAttrib(n,na);if(v)pa[na]=v;});each(dom.select('span',n),function(n){if(dom.hasClass(n,'mceItemParam'))pa[dom.getAttrib(n,'name')]=dom.getAttrib(n,'_value');});if(pa.movie){pa.src=pa.movie;delete pa.movie;}delete pa.width;delete pa.height;im.title=this._serialize(pa);return im;},_parse:function(s){return tinymce.util.JSON.parse('{'+s+'}');},_serialize:function(o){return tinymce.util.JSON.serialize(o).replace(/[{}]/g,'');}});tinymce.PluginManager.add('media',tinymce.plugins.MediaPlugin);})();