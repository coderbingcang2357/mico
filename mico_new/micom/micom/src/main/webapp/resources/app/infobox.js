var boxzindex__ = 10000;
function ShowInformation(title, inf, width, height) {
    boxzindex__ += 1;
    var w = width || 300;
    var h = height || 200;
    var dlgstr=
        '<div style="display:none;background-color:rgba(51,51,51,0.7);position:fixed;z-index:{0};top:0px;left:0px;right:0px;bottom:0px;">\
            <div class="box box-primary" style="left:30%;top:20%;width:{1}px;height:{2}px;">\
                <div class="box-header with-border">\
                  <h3 class="box-title">{3}</h3>\
                  <div class="box-tools pull-right">\
                    <button type="button" class="btn btn-box-tool closeDlg"><i class="fa fa-times"></i></button>\
                  </div>\
                </div>\
                <div class="box-body">\
                    <span>{4}</span>\
                </div>\
                <div class="box-footer">\
                  <form action="#" method="post">\
                    <div class="input-group">\
                          <span class="input-group-btn">\
                            <button type="button" class="btn btn-primary btn-flat closeDlg pull-right">OK</button>\
                          </span>\
                    </div>\
                  </form>\
                </div>\
            </div>\
        </div>'.format(boxzindex__, w, h, title, inf);
    var dlg = $(dlgstr);
    dlg.appendTo($("body"));
    dlg.find(".closeDlg").click(function () {
        dlg.fadeOut(function () {
           dlg.remove();     
        });
    });
    dlg.fadeIn();
}