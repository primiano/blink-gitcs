(function(){tinymce.create("tinymce.plugins.wpGallery",{init:function(a,b){var c=this;c.url=b;c._createButtons();a.addCommand("WP_Gallery",function(){var h=a.selection.getNode(),f,e=tinymce.DOM.getViewPort(),g=e.h-80,d=(640<e.w)?640:e.w;if(h.nodeName!="IMG"){return}if(a.dom.getAttrib(h,"class").indexOf("wpGallery")==-1){return}f=tinymce.DOM.get("post_ID").value;tb_show("",tinymce.documentBaseURL+"/media-upload.php?post_id="+f+"&tab=gallery&TB_iframe=true&width="+d+"&height="+g);tinymce.DOM.setStyle(["TB_overlay","TB_window","TB_load"],"z-index","999999")});a.onInit.add(function(d){tinymce.dom.Event.add(d.getWin(),"scroll",function(f){d.plugins.wpgallery.hideButtons()})});a.onBeforeExecCommand.add(function(d,f,e,g){d.plugins.wpgallery.hideButtons()});a.onSaveContent.add(function(d,e){d.plugins.wpgallery.hideButtons()});a.onMouseUp.add(function(d,f){if(tinymce.isOpera){if(f.target.nodeName=="IMG"){d.plugins.wpgallery.showButtons(f.target)}}});a.onMouseDown.add(function(d,f){if(tinymce.isOpera||f.target.nodeName!="IMG"){c.hideButtons();return}d.plugins.wpgallery.showButtons(f.target)});a.onBeforeSetContent.add(function(d,e){e.content=c._do_gallery(e.content)});a.onPostProcess.add(function(d,e){if(e.get){e.content=c._get_gallery(e.content)}})},_do_gallery:function(a){return a.replace(/\[gallery([^\]]*)\]/g,function(d,c){return'<img src="'+tinymce.baseURL+'/plugins/wpgallery/img/t.gif" class="wpGallery mceItem" title="gallery'+tinymce.DOM.encode(c)+'" />'})},_get_gallery:function(b){function a(c,d){d=new RegExp(d+'="([^"]+)"',"g").exec(c);return d?tinymce.DOM.decode(d[1]):""}return b.replace(/(?:<p[^>]*>)*(<img[^>]+>)(?:<\/p>)*/g,function(e,d){var c=a(d,"class");if(c.indexOf("wpGallery")!=-1){return"<p>["+tinymce.trim(a(d,"title"))+"]</p>"}return e})},showButtons:function(d){var i=this,e=tinyMCE.activeEditor,g,f,a,h=tinymce.DOM,c,b;if(e.dom.getAttrib(d,"class").indexOf("wpGallery")==-1){return}a=e.dom.getViewPort(e.getWin());g=h.getPos(e.getContentAreaContainer());f=e.dom.getPos(d);c=Math.max(f.x-a.x,0)+g.x;b=Math.max(f.y-a.y,0)+g.y;h.setStyles("wp_gallerybtns",{top:b+5+"px",left:c+5+"px",display:"block"});i.btnsTout=window.setTimeout(function(){e.plugins.wpgallery.hideButtons()},5000)},hideButtons:function(){if(tinymce.DOM.isHidden("wp_gallerybtns")){return}tinymce.DOM.hide("wp_gallerybtns");window.clearTimeout(this.btnsTout)},_createButtons:function(){var d=this,b=tinyMCE.activeEditor,e=tinymce.DOM,c,f,a;e.remove("wp_gallerybtns");c=e.add(document.body,"div",{id:"wp_gallerybtns",style:"display:none;"});f=e.add("wp_gallerybtns","img",{src:d.url+"/img/edit.png",id:"wp_editgallery",width:"24",height:"24",title:b.getLang("wordpress.editgallery")});f.onmousedown=function(h){var g=tinyMCE.activeEditor;g.windowManager.bookmark=g.selection.getBookmark("simple");g.execCommand("WP_Gallery");this.parentNode.style.display="none"};a=e.add("wp_gallerybtns","img",{src:d.url+"/img/delete.png",id:"wp_delgallery",width:"24",height:"24",title:b.getLang("wordpress.delgallery")});a.onmousedown=function(i){var g=tinyMCE.activeEditor,h=g.selection.getNode();if(h.nodeName=="IMG"&&g.dom.getAttrib(h,"class").indexOf("wpGallery")!=-1){g.dom.remove(h);this.parentNode.style.display="none";g.execCommand("mceRepaint");return false}}},getInfo:function(){return{longname:"Gallery Settings",author:"WordPress",authorurl:"http://wordpress.org",infourl:"",version:"1.0"}}});tinymce.PluginManager.add("wpgallery",tinymce.plugins.wpGallery)})();